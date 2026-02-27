//YgorIOgzip.h - Written by hal clark in 2026.
//
// This file provides gzip compression and decompression stream wrappers
// using pure C++17 without external dependencies. The implementation
// produces and consumes standard gzip files (RFC 1952) using the
// DEFLATE algorithm (RFC 1951).
//

#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>

namespace ygor {
namespace io {

// Forward declarations for implementation details.
namespace gzip_impl {
    class compress_streambuf;
    class decompress_streambuf;
} // namespace gzip_impl.

// A std::ostream that gzip-compresses all data written to it.
//
// Usage:
//     std::ofstream ofs("file.gz", std::ios::binary);
//     ygor::io::gzip_ostream gofs(ofs);
//     gofs << "Hello, world!" << std::endl;
//     gofs.flush();  // or let destructor finalize
//
class gzip_ostream : public std::ostream {
  public:
    explicit gzip_ostream(std::ostream &sink);
    ~gzip_ostream() override;

    gzip_ostream(const gzip_ostream &) = delete;
    gzip_ostream &operator=(const gzip_ostream &) = delete;

  private:
    std::unique_ptr<gzip_impl::compress_streambuf> buf_;
};

// A std::istream that gzip-decompresses data read from it.
//
// Usage:
//     std::ifstream ifs("file.gz", std::ios::binary);
//     ygor::io::gzip_istream gifs(ifs);
//     std::string line;
//     std::getline(gifs, line);
//
class gzip_istream : public std::istream {
  public:
    explicit gzip_istream(std::istream &source);
    ~gzip_istream() override;

    gzip_istream(const gzip_istream &) = delete;
    gzip_istream &operator=(const gzip_istream &) = delete;

  private:
    std::unique_ptr<gzip_impl::decompress_streambuf> buf_;
};

} // namespace io.
} // namespace ygor.
