//YgorIOgzip.cc - A part of Ygor, 2026. Written by hal clark.
//
// Pure C++17 implementation of gzip compression and decompression.
// Implements RFC 1952 (gzip) and RFC 1951 (DEFLATE).
//

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "YgorIOgzip.h"

namespace ygor {
namespace io {
namespace gzip_impl {

// -----------------------------------------------------------------------
// CRC-32 (ISO 3309 / ITU-T V.42, used by gzip).
// -----------------------------------------------------------------------
static std::array<uint32_t, 256> make_crc32_table(){
    std::array<uint32_t, 256> t{};
    for(uint32_t n = 0U; n < 256U; ++n){
        uint32_t c = n;
        for(int k = 0; k < 8; ++k){
            if(c & 1U){
                c = 0xEDB88320U ^ (c >> 1U);
            }else{
                c >>= 1U;
            }
        }
        t[n] = c;
    }
    return t;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len){
    static const auto table = make_crc32_table();
    crc = ~crc;
    for(size_t i = 0U; i < len; ++i){
        crc = table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return ~crc;
}

// -----------------------------------------------------------------------
// Bit-level writer (LSB-first, as required by DEFLATE).
//
// Per RFC 1951 section 3.1.1, data elements other than Huffman codes are
// packed starting with the least-significant bit of the data element.
// Huffman codes are packed starting with the most-significant bit of the
// code.  In both cases, the bits are packed into bytes starting from the
// least-significant bit of the byte.
//
// This writer takes the value argument and emits its bits starting from
// the value's least-significant bit.  For non-Huffman data this is the
// natural order.  For Huffman codes, the caller must bit-reverse the
// canonical (MSB-first) code before passing it here so that the code's
// MSB ends up at the lowest bit position in the output stream.
// -----------------------------------------------------------------------
class bit_writer {
  public:
    explicit bit_writer(std::vector<uint8_t> &out) : out_(out) {}

    void write_bits(uint32_t value, int nbits){
        while(nbits > 0){
            int space = 8 - bit_pos_;
            int take = std::min(nbits, space);
            uint32_t mask = (1U << take) - 1U;
            accum_ |= static_cast<uint8_t>((value & mask) << bit_pos_);
            bit_pos_ += take;
            value >>= take;
            nbits -= take;
            if(bit_pos_ == 8){
                out_.push_back(accum_);
                accum_ = 0;
                bit_pos_ = 0;
            }
        }
    }

    void flush_to_byte(){
        if(bit_pos_ > 0){
            out_.push_back(accum_);
            accum_ = 0;
            bit_pos_ = 0;
        }
    }

  private:
    std::vector<uint8_t> &out_;
    uint8_t accum_ = 0;
    int bit_pos_ = 0;
};

// -----------------------------------------------------------------------
// Bit-level reader (LSB-first, as required by DEFLATE).
// -----------------------------------------------------------------------
class bit_reader {
  public:
    bit_reader() = default;

    explicit bit_reader(const uint8_t *data, size_t len)
        : data_(data), len_(len) {}

    uint32_t read_bits(int nbits){
        uint32_t result = 0;
        int shift = 0;
        while(nbits > 0){
            if(byte_pos_ >= len_) throw std::runtime_error("gzip: unexpected end of compressed data");
            int avail = 8 - bit_pos_;
            int take = std::min(nbits, avail);
            uint32_t mask = (1U << take) - 1U;
            result |= ((static_cast<uint32_t>(data_[byte_pos_]) >> bit_pos_) & mask) << shift;
            bit_pos_ += take;
            shift += take;
            nbits -= take;
            if(bit_pos_ == 8){
                bit_pos_ = 0;
                ++byte_pos_;
            }
        }
        return result;
    }

    // Return nbits bits to the reader (rewind position).
    void put_back_bits(int nbits){
        int total = static_cast<int>(byte_pos_) * 8 + bit_pos_;
        total -= nbits;
        if(total < 0) throw std::runtime_error("gzip: cannot rewind past beginning of data");
        byte_pos_ = static_cast<size_t>(total / 8);
        bit_pos_ = total % 8;
    }

    void align_to_byte(){
        if(bit_pos_ > 0){
            bit_pos_ = 0;
            ++byte_pos_;
        }
    }

    uint8_t read_byte(){
        align_to_byte();
        if(byte_pos_ >= len_) throw std::runtime_error("gzip: unexpected end of data");
        return data_[byte_pos_++];
    }

    uint16_t read_u16(){
        uint16_t lo = read_byte();
        uint16_t hi = read_byte();
        return static_cast<uint16_t>(lo | (hi << 8U));
    }

    uint32_t read_u32_le(){
        uint32_t b0 = read_byte();
        uint32_t b1 = read_byte();
        uint32_t b2 = read_byte();
        uint32_t b3 = read_byte();
        return b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U);
    }

  private:
    const uint8_t *data_ = nullptr;
    size_t len_ = 0;
    size_t byte_pos_ = 0;
    int bit_pos_ = 0;
};

// -----------------------------------------------------------------------
// Huffman decoder used for DEFLATE decompression.
// -----------------------------------------------------------------------
struct huffman_decoder {
    // Build a Huffman decoder from an array of code lengths.
    // code_lengths[i] = bit length of code for symbol i (0 = not used).
    void build(const std::vector<int> &code_lengths){
        int max_bits = 0;
        for(auto cl : code_lengths) if(cl > max_bits) max_bits = cl;
        if(max_bits == 0){
            max_bits_ = 0;
            return;
        }
        max_bits_ = max_bits;

        // Count codes of each length.
        std::vector<int> bl_count(static_cast<size_t>(max_bits + 1), 0);
        for(auto cl : code_lengths){
            if(cl > 0) ++bl_count[static_cast<size_t>(cl)];
        }

        // Compute the first code value for each bit length.
        std::vector<uint32_t> next_code(static_cast<size_t>(max_bits + 1), 0);
        uint32_t code = 0;
        for(int bits = 1; bits <= max_bits; ++bits){
            code = (code + static_cast<uint32_t>(bl_count[static_cast<size_t>(bits - 1)])) << 1U;
            next_code[static_cast<size_t>(bits)] = code;
        }

        // Build lookup table indexed by bit-reversed codes.  The table is
        // 2^max_bits entries; shorter codes are filled across all matching
        // suffixes so that a single read of max_bits bits suffices to look
        // up any symbol.
        table_size_ = 1U << max_bits;
        table_.assign(table_size_, -1);
        lengths_.assign(code_lengths.size(), 0);

        for(size_t sym = 0; sym < code_lengths.size(); ++sym){
            int len = code_lengths[sym];
            if(len == 0) continue;
            lengths_[sym] = len;

            uint32_t c = next_code[static_cast<size_t>(len)]++;
            // Reverse the bits of c (len bits) for LSB-first packing.
            uint32_t rev = 0;
            for(int b = 0; b < len; ++b){
                rev |= ((c >> b) & 1U) << (len - 1 - b);
            }
            // Fill all table entries that share this prefix.
            uint32_t fill = 1U << len;
            for(uint32_t entry = rev; entry < table_size_; entry += fill){
                table_[entry] = static_cast<int>(sym);
            }
        }
    }

