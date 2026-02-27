
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

TEST_CASE( "YgorIOgzip interoperability with known-good gzip data" ){
    // These byte sequences were generated by Python's gzip module and the
    // system gzip command.  Verifying that our decompressor can handle them
    // ensures RFC 1952 / RFC 1951 compliance and interoperability.

    SUBCASE("decompress known-good gzip: Hello, World! (Python gzip)"){
        // "Hello, World!" compressed by Python's gzip module with mtime=0.
        static constexpr uint8_t known_good_gz[] = {
            0x1FU,0x8BU,0x08U,0x00U,0x00U,0x00U,0x00U,0x00U,0x02U,0xFFU,0xF3U,0x48U,
            0xCDU,0xC9U,0xC9U,0xD7U,0x51U,0x08U,0xCFU,0x2FU,0xCAU,0x49U,0x51U,0x04U,
            0x00U,0xD0U,0xC3U,0x4AU,0xECU,0x0DU,0x00U,0x00U,0x00U
        };
        std::string gz_data(reinterpret_cast<const char *>(known_good_gz), sizeof(known_good_gz));
        std::istringstream iss(gz_data);
        ygor::io::gzip_istream gis(iss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "Hello, World!");
    }

    SUBCASE("decompress known-good gzip: pangram (Python gzip)"){
        // "The quick brown fox jumps over the lazy dog\n" compressed by Python.
        static constexpr uint8_t known_good_fox_gz[] = {
            0x1FU,0x8BU,0x08U,0x00U,0x00U,0x00U,0x00U,0x00U,0x02U,0xFFU,0x0BU,0xC9U,
            0x48U,0x55U,0x28U,0x2CU,0xCDU,0x4CU,0xCEU,0x56U,0x48U,0x2AU,0xCAU,0x2FU,
            0xCFU,0x53U,0x48U,0xCBU,0xAFU,0x50U,0xC8U,0x2AU,0xCDU,0x2DU,0x28U,0x56U,
            0xC8U,0x2FU,0x4BU,0x2DU,0x52U,0x28U,0x01U,0x4AU,0xE7U,0x24U,0x56U,0x55U,
            0x2AU,0xA4U,0xE4U,0xA7U,0x73U,0x01U,0x00U,0x38U,0xC1U,0x93U,0x6DU,0x2CU,
            0x00U,0x00U,0x00U
        };
        std::string gz_data(reinterpret_cast<const char *>(known_good_fox_gz), sizeof(known_good_fox_gz));
        std::istringstream iss(gz_data);
        ygor::io::gzip_istream gis(iss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "The quick brown fox jumps over the lazy dog\n");
    }

    SUBCASE("decompress known-good gzip: empty data (Python gzip)"){
        static constexpr uint8_t known_good_empty_gz[] = {
            0x1FU,0x8BU,0x08U,0x00U,0x00U,0x00U,0x00U,0x00U,0x02U,0xFFU,0x03U,0x00U,
            0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U
        };
        std::string gz_data(reinterpret_cast<const char *>(known_good_empty_gz), sizeof(known_good_empty_gz));
        std::istringstream iss(gz_data);
        ygor::io::gzip_istream gis(iss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result.empty());
    }

    SUBCASE("decompress known-good gzip: from system gzip command"){
        // "Test data from system gzip" compressed by system gzip -n.
        static constexpr uint8_t system_gzip_gz[] = {
            0x1FU,0x8BU,0x08U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x03U,0x0BU,0x49U,
            0x2DU,0x2EU,0x51U,0x48U,0x49U,0x2CU,0x49U,0x54U,0x48U,0x2BU,0xCAU,0xCFU,
            0x55U,0x28U,0xAEU,0x2CU,0x2EU,0x49U,0xCDU,0x55U,0x48U,0xAFU,0xCAU,0x2CU,
            0x00U,0x00U,0xF0U,0xD2U,0x86U,0x37U,0x1AU,0x00U,0x00U,0x00U
        };
        std::string gz_data(reinterpret_cast<const char *>(system_gzip_gz), sizeof(system_gzip_gz));
        std::istringstream iss(gz_data);
        ygor::io::gzip_istream gis(iss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "Test data from system gzip");
    }
}

