//MD5.cc - An implementation of the RFC 1321 MD5 algorithm. 
//
//This code was re-written and tidied by hal clark in 2013. The code was
// originally written by Alexander Peslyak (aka "Solar Designer") in 2001.
// He kindly released his code into the public domain. His website is given as 
// http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
// The original implementation, including his copyright notice, should be 
// included with this source. 
//
//The original implementation goals were to allow for integers which might
// be >= 32bits, and to pass to the OpenSSL implementation, if one exists.
// I've removed the latter because I had no intention to use it.

#include <string.h>
#include "md5.h"

//"F and G are optimized compared to their RFC 1321 definitions for
// architectures that lack an AND-NOT instruction" -- Alexander.
#define MD5F(x, y, z)   ((z) ^ ((x) & ((y) ^ (z))))
#define MD5G(x, y, z)   ((y) ^ ((z) & ((x) ^ (y))))
#define MD5H(x, y, z)   ((x) ^ (y) ^ (z))
#define MD5I(x, y, z)   ((y) ^ ((x) | ~(z)))


//"The MD5 transformation for all four rounds." -- Alexander.
#define MD5STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);


//"MD5SET reads 4 input bytes in little-endian byte order and stores them in a 
// properly aligned word in host byte order. The check for little-endian 
// architectures that tolerate unaligned memory accesses is just an 
// optimization. Nothing will break if it doesn't work." -- Alexander.
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
    #define MD5SET(n)  (*(MD5::YGOR_MD5_u_t *)&ptr[(n) * 4])
    #define MD5GET(n)  MD5SET(n)
#else
    #define MD5SET(n)  (ctx->block[(n)] = \
       (MD5::YGOR_MD5_u_t)ptr[(n) * 4] | \
       ((MD5::YGOR_MD5_u_t)ptr[(n) * 4 + 1] << 8) | \
       ((MD5::YGOR_MD5_u_t)ptr[(n) * 4 + 2] << 16) | \
       ((MD5::YGOR_MD5_u_t)ptr[(n) * 4 + 3] << 24))
    #define MD5GET(n)  (ctx->block[(n)])
#endif