    // Decode one Huffman symbol from the bitstream.  Reads max_bits_ at
    // once for a fast table lookup, then returns the excess bits that were
    // not part of the resolved code via put_back_bits.
    int decode(bit_reader &br) const {
        if(max_bits_ == 0) throw std::runtime_error("gzip: empty Huffman table");
        uint32_t bits = br.read_bits(max_bits_);
        int sym = table_[bits];
        if(sym < 0) throw std::runtime_error("gzip: invalid Huffman code");
        int actual_len = lengths_[static_cast<size_t>(sym)];
        br.put_back_bits(max_bits_ - actual_len);
        return sym;
    }

    int max_bits_ = 0;
    uint32_t table_size_ = 0;
    std::vector<int> table_;
    std::vector<int> lengths_;
};

// -----------------------------------------------------------------------
// DEFLATE fixed Huffman tables (RFC 1951, section 3.2.6).
// -----------------------------------------------------------------------
static void build_fixed_lit_len(huffman_decoder &dec){
    std::vector<int> lengths(288);
    for(int i = 0;   i <= 143; ++i) lengths[static_cast<size_t>(i)] = 8;
    for(int i = 144; i <= 255; ++i) lengths[static_cast<size_t>(i)] = 9;
    for(int i = 256; i <= 279; ++i) lengths[static_cast<size_t>(i)] = 7;
    for(int i = 280; i <= 287; ++i) lengths[static_cast<size_t>(i)] = 8;
    dec.build(lengths);
}

static void build_fixed_dist(huffman_decoder &dec){
    std::vector<int> lengths(32, 5);
    dec.build(lengths);
}

// -----------------------------------------------------------------------
// DEFLATE length and distance extra-bit tables (RFC 1951, section 3.2.5).
// -----------------------------------------------------------------------
struct len_entry { int base; int extra_bits; };

static constexpr std::array<len_entry, 29> length_table = {{
    {3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0},
    {11,1},{13,1},{15,1},{17,1},
    {19,2},{23,2},{27,2},{31,2},
    {35,3},{43,3},{51,3},{59,3},
    {67,4},{83,4},{99,4},{115,4},
    {131,5},{163,5},{195,5},{227,5},
    {258,0}
}};

struct dist_entry { int base; int extra_bits; };

static constexpr std::array<dist_entry, 30> distance_table = {{
    {1,0},{2,0},{3,0},{4,0},
    {5,1},{7,1},
    {9,2},{13,2},
    {17,3},{25,3},
    {33,4},{49,4},
    {65,5},{97,5},
    {129,6},{193,6},
    {257,7},{385,7},
    {513,8},{769,8},
    {1025,9},{1537,9},
    {2049,10},{3073,10},
    {4097,11},{6145,11},
    {8193,12},{12289,12},
    {16385,13},{24577,13}
}};

// -----------------------------------------------------------------------
// DEFLATE block-level decompression.
//
// Decompresses a single DEFLATE block from br, appending output to
// the window (which also serves as the back-reference sliding window).
// Returns true if this was the final block (BFINAL = 1).
// -----------------------------------------------------------------------
static bool decompress_one_block(bit_reader &br, std::vector<uint8_t> &window){
    bool bfinal = (br.read_bits(1) != 0);
    uint32_t btype = br.read_bits(2);

    if(btype == 0U){
        // Stored (uncompressed) block.
        br.align_to_byte();
        uint16_t block_len  = br.read_u16();
        uint16_t block_nlen = br.read_u16();
        if(static_cast<uint16_t>(block_len ^ 0xFFFFU) != block_nlen){
            throw std::runtime_error("gzip: stored block length mismatch");
        }
        for(uint16_t i = 0; i < block_len; ++i){
            window.push_back(br.read_byte());
        }

    }else if(btype == 1U || btype == 2U){
        // Compressed block: fixed or dynamic Huffman.
        huffman_decoder lit_len_dec;
        huffman_decoder dist_dec;

        if(btype == 1U){
            build_fixed_lit_len(lit_len_dec);
            build_fixed_dist(dist_dec);
        }else{
            // Dynamic Huffman tables.
            uint32_t hlit  = br.read_bits(5) + 257U;
            uint32_t hdist = br.read_bits(5) + 1U;
            uint32_t hclen = br.read_bits(4) + 4U;

            // Code length alphabet order (RFC 1951, section 3.2.7).
            static constexpr std::array<int, 19> cl_order = {{
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
            }};

            std::vector<int> cl_lengths(19, 0);
            for(uint32_t i = 0; i < hclen; ++i){
                cl_lengths[static_cast<size_t>(cl_order[i])] = static_cast<int>(br.read_bits(3));
            }

            huffman_decoder cl_dec;
            cl_dec.build(cl_lengths);

            // Decode literal/length and distance code lengths.
            const size_t total_codes = hlit + hdist;
            std::vector<int> all_lengths;
            all_lengths.reserve(total_codes);

            while(all_lengths.size() < total_codes){
                int sym = cl_dec.decode(br);
                if(sym <= 15){
                    all_lengths.push_back(sym);
                }else if(sym == 16){
                    if(all_lengths.empty()) throw std::runtime_error("gzip: repeat with no previous length");
                    int repeat = static_cast<int>(br.read_bits(2)) + 3;
                    int prev = all_lengths.back();
                    for(int r = 0; r < repeat; ++r) all_lengths.push_back(prev);
                }else if(sym == 17){
                    int repeat = static_cast<int>(br.read_bits(3)) + 3;
                    for(int r = 0; r < repeat; ++r) all_lengths.push_back(0);
                }else if(sym == 18){
                    int repeat = static_cast<int>(br.read_bits(7)) + 11;
                    for(int r = 0; r < repeat; ++r) all_lengths.push_back(0);
                }else{
                    throw std::runtime_error("gzip: invalid code-length symbol");
                }

                if(all_lengths.size() > total_codes){
                    throw std::runtime_error("gzip: code lengths exceed expected count");
                }
            }

            std::vector<int> lit_lengths(all_lengths.begin(),
                                         all_lengths.begin() + static_cast<ptrdiff_t>(hlit));
            std::vector<int> dist_lengths(all_lengths.begin() + static_cast<ptrdiff_t>(hlit),
                                          all_lengths.begin() + static_cast<ptrdiff_t>(hlit + hdist));
            lit_len_dec.build(lit_lengths);
            dist_dec.build(dist_lengths);
        }

        // Decode symbols.
        for(;;){
            int sym = lit_len_dec.decode(br);
            if(sym < 256){
                window.push_back(static_cast<uint8_t>(sym));
            }else if(sym == 256){
                break; // End of block.
            }else{
                // Length/distance pair.
                int len_idx = sym - 257;
                if(len_idx < 0 || len_idx >= static_cast<int>(length_table.size())){
                    throw std::runtime_error("gzip: invalid length code");
                }
                const auto &le = length_table[static_cast<size_t>(len_idx)];
                int match_len = le.base;
                if(le.extra_bits > 0){
                    match_len += static_cast<int>(br.read_bits(le.extra_bits));
                }

                int dist_sym = dist_dec.decode(br);
                if(dist_sym < 0 || dist_sym >= static_cast<int>(distance_table.size())){
                    throw std::runtime_error("gzip: invalid distance code");
                }
                const auto &de = distance_table[static_cast<size_t>(dist_sym)];
                int match_dist = de.base;
                if(de.extra_bits > 0){
                    match_dist += static_cast<int>(br.read_bits(de.extra_bits));
                }

                if(match_dist < 1 || static_cast<size_t>(match_dist) > window.size()){
                    throw std::runtime_error("gzip: invalid back-reference distance");
                }

                // Copy from window (byte-by-byte for overlapping copies).
                size_t src_pos = window.size() - static_cast<size_t>(match_dist);
                for(int j = 0; j < match_len; ++j){
                    window.push_back(window[src_pos + static_cast<size_t>(j)]);
                }
            }
        }
    }else{
        throw std::runtime_error("gzip: reserved block type");
    }
    return bfinal;
}

// -----------------------------------------------------------------------
// DEFLATE compression (fixed Huffman codes + LZ77).
// -----------------------------------------------------------------------

// Encode a literal/length symbol using fixed Huffman codes.
//
// Per RFC 1951 section 3.2.6, the fixed literal/length codes are:
//   0-143   -> 8-bit codes starting at 0b00110000  (0x030)
//   144-255 -> 9-bit codes starting at 0b110010000 (0x190)
//   256-279 -> 7-bit codes starting at 0b0000000   (0x000)
//   280-287 -> 8-bit codes starting at 0b11000000  (0x0C0)
//
// These canonical codes are MSB-first.  Per RFC 1951 section 3.1.1,
// "Huffman codes are packed starting with the most-significant bit of
// the code."  Since bit_writer emits the LSB of its value argument
// first, we bit-reverse the canonical code so that the code's MSB
// lands at the lowest bit position in the output byte stream.
static void emit_fixed_lit_len(bit_writer &bw, int sym){
    uint32_t code;
    int nbits;
    if(sym <= 143){
        code = static_cast<uint32_t>(0x30 + sym);
        nbits = 8;
    }else if(sym <= 255){
        code = static_cast<uint32_t>(0x190 + (sym - 144));
        nbits = 9;
    }else if(sym <= 279){
        code = static_cast<uint32_t>(0x00 + (sym - 256));
        nbits = 7;
    }else{
        code = static_cast<uint32_t>(0xC0 + (sym - 280));
        nbits = 8;
    }

    // Bit-reverse the canonical code for LSB-first emission.
    uint32_t rev = 0;
    for(int b = 0; b < nbits; ++b){
        rev |= ((code >> b) & 1U) << (nbits - 1 - b);
    }
    bw.write_bits(rev, nbits);
}

// Encode a fixed-Huffman distance code (5 bits, values 0-31).
// As with literal/length codes, the canonical code is bit-reversed
// before being passed to write_bits for correct MSB-first Huffman
// packing (see RFC 1951 section 3.1.1).
static void emit_fixed_dist(bit_writer &bw, int dist_code){
    uint32_t rev = 0;
    uint32_t code = static_cast<uint32_t>(dist_code);
    for(int b = 0; b < 5; ++b){
        rev |= ((code >> b) & 1U) << (4 - b);
    }
    bw.write_bits(rev, 5);
}

// Find the length code and extra bits for a given match length.
static void encode_length(bit_writer &bw, int match_len){
    for(size_t i = 0; i < length_table.size(); ++i){
        int base = length_table[i].base;
        int extra = length_table[i].extra_bits;
        int range = (1 << extra);
        if(match_len >= base && match_len < base + range){
            emit_fixed_lit_len(bw, static_cast<int>(i) + 257);
            if(extra > 0){
                bw.write_bits(static_cast<uint32_t>(match_len - base), extra);
            }
            return;
        }
    }
    // match_len == 258 is the last entry.
    emit_fixed_lit_len(bw, 285);
}

// Find the distance code and extra bits for a given match distance.
static void encode_distance(bit_writer &bw, int match_dist){
    for(size_t i = 0; i < distance_table.size(); ++i){
        int base = distance_table[i].base;
        int extra = distance_table[i].extra_bits;
        int range = (1 << extra);
        if(match_dist >= base && match_dist < base + range){
            emit_fixed_dist(bw, static_cast<int>(i));
            if(extra > 0){
                bw.write_bits(static_cast<uint32_t>(match_dist - base), extra);
            }
            return;
        }
    }
    throw std::runtime_error("gzip: distance out of range");
}

// Simple hash-chain LZ77 compressor.
static constexpr int WINDOW_SIZE = 32768;
static constexpr int MAX_MATCH = 258;
static constexpr int MIN_MATCH = 3;
static constexpr int HASH_BITS = 15;
static constexpr int HASH_SIZE = (1 << HASH_BITS);
static constexpr int MAX_CHAIN = 64; // Maximum chain search depth.

static uint32_t hash3(const uint8_t *p){
    return (static_cast<uint32_t>(p[0]) ^
           (static_cast<uint32_t>(p[1]) << 5U) ^
           (static_cast<uint32_t>(p[2]) << 10U)) & (HASH_SIZE - 1U);
}

// Write a single DEFLATE block into the provided bit_writer.
//
// The caller owns the bit_writer so that consecutive blocks share the
// same bitstream without spurious byte-alignment padding between them.
// Only the final block (bfinal == true) is flushed to a byte boundary.
static void deflate_write_block(bit_writer &bw, const uint8_t *data, size_t len, bool bfinal){
    bw.write_bits(bfinal ? 1U : 0U, 1); // BFINAL.

    if(len == 0){
        // Empty block: use stored (BTYPE = 00).
        bw.write_bits(0, 2); // BTYPE = 00 (stored).
        bw.flush_to_byte();
        // LEN = 0, NLEN = 0xFFFF.
        bw.write_bits(0x00, 8);
        bw.write_bits(0x00, 8);
        bw.write_bits(0xFF, 8);
        bw.write_bits(0xFF, 8);
        if(bfinal) bw.flush_to_byte();
        return;
    }

    bw.write_bits(1, 2); // BTYPE = 01 (fixed Huffman).

    // Hash table: head[hash] = most recent position with this hash.
    std::vector<int> head(HASH_SIZE, -1);
    // Chain: prev[pos % WINDOW_SIZE] = previous position with same hash.
    std::vector<int> prev(WINDOW_SIZE, -1);

    size_t pos = 0;
    while(pos < len){
        int best_len = 0;
        int best_dist = 0;

        if(pos + MIN_MATCH <= len){
            uint32_t h = hash3(data + pos);
            int chain_pos = head[h];
            int chain_count = 0;

            while(chain_pos >= 0 && chain_count < MAX_CHAIN){
                int dist = static_cast<int>(pos) - chain_pos;
                if(dist > WINDOW_SIZE || dist <= 0) break;

                // Compute match length.
                int max_possible = std::min(MAX_MATCH, static_cast<int>(len - pos));
                int mlen = 0;
                while(mlen < max_possible && data[static_cast<size_t>(chain_pos) + static_cast<size_t>(mlen)] == data[pos + static_cast<size_t>(mlen)]){
                    ++mlen;
                }
                if(mlen > best_len){
                    best_len = mlen;
                    best_dist = dist;
                    if(best_len == MAX_MATCH) break;
                }

                chain_pos = prev[static_cast<size_t>(chain_pos % WINDOW_SIZE)];
                ++chain_count;
            }

            // Update hash chain.
            prev[pos % WINDOW_SIZE] = head[h];
            head[h] = static_cast<int>(pos);
        }

        if(best_len >= MIN_MATCH){
            encode_length(bw, best_len);
            encode_distance(bw, best_dist);

            // Insert hash entries for the matched bytes (skip first, already inserted).
            for(int i = 1; i < best_len && (pos + static_cast<size_t>(i) + MIN_MATCH) <= len; ++i){
                uint32_t h2 = hash3(data + pos + static_cast<size_t>(i));
                prev[(pos + static_cast<size_t>(i)) % WINDOW_SIZE] = head[h2];
                head[h2] = static_cast<int>(pos) + i;
            }
            pos += static_cast<size_t>(best_len);
        }else{
            emit_fixed_lit_len(bw, static_cast<int>(data[pos]));
            ++pos;
        }
    }

    // End of block marker.
    emit_fixed_lit_len(bw, 256);
    if(bfinal) bw.flush_to_byte();
}

// -----------------------------------------------------------------------
// Gzip framing helpers (RFC 1952).
// -----------------------------------------------------------------------
static void write_gzip_header_to(std::ostream &os){
    static constexpr uint8_t header[] = {
        0x1F, 0x8B, // ID1, ID2.
        0x08,       // CM = deflate.
        0x00,       // FLG = none.
        0x00, 0x00, 0x00, 0x00, // MTIME = 0.
        0x00,       // XFL.
        0xFF        // OS = unknown.
    };
    os.write(reinterpret_cast<const char *>(header), sizeof(header));
}

static void write_u32_le_to(std::ostream &os, uint32_t val){
    uint8_t bytes[4] = {
        static_cast<uint8_t>( val        & 0xFFU),
        static_cast<uint8_t>((val >>  8U) & 0xFFU),
        static_cast<uint8_t>((val >> 16U) & 0xFFU),
        static_cast<uint8_t>((val >> 24U) & 0xFFU)
    };
    os.write(reinterpret_cast<const char *>(bytes), 4);
}

// -----------------------------------------------------------------------
// compress_streambuf: streaming gzip compressor.
//
// The gzip header is written to the sink immediately upon construction.
// Data written to this streambuf is accumulated into an internal buffer.
// When the buffer reaches BLOCK_SIZE, a non-final DEFLATE block is
// emitted.  On sync(), any pending data is flushed as a non-final block.
// On finalize() (called from the destructor or explicitly), the
// remaining data is emitted as the final block and the gzip trailer
// (CRC32 + ISIZE) is written.
//
// A single persistent bit_writer is used across all DEFLATE blocks so
// that consecutive blocks are correctly bit-packed without spurious
// inter-block byte-alignment padding.
// -----------------------------------------------------------------------
class compress_streambuf : public std::streambuf {
  public:
    explicit compress_streambuf(std::ostream &sink)
        : sink_(sink), bw_(deflate_out_)
    {
        setp(buf_.data(), buf_.data() + buf_.size());
        // Write the gzip header immediately.
        write_gzip_header_to(sink_);
    }

