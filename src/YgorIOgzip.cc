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

    bool at_end() const {
        return (byte_pos_ >= len_) && (bit_pos_ == 0);
    }

    size_t byte_position() const {
        return byte_pos_ + (bit_pos_ > 0 ? 1 : 0);
    }

    size_t exact_byte_position() const {
        return byte_pos_;
    }

    int current_bit_position() const {
        return bit_pos_;
    }

  private:
    const uint8_t *data_;
    size_t len_;
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

        // Build lookup table: for each possible code (up to max_bits), store the symbol.
        // Use a flat table for fast lookup (works well for typical DEFLATE code lengths <= 15 bits).
        table_size_ = 1U << max_bits;
        table_.assign(table_size_, -1);
        lengths_.resize(code_lengths.size());

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

    int decode(bit_reader &br) const {
        if(max_bits_ == 0) throw std::runtime_error("gzip: empty Huffman table");
        uint32_t code = br.read_bits(max_bits_);
        int sym = table_[code];
        if(sym < 0) throw std::runtime_error("gzip: invalid Huffman code");
        // We consumed max_bits_, but the actual code may be shorter.
        // We need to "put back" the excess bits.
        // Since bit_reader doesn't support putback, we instead use a different approach:
        // consume only the bits needed.
        // Rebuild: consume bit-by-bit.
        // Actually, the table-lookup approach already works if we account for the extra bits.
        // The key insight: our table maps reversed-prefix to symbol, and for codes shorter
        // than max_bits, we filled all entries. So the sym found from the first max_bits is correct.
        // But we over-consumed bits. We need to "return" the excess bits.
        return sym;
    }

    // Decode using bit-by-bit approach (slower but simpler, avoids over-consumption).
    int decode_slow(bit_reader &br) const {
        uint32_t code = 0;
        for(int len = 1; len <= max_bits_; ++len){
            code = (code << 1U) | br.read_bits(1);
            // Reverse code to check against table.
            uint32_t rev = 0;
            for(int b = 0; b < len; ++b){
                rev |= ((code >> b) & 1U) << (len - 1 - b);
            }
            if(rev < table_size_){
                int sym = table_[rev];
                if(sym >= 0 && lengths_[static_cast<size_t>(sym)] == len){
                    return sym;
                }
            }
        }
        throw std::runtime_error("gzip: could not decode Huffman symbol");
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
// DEFLATE decompression.
// -----------------------------------------------------------------------
static std::vector<uint8_t> deflate_decompress(const uint8_t *data, size_t len){
    std::vector<uint8_t> output;
    output.reserve(len * 4U); // Heuristic pre-allocation.
    bit_reader br(data, len);

    bool bfinal = false;
    while(!bfinal){
        bfinal = (br.read_bits(1) != 0);
        uint32_t btype = br.read_bits(2);

        if(btype == 0U){
            // Stored block.
            br.align_to_byte();
            uint16_t block_len  = br.read_u16();
            uint16_t block_nlen = br.read_u16();
            if(static_cast<uint16_t>(block_len ^ 0xFFFFU) != block_nlen){
                throw std::runtime_error("gzip: stored block length mismatch");
            }
            for(uint16_t i = 0; i < block_len; ++i){
                output.push_back(br.read_byte());
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
                std::vector<int> all_lengths;
                all_lengths.reserve(hlit + hdist);

                while(all_lengths.size() < hlit + hdist){
                    int sym = cl_dec.decode_slow(br);
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
                int sym = lit_len_dec.decode_slow(br);
                if(sym < 256){
                    output.push_back(static_cast<uint8_t>(sym));
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

                    int dist_sym = dist_dec.decode_slow(br);
                    if(dist_sym < 0 || dist_sym >= static_cast<int>(distance_table.size())){
                        throw std::runtime_error("gzip: invalid distance code");
                    }
                    const auto &de = distance_table[static_cast<size_t>(dist_sym)];
                    int match_dist = de.base;
                    if(de.extra_bits > 0){
                        match_dist += static_cast<int>(br.read_bits(de.extra_bits));
                    }

                    if(match_dist < 1 || static_cast<size_t>(match_dist) > output.size()){
                        throw std::runtime_error("gzip: invalid back-reference distance");
                    }

                    // Copy from output buffer (byte-by-byte for overlapping copies).
                    size_t src_pos = output.size() - static_cast<size_t>(match_dist);
                    for(int j = 0; j < match_len; ++j){
                        output.push_back(output[src_pos + static_cast<size_t>(j)]);
                    }
                }
            }
        }else{
            throw std::runtime_error("gzip: reserved block type");
        }
    }
    return output;
}

// -----------------------------------------------------------------------
// DEFLATE compression (fixed Huffman codes + LZ77).
// -----------------------------------------------------------------------

// Encode a literal/length symbol using fixed Huffman codes (RFC 1951 section 3.2.6).
// Codes are written in reverse bit order (LSB first) per the DEFLATE spec.
static void emit_fixed_lit_len(bit_writer &bw, int sym){
    // Fixed Huffman table for literal/length:
    //   0-143   -> 8-bit codes, starting at 0x30  (00110000 - 10111111)
    //   144-255 -> 9-bit codes, starting at 0x190 (110010000 - 111111111)
    //   256-279 -> 7-bit codes, starting at 0x00  (0000000 - 0010111)
    //   280-287 -> 8-bit codes, starting at 0xC0  (11000000 - 11000111)
    //
    // But we write bits LSB first, so we need to reverse the code bits.
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

    // Reverse the bits for LSB-first output.
    uint32_t rev = 0;
    for(int b = 0; b < nbits; ++b){
        rev |= ((code >> b) & 1U) << (nbits - 1 - b);
    }
    bw.write_bits(rev, nbits);
}

static void emit_fixed_dist(bit_writer &bw, int dist_code){
    // All 32 distance codes are 5 bits, values 0-31.
    // Reverse 5 bits for LSB-first output.
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

static std::vector<uint8_t> deflate_compress(const uint8_t *data, size_t len){
    std::vector<uint8_t> output;
    output.reserve(len + len / 8U + 64U);
    bit_writer bw(output);

    if(len == 0){
        // Empty input: write a final stored block with zero length.
        bw.write_bits(1, 1); // BFINAL = 1.
        bw.write_bits(0, 2); // BTYPE = 00 (stored).
        bw.flush_to_byte();
        // LEN = 0, NLEN = 0xFFFF.
        bw.write_bits(0x00, 8);
        bw.write_bits(0x00, 8);
        bw.write_bits(0xFF, 8);
        bw.write_bits(0xFF, 8);
        bw.flush_to_byte();
        return output;
    }

    // Use a single DEFLATE block with fixed Huffman codes.
    bw.write_bits(1, 1); // BFINAL = 1.
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
    bw.flush_to_byte();
    return output;
}

// -----------------------------------------------------------------------
// Gzip framing (RFC 1952).
// -----------------------------------------------------------------------
static void write_gzip_header(std::vector<uint8_t> &out){
    out.push_back(0x1F); // ID1.
    out.push_back(0x8B); // ID2.
    out.push_back(0x08); // CM = deflate.
    out.push_back(0x00); // FLG = none.
    // MTIME (4 bytes, 0 = not available).
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x00); // XFL.
    out.push_back(0xFF); // OS = unknown.
}

static void write_u32_le(std::vector<uint8_t> &out, uint32_t val){
    out.push_back(static_cast<uint8_t>( val        & 0xFFU));
    out.push_back(static_cast<uint8_t>((val >>  8U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((val >> 16U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((val >> 24U) & 0xFFU));
}

static uint32_t read_u32_le(const uint8_t *p){
    return  static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) <<  8U)
         | (static_cast<uint32_t>(p[2]) << 16U)
         | (static_cast<uint32_t>(p[3]) << 24U);
}

// -----------------------------------------------------------------------
// compress_streambuf: compresses data written to it.
// -----------------------------------------------------------------------
class compress_streambuf : public std::streambuf {
  public:
    explicit compress_streambuf(std::ostream &sink)
        : sink_(sink)
    {
        // Use a put-back area of size 0 and a buffer for output.
        setp(buf_.data(), buf_.data() + buf_.size());
    }

    ~compress_streambuf() override {
        try {
            finalize();
        }catch(...){
            // Suppress exceptions in destructor.
        }
    }

    // Finalize the gzip stream (write header + compressed data + trailer).
    void finalize(){
        if(finalized_) return;
        finalized_ = true;

        // Flush any remaining buffered data.
        flush_buffer();

        // Now compress accumulated_ and write gzip output.
        std::vector<uint8_t> gzip_out;
        write_gzip_header(gzip_out);

        auto deflated = deflate_compress(accumulated_.data(), accumulated_.size());
        gzip_out.insert(gzip_out.end(), deflated.begin(), deflated.end());

        // CRC32 and ISIZE trailer.
        uint32_t crc = crc32_update(0, accumulated_.data(), accumulated_.size());
        write_u32_le(gzip_out, crc);
        uint32_t isize = static_cast<uint32_t>(accumulated_.size() & 0xFFFFFFFFU);
        write_u32_le(gzip_out, isize);

        sink_.write(reinterpret_cast<const char *>(gzip_out.data()),
                    static_cast<std::streamsize>(gzip_out.size()));
        sink_.flush();
    }

  protected:
    int_type overflow(int_type ch) override {
        flush_buffer();
        if(!traits_type::eq_int_type(ch, traits_type::eof())){
            accumulated_.push_back(static_cast<uint8_t>(ch));
        }
        return ch;
    }

    int sync() override {
        flush_buffer();
        return 0;
    }

  private:
    void flush_buffer(){
        auto n = pptr() - pbase();
        if(n > 0){
            accumulated_.insert(accumulated_.end(),
                                reinterpret_cast<const uint8_t *>(pbase()),
                                reinterpret_cast<const uint8_t *>(pbase()) + n);
            setp(buf_.data(), buf_.data() + buf_.size());
        }
    }

    std::ostream &sink_;
    std::array<char, 8192> buf_{};
    std::vector<uint8_t> accumulated_;
    bool finalized_ = false;
};

// -----------------------------------------------------------------------
// decompress_streambuf: decompresses gzip data read from source stream.
// -----------------------------------------------------------------------
class decompress_streambuf : public std::streambuf {
  public:
    explicit decompress_streambuf(std::istream &source)
        : source_(source)
    {
        // Read and decompress all data eagerly.
        decompress_all();
        // Set up the get area.
        if(!decompressed_.empty()){
            char *base = reinterpret_cast<char *>(decompressed_.data());
            setg(base, base, base + decompressed_.size());
        }
    }

    ~decompress_streambuf() override = default;

  protected:
    int_type underflow() override {
        if(gptr() == egptr()){
            return traits_type::eof();
        }
        return traits_type::to_int_type(*gptr());
    }

  private:
    void decompress_all(){
        // Read entire compressed input.
        std::vector<uint8_t> compressed(
            (std::istreambuf_iterator<char>(source_)),
            std::istreambuf_iterator<char>());

        if(compressed.size() < 18){
            throw std::runtime_error("gzip: input too short to be a valid gzip file");
        }

        size_t pos = 0;

        // Verify gzip magic number.
        if(compressed[0] != 0x1FU || compressed[1] != 0x8BU){
            throw std::runtime_error("gzip: invalid magic number");
        }
        if(compressed[2] != 0x08U){
            throw std::runtime_error("gzip: unsupported compression method (only deflate is supported)");
        }

        uint8_t flg = compressed[3];
        pos = 10; // Skip past the 10-byte header.

        // Handle optional header fields.
        if(flg & 0x04U){ // FEXTRA.
            if(pos + 2 > compressed.size()) throw std::runtime_error("gzip: truncated FEXTRA");
            uint16_t xlen = static_cast<uint16_t>(compressed[pos]) |
                            static_cast<uint16_t>(static_cast<uint16_t>(compressed[pos + 1]) << 8U);
            pos += 2U + xlen;
        }
        if(flg & 0x08U){ // FNAME.
            while(pos < compressed.size() && compressed[pos] != 0) ++pos;
            ++pos; // Skip null terminator.
        }
        if(flg & 0x10U){ // FCOMMENT.
            while(pos < compressed.size() && compressed[pos] != 0) ++pos;
            ++pos;
        }
        if(flg & 0x02U){ // FHCRC.
            pos += 2;
        }

        if(pos >= compressed.size() || compressed.size() - pos < 8){
            throw std::runtime_error("gzip: truncated gzip data");
        }

        // The last 8 bytes are CRC32 and ISIZE.
        size_t trailer_pos = compressed.size() - 8;
        uint32_t expected_crc = read_u32_le(compressed.data() + trailer_pos);
        uint32_t expected_isize = read_u32_le(compressed.data() + trailer_pos + 4);

        // Decompress the DEFLATE data.
        decompressed_ = deflate_decompress(compressed.data() + pos, trailer_pos - pos);

        // Verify CRC32.
        uint32_t actual_crc = crc32_update(0, decompressed_.data(), decompressed_.size());
        if(actual_crc != expected_crc){
            throw std::runtime_error("gzip: CRC32 mismatch");
        }

        // Verify ISIZE (lower 32 bits of original size).
        uint32_t actual_isize = static_cast<uint32_t>(decompressed_.size() & 0xFFFFFFFFU);
        if(actual_isize != expected_isize){
            throw std::runtime_error("gzip: size mismatch");
        }
    }

    std::istream &source_;
    std::vector<uint8_t> decompressed_;
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
    // Ensure the compressor is finalized before destruction.
    buf_->finalize();
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