//"This processes one or more 64-byte data blocks, but does NOT update the bit 
// counters. There are no alignment requirements." -- Alexander.
static void *body(MD5::Context *ctx, void *data, unsigned long size){
    MD5::YGOR_MD5_u_t a, b, c, d, saved_a, saved_b, saved_c, saved_d;
    unsigned char *ptr = reinterpret_cast<unsigned char *>(data);
    a = ctx->a;  b = ctx->b;
    c = ctx->c;  d = ctx->d;

    //Walk over the data 64 bytes at a time.
    do{
        saved_a = a;  saved_b = b;
        saved_c = c;  saved_d = d;

        MD5STEP(MD5F, a, b, c, d, MD5SET(0), 0xd76aa478, 7)
        MD5STEP(MD5F, d, a, b, c, MD5SET(1), 0xe8c7b756, 12)
        MD5STEP(MD5F, c, d, a, b, MD5SET(2), 0x242070db, 17)
        MD5STEP(MD5F, b, c, d, a, MD5SET(3), 0xc1bdceee, 22)
        MD5STEP(MD5F, a, b, c, d, MD5SET(4), 0xf57c0faf, 7)
        MD5STEP(MD5F, d, a, b, c, MD5SET(5), 0x4787c62a, 12)
        MD5STEP(MD5F, c, d, a, b, MD5SET(6), 0xa8304613, 17)
        MD5STEP(MD5F, b, c, d, a, MD5SET(7), 0xfd469501, 22)
        MD5STEP(MD5F, a, b, c, d, MD5SET(8), 0x698098d8, 7)
        MD5STEP(MD5F, d, a, b, c, MD5SET(9), 0x8b44f7af, 12)
        MD5STEP(MD5F, c, d, a, b, MD5SET(10), 0xffff5bb1, 17)
        MD5STEP(MD5F, b, c, d, a, MD5SET(11), 0x895cd7be, 22)
        MD5STEP(MD5F, a, b, c, d, MD5SET(12), 0x6b901122, 7)
        MD5STEP(MD5F, d, a, b, c, MD5SET(13), 0xfd987193, 12)
        MD5STEP(MD5F, c, d, a, b, MD5SET(14), 0xa679438e, 17)
        MD5STEP(MD5F, b, c, d, a, MD5SET(15), 0x49b40821, 22)

        MD5STEP(MD5G, a, b, c, d, MD5GET(1), 0xf61e2562, 5)
        MD5STEP(MD5G, d, a, b, c, MD5GET(6), 0xc040b340, 9)
        MD5STEP(MD5G, c, d, a, b, MD5GET(11), 0x265e5a51, 14)
        MD5STEP(MD5G, b, c, d, a, MD5GET(0), 0xe9b6c7aa, 20)
        MD5STEP(MD5G, a, b, c, d, MD5GET(5), 0xd62f105d, 5)
        MD5STEP(MD5G, d, a, b, c, MD5GET(10), 0x02441453, 9)
        MD5STEP(MD5G, c, d, a, b, MD5GET(15), 0xd8a1e681, 14)
        MD5STEP(MD5G, b, c, d, a, MD5GET(4), 0xe7d3fbc8, 20)
        MD5STEP(MD5G, a, b, c, d, MD5GET(9), 0x21e1cde6, 5)
        MD5STEP(MD5G, d, a, b, c, MD5GET(14), 0xc33707d6, 9)
        MD5STEP(MD5G, c, d, a, b, MD5GET(3), 0xf4d50d87, 14)
        MD5STEP(MD5G, b, c, d, a, MD5GET(8), 0x455a14ed, 20)
        MD5STEP(MD5G, a, b, c, d, MD5GET(13), 0xa9e3e905, 5)
        MD5STEP(MD5G, d, a, b, c, MD5GET(2), 0xfcefa3f8, 9)
        MD5STEP(MD5G, c, d, a, b, MD5GET(7), 0x676f02d9, 14)
        MD5STEP(MD5G, b, c, d, a, MD5GET(12), 0x8d2a4c8a, 20)

        MD5STEP(MD5H, a, b, c, d, MD5GET(5), 0xfffa3942, 4)
        MD5STEP(MD5H, d, a, b, c, MD5GET(8), 0x8771f681, 11)
        MD5STEP(MD5H, c, d, a, b, MD5GET(11), 0x6d9d6122, 16)
        MD5STEP(MD5H, b, c, d, a, MD5GET(14), 0xfde5380c, 23)
        MD5STEP(MD5H, a, b, c, d, MD5GET(1), 0xa4beea44, 4)
        MD5STEP(MD5H, d, a, b, c, MD5GET(4), 0x4bdecfa9, 11)
        MD5STEP(MD5H, c, d, a, b, MD5GET(7), 0xf6bb4b60, 16)
        MD5STEP(MD5H, b, c, d, a, MD5GET(10), 0xbebfbc70, 23)
        MD5STEP(MD5H, a, b, c, d, MD5GET(13), 0x289b7ec6, 4)
        MD5STEP(MD5H, d, a, b, c, MD5GET(0), 0xeaa127fa, 11)
        MD5STEP(MD5H, c, d, a, b, MD5GET(3), 0xd4ef3085, 16)
        MD5STEP(MD5H, b, c, d, a, MD5GET(6), 0x04881d05, 23)
        MD5STEP(MD5H, a, b, c, d, MD5GET(9), 0xd9d4d039, 4)
        MD5STEP(MD5H, d, a, b, c, MD5GET(12), 0xe6db99e5, 11)
        MD5STEP(MD5H, c, d, a, b, MD5GET(15), 0x1fa27cf8, 16)
        MD5STEP(MD5H, b, c, d, a, MD5GET(2), 0xc4ac5665, 23)

        MD5STEP(MD5I, a, b, c, d, MD5GET(0), 0xf4292244, 6)
        MD5STEP(MD5I, d, a, b, c, MD5GET(7), 0x432aff97, 10)
        MD5STEP(MD5I, c, d, a, b, MD5GET(14), 0xab9423a7, 15)
        MD5STEP(MD5I, b, c, d, a, MD5GET(5), 0xfc93a039, 21)
        MD5STEP(MD5I, a, b, c, d, MD5GET(12), 0x655b59c3, 6)
        MD5STEP(MD5I, d, a, b, c, MD5GET(3), 0x8f0ccc92, 10)
        MD5STEP(MD5I, c, d, a, b, MD5GET(10), 0xffeff47d, 15)
        MD5STEP(MD5I, b, c, d, a, MD5GET(1), 0x85845dd1, 21)
        MD5STEP(MD5I, a, b, c, d, MD5GET(8), 0x6fa87e4f, 6)
        MD5STEP(MD5I, d, a, b, c, MD5GET(15), 0xfe2ce6e0, 10)
        MD5STEP(MD5I, c, d, a, b, MD5GET(6), 0xa3014314, 15)
        MD5STEP(MD5I, b, c, d, a, MD5GET(13), 0x4e0811a1, 21)
        MD5STEP(MD5I, a, b, c, d, MD5GET(4), 0xf7537e82, 6)
        MD5STEP(MD5I, d, a, b, c, MD5GET(11), 0xbd3af235, 10)
        MD5STEP(MD5I, c, d, a, b, MD5GET(2), 0x2ad7d2bb, 15)
        MD5STEP(MD5I, b, c, d, a, MD5GET(9), 0xeb86d391, 21)

        a += saved_a;  b += saved_b;
        c += saved_c;  d += saved_d;
        ptr += 64;
    }while(size -= 64);

    ctx->a = a;  ctx->b = b;
    ctx->c = c;  ctx->d = d;
    return ptr;
}