    ~compress_streambuf() override {
        try {
            finalize();
        }catch(...){
            // Suppress exceptions in destructor.
        }
    }

    void finalize(){
        if(finalized_) return;
        finalized_ = true;

        // Flush the streambuf put-area into pending_.
        drain_put_area();

        if(!pending_.empty()){
            emit_block(true);
        }else{
            // No remaining data; emit an empty final block.
            deflate_write_block(bw_, nullptr, 0, true);
        }

        // Flush any remaining compressed bytes to the sink.
        flush_deflate_output();

        // Write gzip trailer.
        write_u32_le_to(sink_, crc_);
        write_u32_le_to(sink_, isize_);
        sink_.flush();
    }

  protected:
    int_type overflow(int_type ch) override {
        drain_put_area();
        if(!traits_type::eq_int_type(ch, traits_type::eof())){
            pending_.push_back(static_cast<uint8_t>(ch));
        }
        if(pending_.size() >= BLOCK_SIZE){
            emit_block(false);
        }
        return ch;
    }

    int sync() override {
        drain_put_area();
        if(!pending_.empty()){
            emit_block(false);
        }
        flush_deflate_output();
        sink_.flush();
        return 0;
    }

  private:
    static constexpr size_t BLOCK_SIZE = 32768;

    void drain_put_area(){
        auto n = pptr() - pbase();
        if(n > 0){
            pending_.insert(pending_.end(),
                            reinterpret_cast<const uint8_t *>(pbase()),
                            reinterpret_cast<const uint8_t *>(pbase()) + n);
            setp(buf_.data(), buf_.data() + buf_.size());
        }
    }

