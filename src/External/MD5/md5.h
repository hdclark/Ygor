//MD5.h - An implementation of the RFC 1321 MD5 algorithm. 
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

#ifndef YGOR_MD5_H_
#define YGOR_MD5_H_
namespace MD5 {

//No need for fixed-width here. Anything >= 32 bits is OK.
typedef unsigned int YGOR_MD5_u_t;

struct Context {
    YGOR_MD5_u_t lo, hi, a, b, c, d;
    unsigned char buffer[64];
    YGOR_MD5_u_t block[16];
};

extern void Init(Context *c);
extern void Update(Context *c, void *data, unsigned long size);
extern void Final(unsigned char *result, Context *ctx);

} //namespace MD5
#endif