TEST_CASE( "YgorIOgzip system gzip interoperability" ){
    const auto tmpdir = std::filesystem::temp_directory_path();
    const auto tmpgz  = (tmpdir / ("ygor_gzip_sysinterop_" + std::to_string(::getpid()) + ".gz")).string();
    const auto tmpout = (tmpdir / ("ygor_gzip_sysinterop_" + std::to_string(::getpid()) + ".out")).string();

    SUBCASE("our compressor output is valid for system gunzip"){
        const std::string input = "System interop test: The quick brown fox jumps over the lazy dog. 0123456789";
        {
            std::ofstream ofs(tmpgz, std::ios::out | std::ios::trunc | std::ios::binary);
            REQUIRE(ofs.good());
            ygor::io::gzip_ostream gos(ofs);
            gos << input;
        }
        // Use system gzip -d to decompress.
        std::string cmd = "gzip -d -c '" + tmpgz + "' > '" + tmpout + "' 2>&1";
        REQUIRE(std::system(cmd.c_str()) == 0);

        std::ifstream ifs(tmpout, std::ios::binary);
        std::string result((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);

        std::remove(tmpgz.c_str());
        std::remove(tmpout.c_str());
    }

    SUBCASE("system gzip output is valid for our decompressor"){
        const std::string input = "System gzip compressed this string. 0123456789 ABCDEFGHIJ";
        // Write input to a temp file, gzip it, then decompress with our code.
        auto tmpsrc = (tmpdir / ("ygor_gzip_sysinterop_src_" + std::to_string(::getpid()))).string();
        {
            std::ofstream ofs(tmpsrc, std::ios::binary);
            ofs << input;
        }
        std::string cmd = "gzip -n -c '" + tmpsrc + "' > '" + tmpgz + "' 2>&1";
        REQUIRE(std::system(cmd.c_str()) == 0);

        std::ifstream ifs(tmpgz, std::ios::binary);
        REQUIRE(ifs.good());
        ygor::io::gzip_istream gis(ifs);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == input);

        std::remove(tmpsrc.c_str());
        std::remove(tmpgz.c_str());
    }

    SUBCASE("large data round-trip via system gzip"){
        // Exercise multiple DEFLATE blocks with data exceeding 64 KB.
        std::string input;
        for(int i = 0; i < 100000; ++i){
            input += static_cast<char>('a' + (i % 26));
            if(i % 80 == 79) input += '\n';
        }
        {
            std::ofstream ofs(tmpgz, std::ios::out | std::ios::trunc | std::ios::binary);
            REQUIRE(ofs.good());
            ygor::io::gzip_ostream gos(ofs);
            gos.write(input.data(), static_cast<std::streamsize>(input.size()));
        }
        // Validate via system gzip -t (integrity check).
        std::string cmd = "gzip -t '" + tmpgz + "' 2>&1";
        REQUIRE(std::system(cmd.c_str()) == 0);

        // Also round-trip decompress with our code.
        {
            std::ifstream ifs(tmpgz, std::ios::binary);
            ygor::io::gzip_istream gis(ifs);
            std::string result((std::istreambuf_iterator<char>(gis)),
                                std::istreambuf_iterator<char>());
            REQUIRE(result == input);
        }

        std::remove(tmpgz.c_str());
    }
}

TEST_CASE( "YgorIOgzip streaming and large data" ){

    SUBCASE("data much larger than internal block size"){
        // 200 KB of varied data to exercise multiple DEFLATE blocks.
        std::string input;
        for(int i = 0; i < 200000; ++i){
            input += static_cast<char>(i & 0xFF);
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

    SUBCASE("incremental read via get()"){
        const std::string input = "ABCDEF";
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << input;
        }
        ygor::io::gzip_istream gis(ss);
        for(char expected : input){
            int ch = gis.get();
            REQUIRE(ch != std::char_traits<char>::eof());
            REQUIRE(static_cast<char>(ch) == expected);
        }
        REQUIRE(gis.get() == std::char_traits<char>::eof());
    }

    SUBCASE("explicit flush between writes"){
        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos << "part1";
            gos.flush();
            gos << "part2";
            gos.flush();
            gos << "part3";
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == "part1part2part3");
    }

    SUBCASE("all byte values round-trip"){
        // Ensure every possible byte value survives compression.
        std::string input;
        for(int i = 0; i < 256; ++i){
            input += static_cast<char>(i);
        }
        // Repeat to give the compressor something to work with.
        std::string repeated;
        for(int r = 0; r < 100; ++r) repeated += input;

        std::stringstream ss;
        {
            ygor::io::gzip_ostream gos(ss);
            gos.write(repeated.data(), static_cast<std::streamsize>(repeated.size()));
        }

        ygor::io::gzip_istream gis(ss);
        std::string result((std::istreambuf_iterator<char>(gis)),
                            std::istreambuf_iterator<char>());
        REQUIRE(result == repeated);
    }
}