    void emit_block(bool is_final){
        // Update CRC and total size over the uncompressed pending data.
        crc_ = crc32_update(crc_, pending_.data(), pending_.size());
        isize_ += static_cast<uint32_t>(pending_.size());

        deflate_write_block(bw_, pending_.data(), pending_.size(), is_final);
        pending_.clear();

        // Write completed bytes to the sink.
        flush_deflate_output();
    }

    // Write all completed bytes from deflate_out_ to the sink.
    // The bit_writer's partial byte (if any) stays in its accumulator
    // and is not in deflate_out_, so this is always safe.
    void flush_deflate_output(){
        if(!deflate_out_.empty()){
            sink_.write(reinterpret_cast<const char *>(deflate_out_.data()),
                        static_cast<std::streamsize>(deflate_out_.size()));
            deflate_out_.clear();
        }
    }

    std::ostream &sink_;
    std::array<char, 8192> buf_{};
    std::vector<uint8_t> pending_;
    std::vector<uint8_t> deflate_out_;
    bit_writer bw_;
    uint32_t crc_ = 0;
    uint32_t isize_ = 0;
    bool finalized_ = false;
};

// -----------------------------------------------------------------------
// decompress_streambuf: streaming gzip decompressor.
//
// The compressed data is read from the source stream and buffered.
// The gzip header is parsed on construction.  DEFLATE blocks are
// decompressed one at a time in underflow(), producing output into
// out_buf_ which is served as the read area.  A sliding window
// (last 32 KB) is maintained for back-references across blocks.
// The gzip trailer (CRC32, ISIZE) is verified after the final block.
// -----------------------------------------------------------------------
class decompress_streambuf : public std::streambuf {
  public:
    explicit decompress_streambuf(std::istream &source){
        // Read all compressed data from the source.
        compressed_.assign(std::istreambuf_iterator<char>(source),
                           std::istreambuf_iterator<char>());
        parse_gzip_header();
        br_ = bit_reader(compressed_.data() + deflate_start_,
                         compressed_.size() - deflate_start_);
    }