void MD5::Init(MD5::Context *ctx){
    ctx->a = 0x67452301;  ctx->b = 0xefcdab89;
    ctx->c = 0x98badcfe;  ctx->d = 0x10325476;
    ctx->lo = 0;          ctx->hi = 0;
    return;
}

void MD5::Update(MD5::Context *ctx, void *data, unsigned long size){
    MD5::YGOR_MD5_u_t saved_lo;
    unsigned long used, free;
    saved_lo = ctx->lo;
    if((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo) ctx->hi++;
    ctx->hi += size >> 29;
    used = saved_lo & 0x3f;
    if(used){
        free = 64 - used;
        if(size < free){
            memcpy(&ctx->buffer[used], data, size);
            return;
        }
        memcpy(&ctx->buffer[used], data, free);
        data = (unsigned char *)data + free;
        size -= free;
        body(ctx, ctx->buffer, 64);
    }
    if(size >= 64){
        data = body(ctx, data, size & ~(unsigned long)0x3f);
        size &= 0x3f;
    }
    memcpy(ctx->buffer, data, size);
}

void MD5::Final(unsigned char *result, MD5::Context *ctx){
    unsigned long used, free;
    used = ctx->lo & 0x3f;
    ctx->buffer[used++] = 0x80;
    free = 64 - used;
    if (free < 8) {
        memset(&ctx->buffer[used], 0, free);
        body(ctx, ctx->buffer, 64);
        used = 0;
        free = 64;
    }
    memset(&ctx->buffer[used], 0, free - 8);
    ctx->lo <<= 3;
    ctx->buffer[56] = static_cast<unsigned char>(ctx->lo);
    ctx->buffer[57] = static_cast<unsigned char>(ctx->lo >> 8);
    ctx->buffer[58] = static_cast<unsigned char>(ctx->lo >> 16);
    ctx->buffer[59] = static_cast<unsigned char>(ctx->lo >> 24);
    ctx->buffer[60] = static_cast<unsigned char>(ctx->hi);         
    ctx->buffer[61] = static_cast<unsigned char>(ctx->hi >> 8);
    ctx->buffer[62] = static_cast<unsigned char>(ctx->hi >> 16);   
    ctx->buffer[63] = static_cast<unsigned char>(ctx->hi >> 24);
    body(ctx, ctx->buffer, 64);
    result[0]  = static_cast<unsigned char>(ctx->a);
    result[1]  = static_cast<unsigned char>(ctx->a >> 8);
    result[2]  = static_cast<unsigned char>(ctx->a >> 16);
    result[3]  = static_cast<unsigned char>(ctx->a >> 24);
    result[4]  = static_cast<unsigned char>(ctx->b);      
    result[5]  = static_cast<unsigned char>(ctx->b >> 8);
    result[6]  = static_cast<unsigned char>(ctx->b >> 16); 
    result[7]  = static_cast<unsigned char>(ctx->b >> 24);
    result[8]  = static_cast<unsigned char>(ctx->c);          
    result[9]  = static_cast<unsigned char>(ctx->c >> 8);
    result[10] = static_cast<unsigned char>(ctx->c >> 16);
    result[11] = static_cast<unsigned char>(ctx->c >> 24);
    result[12] = static_cast<unsigned char>(ctx->d);           
    result[13] = static_cast<unsigned char>(ctx->d >> 8);
    result[14] = static_cast<unsigned char>(ctx->d >> 16);     
    result[15] = static_cast<unsigned char>(ctx->d >> 24);
    memset(ctx, 0, sizeof(*ctx));
    return;
}


