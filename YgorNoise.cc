//YgorNoise.cc - Routines for providing smooth, deterministic random noise functions over some space. 
//

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <inttypes.h>  //For uint64_t - is this still needed??

#include "YgorMisc.h"    //Used for some bitwise operations.


//--------------------------------------------------------------------------------------------------------
//----------------------------------------- Simple Perlin Noise. -----------------------------------------
//--------------------------------------------------------------------------------------------------------
//Found this in a book "Texturing & Modelling - A Procedural Approach" by Ebert et al. (Third Edition.)
// Code itself begins on page 344. (I haven't seen such ugly code in a LONG time!! Little attempt was
// made to sanitize the code due to impending deadline.) Constants and definitions from the original
// source have been renamed to be slightly more safe for embedding into other code.
//
//NOTE: The code as seen in the original text will not compile due to errors (undefined variables)
// and typos. Also, C++ will take issue with the use of the 'restrict' used. These have been corrected
// here.
//
//NOTE: This function is (I think) NOT threadsafe. Encapsulating this code in a class would probably
// help. As such, feel free to revamp this code into a more safe, more convenient structure.
//
//This file provides a simple (as in, not full-featured) Perlin noise generator. Specifically it is
// a noise function over R3-implemented by a pseudorandom tricubic spline. The scale of the noise is 
// not adjustable. Assume a basic feature scale of size ~1.0. This means that stepping ~1.0 in any
// direction will result in moving from the current feature to another. Smooth stepping should be 
// around ~0.1.
//

static long int PERLIN_HAS_NOT_BEEN_INITIALIZED = 1;

#define PERLIN_DOT(a,b)       (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
#define PERLIN_B              256
#define PERLIN_AT(rx,ry,rz)   ( rx * q[0] + ry * q[1] + rz * q[2] )
#define PERLIN_S_CURVE(t)     ( t * t * (3.0f - 2.0f * t) )
#define PERLIN_LERP(t, a, b)  ( a + t * (b - a) )

static long int  PERLIN_P[PERLIN_B + PERLIN_B + 2];
static float PERLIN_G[PERLIN_B + PERLIN_B + 2][3];

#define PERLIN_SETUP(i,b0,b1,r0,r1) \
            t = vec[i] + 10000.0f;\
            b0 = static_cast<int>(t) & (PERLIN_B-1); \
            b1 = (b0+1) & (PERLIN_B-1); \
            r0 = t - static_cast<float>(static_cast<int>(t)); \
            r1 = r0 - 1.0f;

static void PERLIN_INIT(){
    int i, j, k;
    float v[3], s;

    /* Create an array of random gradient vectors uniformly on the unit sphere */
    srandom(1);
    for(i = 0 ; i < PERLIN_B ; i++){
        do{
            /* Choose uniformly in a cube */
            for(j=0 ; j<3 ; j++)  v[j] = (float)((random() % (PERLIN_B + PERLIN_B)) - PERLIN_B)/PERLIN_B;
            s = PERLIN_DOT(v,v);
        }while(s > 1.0); /* If not in sphere try again */
        s = ::sqrtf(s);

        /* Else normalize */
        for(j = 0 ; j < 3 ; j++)  PERLIN_G[i][j] = v[j] / s;
    }

    /* Create a pseudorandom permutation of [1 .. PERLIN_B] */
    for(i = 0 ; i < PERLIN_B ; i++)  PERLIN_P[i] = i;
    for(i = PERLIN_B ; i > 0 ; i -= 2){
        k = static_cast<int>(PERLIN_P[i]);
        PERLIN_P[i] = PERLIN_P[j = static_cast<int>(random() % PERLIN_B)];
        PERLIN_P[j] = k;
    }

    /* Extend g and p arrays to allow for faster indexing, */
    for(i = 0 ; i < PERLIN_B + 2 ; i++){
        PERLIN_P[PERLIN_B + i] = PERLIN_P[i];
        for(j = 0 ; j < 3 ; j++)  PERLIN_G[PERLIN_B + i][j] = PERLIN_G[i][j];
    }
}