    ~decompress_streambuf() override = default;

  protected:
    int_type underflow() override {
        if(gptr() < egptr()){
            return traits_type::to_int_type(*gptr());
        }
        // Decompress the next block(s) until we have output or reach EOF.
        while(!deflate_done_){
            size_t window_start = window_.size();
            bool is_last = decompress_one_block(br_, window_);

            // The new output is the portion appended to window_.
            size_t new_bytes = window_.size() - window_start;

            // Update CRC and ISIZE over the new decompressed data.
            if(new_bytes > 0){
                crc_ = crc32_update(crc_, window_.data() + window_start, new_bytes);
                isize_ += static_cast<uint32_t>(new_bytes);
            }

            if(is_last){
                deflate_done_ = true;
                verify_trailer();
            }

            if(new_bytes > 0){
                // Copy new output into out_buf_ for the get area.
                out_buf_.assign(window_.begin() + static_cast<ptrdiff_t>(window_start),
                                window_.end());

                // Trim window to the last WINDOW_SIZE bytes for future
                // back-references.
                if(window_.size() > WINDOW_SIZE){
                    window_.erase(window_.begin(),
                                  window_.end() - WINDOW_SIZE);
                }

                char *base = reinterpret_cast<char *>(out_buf_.data());
                setg(base, base, base + out_buf_.size());
                return traits_type::to_int_type(*gptr());
            }
        }
        return traits_type::eof();
    }

