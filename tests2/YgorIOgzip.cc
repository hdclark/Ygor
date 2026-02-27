
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include <YgorIOgzip.h>

#include "doctest/doctest.h"


TEST_CASE( "YgorIOgzip round-trip compression and decompression" ){

    SUBCASE("empty data"){
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            // Write nothing.
        }
        REQUIRE(ss.str().size() > 0); // Should have gzip header/trailer.

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result.empty());
    }

    SUBCASE("short string"){
        const std::string input = "Hello, world!";
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << input;
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);
    }

    SUBCASE("multiple writes"){
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << "Hello, ";
            gos << "world!";
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "Hello, world!");
    }

    SUBCASE("binary data"){
        std::vector<uint8_t> input;
        for(int i = 0; i < 256; ++i){
            input.push_back(static_cast<uint8_t>(i));
        }
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos.write(reinterpret_cast<const char *>(input.data()),
                      static_cast<std::streamsize>(input.size()));
        }

        ygor::io::gzip_istream gis(ss);
        std::vector<uint8_t> result((std::istreambuf_iterator<char>(gis)),
                                     std::istreambuf_iterator<char>());
        REQUIRE(result == input);
    }

    SUBCASE("repeated data compresses well"){
        std::string input;
        for(int i = 0; i < 1000; ++i){
            input += "ABCDEFGHIJ";
        }
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << input;
        }

        // Compressed size should be much smaller than input.
        REQUIRE(ss.str().size() < input.size() / 2);

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);
    }

    SUBCASE("larger data with varied content"){
        std::string input;
        for(int i = 0; i < 10000; ++i){
            input += static_cast<char>('A' + (i % 26));
            if(i % 80 == 79) input += '\n';
        }
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << input;
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);
    }

    SUBCASE("data with newlines and getline"){
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << "line one\n";
            gos << "line two\n";
            gos << "line three\n";
        }

        ygor::io::gzip_istream gis(ss);
        std::string line;
        REQUIRE(std::getline(gis, line));
        REQUIRE(line == "line one");
        REQUIRE(std::getline(gis, line));
        REQUIRE(line == "line two");
        REQUIRE(std::getline(gis, line));
        REQUIRE(line == "line three");
        REQUIRE(!std::getline(gis, line)); // EOF.
    }

    SUBCASE("single byte"){
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos.put('X');
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "X");
    }

    SUBCASE("data exceeding initial buffer size"){
        // Write enough data to overflow the internal buffer multiple times.
        std::string input;
        for(int i = 0; i < 50000; ++i){
            input += static_cast<char>('a' + (i % 26));
        }
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos.write(input.data(), static_cast<std::streamsize>(input.size()));
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);
    }
}

TEST_CASE( "YgorIOgzip file-based round-trip" ){
    // Generate a unique temp file path to avoid races under parallel test runs.
    const auto tmpdir = std::filesystem::temp_directory_path();
    const auto tmpfile = (tmpdir / ("ygor_gzip_test_" + std::to_string(::getpid()) + ".gz")).string();

    SUBCASE("write and read file"){
        const std::string input = "File-based gzip test with some data: 1234567890";

        {
            std::ofstream ofs(tmpfile, std::ios::out | std::ios::trunc | std::ios::binary);
            REQUIRE(ofs.good());
            ygor::io::gzip_ostream gos(ofs);
            gos << input;
        }

        {
            std::ifstream ifs(tmpfile, std::ios::in | std::ios::binary);
            REQUIRE(ifs.good());
            ygor::io::gzip_istream gis(ifs);
            std::string result((std::istreambuf_iterator<char>(gis)),
                                std::istreambuf_iterator<char>());
            REQUIRE(result == input);
        }

        std::remove(tmpfile.c_str());
    }
}

TEST_CASE( "YgorIOgzip error handling" ){

    SUBCASE("invalid gzip data throws"){
        std::stringstream ss("this is not gzip data at all!!");
        REQUIRE_THROWS_AS( (ygor::io::gzip_istream{ss}) , std::runtime_error);
    }

    SUBCASE("truncated gzip data throws"){
        std::stringstream ss;
        ss.put(static_cast<char>(0x1F));
        ss.put(static_cast<char>(0x8B));
        REQUIRE_THROWS_AS( (ygor::io::gzip_istream{ss}) , std::runtime_error);
    }
}