float Perlin_Noise_3D(float vec[3]){
    int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
    float rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v;
    long int i, j;
    if(PERLIN_HAS_NOT_BEEN_INITIALIZED){
        PERLIN_HAS_NOT_BEEN_INITIALIZED = 0;
        PERLIN_INIT();
    }
    PERLIN_SETUP(0, bx0,bx1, rx0,rx1);
    PERLIN_SETUP(1, by0,by1, ry0,ry1);
    PERLIN_SETUP(2, bz0,bz1, rz0,rz1);
    i = PERLIN_P[ bx0 ];
    j = PERLIN_P[ bx1 ];

    b00 = static_cast<int>(PERLIN_P[ static_cast<int>(i) + by0 ]);
    b10 = static_cast<int>(PERLIN_P[ static_cast<int>(j) + by0 ]);
    b01 = static_cast<int>(PERLIN_P[ static_cast<int>(i) + by1 ]);
    b11 = static_cast<int>(PERLIN_P[ static_cast<int>(j) + by1 ]);
    sx = PERLIN_S_CURVE(rx0);
    sy = PERLIN_S_CURVE(ry0);
    sz = PERLIN_S_CURVE(rz0);
    q = PERLIN_G[ b00 + bz0 ] ; u = PERLIN_AT(rx0,ry0,rz0);
    q = PERLIN_G[ b10 + bz0 ] ; v = PERLIN_AT(rx1,ry0,rz0);
    a = PERLIN_LERP(sx, u, v);
    q = PERLIN_G[ b01 + bz0 ] ; u = PERLIN_AT(rx0,ry1,rz0);
    q = PERLIN_G[ b11 + bz0 ] ; v = PERLIN_AT(rx1,ry1,rz0);
    b = PERLIN_LERP(sx, u, v);
    c = PERLIN_LERP(sy, a, b);  /* interpolate in y @ low x */
    q = PERLIN_G[ b00 + bz1 ] ; u = PERLIN_AT(rx0,ry0,rz1);
    q = PERLIN_G[ b10 + bz1 ] ; v = PERLIN_AT(rx1,ry0,rz1);
    a = PERLIN_LERP(sx, u, v);
    q = PERLIN_G[ b01 + bz1 ] ; u = PERLIN_AT(rx0,ry1,rz1);
    q = PERLIN_G[ b11 + bz1 ] ; v = PERLIN_AT(rx1,ry1,rz1);
    b = PERLIN_LERP(sx, u, v);
    d = PERLIN_LERP(sy, a, b);  /* interpolate in y @ high x */
    return 1.5f * PERLIN_LERP(sz, c, d); /* interpolate in z */
}


//--------------------------------------------------------------------------------------------------------
//------------------------------------ Generic, Integer, Non-Smooth Noise --------------------------------
//--------------------------------------------------------------------------------------------------------
uint64_t Number_Jumbler_A(uint64_t x){
    const uint64_t y(x);

    x ^= 11677963410487775812ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 10779949498039803730ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 13404407400705181556ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 6541918881795066415ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 12551287238561482835ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 13047779125789609768ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2994432632735958836ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2467000861203750187ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 8292845813995609071ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 775761688528965408ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 14708771606945328297ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 3090858773646032064ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 15333197229523756226ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5994519175887787353ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 12097003938552935211ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7751136499891663970ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 14423678012146200904ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2088033743662153764ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 18327480670582234338ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 3349430276977723711ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 14073600593881942861ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4905370251551242997ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4503556080391512727ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 1858297172315272890ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 6351728996150245976ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5195107484978264032ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17868261105793408449ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 3943411000588836329ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 11177402618153334904ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4179134216513746350ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 3503138189720758389ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7407686287186576649ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7623866492108957448ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 3138755539887089379ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5516532306583212975ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 11740772793032145975ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2796049708711171099ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 14190032055658373991ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 16306342547769868559ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17694515465111101149ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 333263863792085603ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 10751618092494927052ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 14145961057407284533ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 6046801482689936766ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2380850499900011064ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5385725864950539947ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 15626414895563875691ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4256851693433675165ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9259793020680978138ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 29476127137113175ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 231300097139420897ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5286589245567266574ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 1820675968860990365ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7097480834010725142ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 6555026374276644673ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 13281119593223140774ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 1207248359956480450ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9479028900846873149ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7821019804152727552ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 12724587293365216909ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4684423490755983466ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7406728549200833893ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4878731261823934568ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17233881466973206632ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 8716552197633958503ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 11269414689932863789ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 9009424592425750034ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9491210774050506105ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4519807378737332680ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 6142862450555105085ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9266950187596804657ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 10995328152914833518ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5721368315548020081ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17764168814980580855ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4287029865632944307ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 13777950742951437576ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5949682953073125528ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5870346862516060030ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 17570007847167798916ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9863862280500131296ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 16081966532759700561ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17240375893298921448ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 12306082758509910019ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 5343826326170324117ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 1208712974382018330ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4277589956321002543ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 12999858112414469389ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 3599096295913788669ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 7788554561380934621ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 17267168206154539859ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 2623444373074140248ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= y;
    x ^= 11864595387004505704ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 8720368792257066745ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 8059989578706851368ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 4506901874962675126ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 9332565974672983007ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 16730165574228657939ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 1411249219504144069ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 13169202915281416743ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    x ^= 11385421140405288959ULL; x = PER_BYTE_BITWISE_ROT_L(x, 1);
    return x;
}