  private:
    static constexpr size_t WINDOW_SIZE = 32768;

    void parse_gzip_header(){
        if(compressed_.size() < 10){
            throw std::runtime_error("gzip: input too short to be a valid gzip file");
        }
        if(compressed_[0] != 0x1FU || compressed_[1] != 0x8BU){
            throw std::runtime_error("gzip: invalid magic number");
        }
        if(compressed_[2] != 0x08U){
            throw std::runtime_error("gzip: unsupported compression method (only deflate is supported)");
        }

        uint8_t flg = compressed_[3];
        size_t pos = 10;

        if(flg & 0x04U){ // FEXTRA.
            if(pos + 2 > compressed_.size()) throw std::runtime_error("gzip: truncated FEXTRA");
            uint16_t xlen = static_cast<uint16_t>(compressed_[pos]) |
                            static_cast<uint16_t>(static_cast<uint16_t>(compressed_[pos + 1]) << 8U);
            pos += 2U + xlen;
        }
        if(flg & 0x08U){ // FNAME.
            while(pos < compressed_.size() && compressed_[pos] != 0) ++pos;
            ++pos;
        }
        if(flg & 0x10U){ // FCOMMENT.
            while(pos < compressed_.size() && compressed_[pos] != 0) ++pos;
            ++pos;
        }
        if(flg & 0x02U){ // FHCRC.
            pos += 2;
        }

        if(pos >= compressed_.size()){
            throw std::runtime_error("gzip: truncated gzip data");
        }
        deflate_start_ = pos;
    }

    void verify_trailer(){
        // The trailer immediately follows the DEFLATE stream.
        uint32_t expected_crc = br_.read_u32_le();
        uint32_t expected_isize = br_.read_u32_le();
        if(crc_ != expected_crc){
            throw std::runtime_error("gzip: CRC32 mismatch");
        }
        if(isize_ != expected_isize){
            throw std::runtime_error("gzip: size mismatch");
        }
    }

    std::vector<uint8_t> compressed_;
    size_t deflate_start_ = 0;
    bit_reader br_;
    std::vector<uint8_t> window_;
    std::vector<uint8_t> out_buf_;
    uint32_t crc_ = 0;
    uint32_t isize_ = 0;
    bool deflate_done_ = false;
};

} // namespace gzip_impl.


// -----------------------------------------------------------------------
// gzip_ostream implementation.
// -----------------------------------------------------------------------
gzip_ostream::gzip_ostream(std::ostream &sink)
    : std::ostream(nullptr),
      buf_(std::make_unique<gzip_impl::compress_streambuf>(sink))
{
    rdbuf(buf_.get());
}

gzip_ostream::~gzip_ostream(){
    // Ensure the compressor is finalized before destruction, but never throw
    // from a destructor.  Signal any error via the stream state instead.
    if(buf_){
        try{
            buf_->finalize();
        }catch(...){
            setstate(std::ios::badbit);
        }
    }
}

// -----------------------------------------------------------------------
// gzip_istream implementation.
// -----------------------------------------------------------------------
gzip_istream::gzip_istream(std::istream &source)
    : std::istream(nullptr),
      buf_(std::make_unique<gzip_impl::decompress_streambuf>(source))
{
    rdbuf(buf_.get());
}

gzip_istream::~gzip_istream() = default;

} // namespace io.
} // namespace ygor.
