/* 
 * Copyright Mehdi Sotoodeh.  All rights reserved. 
 * <mehdisotoodeh@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that source code retains the 
 * above copyright notice and following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef ECP_SELF_TEST

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "curve25519_mehdi.h"
#include "curve25519_donna.h"
#include "curve25519_SelfTest.h"
#include "sha512.h"

void ecp_PrintHexBytes(IN const char *name, IN const U8 *data, IN U32 size);
void ecp_PrintHexWords(IN const char *name, IN const U32 *data, IN U32 size);
void ecp_PrintWords(IN const char *name, IN const U_WORD *data, IN U32 size);

extern const U_WORD _w_P[K_WORDS];
extern const U_WORD _w_maxP[K_WORDS];
extern const U_WORD _w_I[K_WORDS];
extern const U_WORD _w_2d[K_WORDS];
extern const U_WORD _w_BPO[K_WORDS];
extern const PA_POINT _w_basepoint_perm64[16];

#define _w_Zero     _w_basepoint_perm64[0].T2d
#define _w_One      _w_basepoint_perm64[0].YpX

#define ECP_MOD(X)  while (ecp_Cmp(X, _w_P) >= 0) ecp_Sub(X, X, _w_P)

/*
    The curve: y2 = x^3 + 486662x^2 + x  over 2^255 - 19
    base point x = 9. 

    b = 256
    p = 2**255 - 19
    l = 2**252 + 27742317777372353535851937790883648493

    p = 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED
    l = 0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED
    d = -121665 * inv(121666)
    I = expmod(2,(p-1)/4,p)
    d = 0x52036CEE2B6FFE738CC740797779E89800700A4D4141D8AB75EB4DCA135978A3
    I = 0x2B8324804FC1DF0B2B4D00993DFBD7A72F431806AD2FE478C4EE1B274A0EA0B0
    -------------------------------------------------------------------------
    Internal points are represened as X/Z and maintained as a single array
    of 16 words: {X[8]:Z[8]}

*/
static const U32 inv_5[8] = {   // 1/5 mod p
    0x99999996,0x99999999,0x99999999,0x99999999,
    0x99999999,0x99999999,0x99999999,0x19999999 };

static const U32 _w_Pm1[8] = {
    0xFFFFFFEC,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
    0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x7FFFFFFF
};

static const U32 _w_D[8] = {
    0x135978A3,0x75EB4DCA,0x4141D8AB,0x00700A4D,
    0x7779E898,0x8CC74079,0x2B6FFE73,0x52036CEE
};

static const U8 _b_Pm1d2[32] = {    // (p-1)/d
    0xF6,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F };

static const U8 _b_Pm1[32] = {      // p-1
    0xEC,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F };

static const U8 _b_Om1[32] = {      // O-1
    0xEC,0xD3,0xF5,0x5C,0x1A,0x63,0x12,0x58,0xD6,0x9C,0xF7,0xA2,0xDE,0xF9,0xDE,0x14,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };

static const U8 _b_O[32] = {        // O    order of the base point
    0xED,0xD3,0xF5,0x5C,0x1A,0x63,0x12,0x58,0xD6,0x9C,0xF7,0xA2,0xDE,0xF9,0xDE,0x14,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };

// k1*k2 = 1 mod l ==> Q1 = k1.Q0 --> k2.Q1 = k2.k1.Q0 = Q0
static const U8 _b_k1[32] = {
    0x0B,0xE3,0xBE,0x63,0xBC,0x01,0x6A,0xAA,0xC9,0xE5,0x27,0x9F,0xB7,0x90,0xFB,0x44,
    0x37,0x2B,0x2D,0x4D,0xA1,0x73,0x5B,0x5B,0xB0,0x1A,0xC0,0x31,0x8D,0x89,0x21,0x03 };

static const U8 _b_k2[32] = {
    0x39,0x03,0xE3,0x27,0x7E,0x41,0x93,0x61,0x2D,0x3D,0x40,0x19,0x3D,0x60,0x68,0x21,
    0x60,0x25,0xEF,0x90,0xB9,0x8B,0x24,0xF2,0x50,0x60,0x94,0x21,0xD4,0x74,0x36,0x05 };

static const U8 pk1[32] = {
    0x46,0xF9,0xD2,0x09,0xC7,0x53,0x69,0xAC,0x5F,0x97,0xA3,0x28,0xA1,0x66,0x7A,0xC7,
    0xF8,0x6D,0x5E,0xC9,0xB2,0x0D,0x51,0x5C,0x11,0x39,0xA2,0x56,0x3B,0x10,0x13,0x60 };

static const U8 pk2[32] = {
    0x5A,0x26,0x6A,0xD3,0xD0,0x8D,0x9E,0x9B,0x8B,0xD9,0x2A,0xCC,0xCD,0x87,0xD5,0xB9,
    0x96,0xD1,0xDB,0xBA,0xB6,0xBC,0xC9,0x75,0x62,0x76,0xD7,0x61,0xF9,0x37,0x5F,0xA7 };

static const U32 _w_Two[8] = {  2,0,0,0,0,0,0,0 };
static const U32 _w_V19[8] = { 19,0,0,0,0,0,0,0 };

// G = Base Point
static const U32 _w_Gx[8] = {  9,0,0,0,0,0,0,0 };
static const U32 _w_Gy[8] = {
    0x7ECED3D9,0x29E9C5A2,0x6D7C61B2,0x923D4D7E,
    0x7748D14C,0xE01EDD2C,0xB8A086B4,0x20AE19A1 };

static const U32 _w_IxD[8] = {
    0x9E451EDD,0x71C41B45,0x7FBCC19E,0x49800849,
    0xBBCB7C34,0xF4C5CE99,0xB32C1AB4,0x024AEE07 };

static const U8 sha512_abc[] = {    // 'abc'
    0xDD,0xAF,0x35,0xA1,0x93,0x61,0x7A,0xBA,0xCC,0x41,0x73,0x49,0xAE,0x20,0x41,0x31,
    0x12,0xE6,0xFA,0x4E,0x89,0xA9,0x7E,0xA2,0x0A,0x9E,0xEE,0xE6,0x4B,0x55,0xD3,0x9A,
    0x21,0x92,0x99,0x2A,0x27,0x4F,0xC1,0xA8,0x36,0xBA,0x3C,0x23,0xA3,0xFE,0xEB,0xBD,
    0x45,0x4D,0x44,0x23,0x64,0x3C,0xE8,0x0E,0x2A,0x9A,0xC9,0x4F,0xA5,0x4C,0xA4,0x9F };

static const U8 sha512_ax1m[] = {   // 'a' repeated 1,000,000 times
    0xE7,0x18,0x48,0x3D,0x0C,0xE7,0x69,0x64,0x4E,0x2E,0x42,0xC7,0xBC,0x15,0xB4,0x63,
    0x8E,0x1F,0x98,0xB1,0x3B,0x20,0x44,0x28,0x56,0x32,0xA8,0x03,0xAF,0xA9,0x73,0xEB,
    0xDE,0x0F,0xF2,0x44,0x87,0x7E,0xA6,0x0A,0x4C,0xB0,0x43,0x2C,0xE5,0x77,0xC3,0x1B,
    0xEB,0x00,0x9C,0x5C,0x2C,0x49,0xAA,0x2E,0x4E,0xAD,0xB2,0x17,0xAD,0x8C,0xC0,0x9B };

int ecp_IsZero(IN const U32 *X)
{
    return (X[0] | X[1] | X[2] | X[3] | X[4] | X[5] | X[6] | X[7]) == 0;
}

int hash_test(int level)
{
    int i, rc = 0;
    SHA512_CTX H;
    U8 buff[100], md[SHA512_DIGEST_LENGTH];

    // [a:b] = H(sk)
    SHA512_Init(&H);
    SHA512_Update(&H, "abc", 3);
    SHA512_Final(md, &H);
    if (memcmp(md, sha512_abc, SHA512_DIGEST_LENGTH) != 0)
    {
        rc++;
        printf("KAT: SHA512('abc') FAILED!!\n");
        ecp_PrintHexBytes("H_1", md, SHA512_DIGEST_LENGTH);
    }

    SHA512_Init(&H);
    memset (buff, 'a', 100);
    for (i = 0; i < 10000; i++) SHA512_Update(&H, buff, 100);
    SHA512_Final(md, &H);
    if (memcmp(md, sha512_ax1m, SHA512_DIGEST_LENGTH) != 0)
    {
        rc++;
        printf("KAT: SHA512('a'*1000000) FAILED!!\n");
        ecp_PrintHexBytes("H_2", md, SHA512_DIGEST_LENGTH);
    }
    return rc;
}

/*
    Calculate: point R = a*P + b*Q  where P is base point
*/
void edp_DualPointMultiply(
    Affine_POINT *r,
    const U8 *a, const U8 *b, const Affine_POINT *q)
{
    int i, j;
    M32 k;
    Ext_POINT S;
    PA_POINT U;
    PE_POINT V;

    // U = pre-compute(Q)
    ecp_AddReduce(U.YpX, q->y, q->x);
    ecp_SubReduce(U.YmX, q->y, q->x);
    ecp_MulReduce(U.T2d, q->y, q->x);
    ecp_MulReduce(U.T2d, U.T2d, _w_2d);

    // set V = pre-compute(P + Q)
    ecp_Copy(S.x, q->x);
    ecp_Copy(S.y, q->y);
    ecp_SetValue(S.z, 1);
    ecp_MulReduce(S.t, S.x, S.y);
    edp_AddBasePoint(&S);   // S = P + Q
    // 
    ecp_AddReduce(V.YpX, S.y, S.x);
    ecp_SubReduce(V.YmX, S.y, S.x);
    ecp_MulReduce(V.T2d, S.t, _w_2d);
    ecp_AddReduce(V.Z2, S.z, S.z);

    // Set S = (0,1)
    ecp_SetValue(S.x, 0);
    ecp_SetValue(S.y, 1);
    ecp_SetValue(S.z, 1);
    ecp_SetValue(S.t, 0);

    for (i = 32; i-- > 0;)
    {
        k.u8.b0 = a[i];
        k.u8.b1 = b[i];
        for (j = 0; j < 8; j++)
        {
            edp_DoublePoint(&S);
            switch (k.u32 & 0x8080)
            {
            case 0x0080: edp_AddBasePoint(&S); break;
            case 0x8000: edp_AddAffinePoint(&S, &U); break;
            case 0x8080: edp_AddPoint(&S, &S, &V); break;
            }
            k.u32 <<= 1;
        }
    }
    ecp_Inverse(S.z, S.z);
    ecp_MulMod(r->x, S.x, S.z);
    ecp_MulMod(r->y, S.y, S.z);
}

void print_words(IN const char *txt, IN const U32 *data, IN U32 size)
{
    U32 i;
    printf("%s0x%08X", txt, *data++);
    for (i = 1; i < size; i++) printf(",0x%08X", *data++);
}

void pre_compute_base_point()
{
    Ext_POINT P = {{0},{1},{1},{0}};
    PA_POINT R;
    int i;
    printf("\nconst PA_POINT pre_BaseMultiples[16] = \n{\n");
    for (i = 0; i < 16; i++)
    {
        printf("  { // %d*P\n", i);
        ecp_AddReduce(R.YpX, P.y, P.x); ECP_MOD(R.YpX);
        ecp_SubReduce(R.YmX, P.y, P.x); ECP_MOD(R.YmX);
        ecp_MulMod(R.T2d, P.t, _w_2d);

        print_words("    W256(",R.YpX, K_WORDS);
        print_words("),\n    W256(",R.YmX, K_WORDS);
        print_words("),\n    W256(",R.T2d, K_WORDS);
        if (i == 15)
        {
            printf(")\n  }\n};\n");
            break;
        }
        printf(")\n  },\n");

        edp_AddBasePoint(&P);
        // make it affine
        ecp_Inverse(P.z, P.z);
        ecp_MulMod(P.x, P.x, P.z);
        ecp_MulMod(P.y, P.y, P.z);
        ecp_MulMod(P.t, P.x, P.y);
        ecp_SetValue(P.z, 1);
    }
}

static const Ext_POINT _w_BasePoint = {   // y = 4/5 mod P
    W256(0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3),
    W256(0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666),
    W256(0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000),
    W256(0xA5B7DDA3,0x6DDE8AB3,0x775152F5,0x20F09F80,0x64ABE37D,0x66EA4E8E,0xD78B7665,0x67875F0F)
};

void Ext2Affine(PA_POINT *r, Ext_POINT *p)
{
    ecp_Inverse(p->z, p->z);
    ecp_MulMod(p->x, p->x, p->z);
    ecp_MulMod(p->y, p->y, p->z);
    ecp_MulMod(p->t, p->x, p->y);
    ecp_SetValue(p->z, 1);

    ecp_AddReduce(r->YpX, p->y, p->x); ECP_MOD(r->YpX);
    ecp_SubReduce(r->YmX, p->y, p->x); ECP_MOD(r->YmX);
    ecp_MulMod(r->T2d, p->t, _w_2d);
}

void pre_compute_base_powers()
{
    Ext_POINT S = _w_BasePoint;
    PA_POINT P0, P1, P2, P3;
    PA_POINT R;
    int i, j;

    // Calculate: P0=base, P1=(2^64)*P0, P2=(2^64)*P1, P3=(2^64)*P2
    Ext2Affine(&P0, &S);
    for (j = 0; j < 64; j++) edp_DoublePoint(&S);
    Ext2Affine(&P1, &S);
    for (j = 0; j < 64; j++) edp_DoublePoint(&S);
    Ext2Affine(&P2, &S);
    for (j = 0; j < 64; j++) edp_DoublePoint(&S);
    Ext2Affine(&P3, &S);

    printf("\nconst PA_POINT _w_basepoint_perm64[16] = \n{\n");
    for (i = 0; i < 16; i++)
    {
        ecp_SetValue(S.x, 0);
        ecp_SetValue(S.y, 1);
        ecp_SetValue(S.z, 1);
        ecp_SetValue(S.t, 0);

        if (i & 1) edp_AddAffinePoint(&S, &P0);
        if (i & 2) edp_AddAffinePoint(&S, &P1);
        if (i & 4) edp_AddAffinePoint(&S, &P2);
        if (i & 8) edp_AddAffinePoint(&S, &P3);
        Ext2Affine(&R, &S);

        printf("  { // P{%d}\n", i);
        print_words("    W256(",R.YpX, K_WORDS);
        print_words("),\n    W256(",R.YmX, K_WORDS);
        print_words("),\n    W256(",R.T2d, K_WORDS);
        if (i == 15)
        {
            printf(")\n  }\n};\n");
            break;
        }
        printf(")\n  },\n");
    }
}

int curve25519_SelfTest(int level)
{
    int rc = 0;
    M32 m;
    U32 A[8], B[8], C[8];
    U8 a[32], b[32], c[32], d[32];

    // Make sure library is built with correct byte ordering
    m.u32 = 0x12345678;
    if (m.u16.w1 != 0x1234 || m.u16.w0 != 0x5678 ||
        m.u8.b3 != 0x12 || m.u8.b2 != 0x34 || m.u8.b1 != 0x56 || m.u8.b0 != 0x78)
    {
        rc++;
        if (m.bytes[0] == 0x12) // big-endian 
            printf("Incorrect byte order configuration used (define ECP_BIG_ENDIAN).\n");
        if (m.bytes[0] == 0x78) // little-endian 
            printf("Incorrect byte order configuration used (define ECP_LITTLE_ENDIAN).\n");
        return rc;
    }

    rc = hash_test(level);

    ecp_AddReduce(A, _w_I, _w_P);
    ECP_MOD(A);
    if (ecp_Cmp(A, _w_I) != 0)
    {
        rc++;
        printf("assert I+p == I mod p FAILED!!\n");
        ecp_PrintHexWords("A_1", A, 8);
    }
    ecp_MulReduce(B, _w_I, _w_D);
    ECP_MOD(B);
    if (ecp_Cmp(B, _w_IxD) != 0)
    {
        rc++;
        printf("assert I*D FAILED!!\n");
        ecp_PrintHexWords("A_2", B, 8);
    }

    ecp_SetValue(A, 50153);
    ecp_Inverse(B, A);
    ecp_MulMod(A, A, B);
    if (ecp_Cmp(A, _w_One) != 0)
    {
        rc++;
        printf("invmod FAILED!!\n");
        ecp_PrintHexWords("inv_50153", B, 8);
        ecp_PrintHexWords("expected_1", A, 8);
    }

    // assert expmod(d,(p-1)/2,p) == p-1
    ecp_ExpMod(A, _w_D, _b_Pm1d2, 32);
    if (ecp_Cmp(A, _w_Pm1) != 0)
    {
        rc++;
        printf("assert expmod(d,(p-1)/2,p) == p-1 FAILED!!\n");
        ecp_PrintHexWords("A_3", A, 8);
    }
    // assert I**2 == p-1
    ecp_MulMod(A, _w_I, _w_I);
    if (ecp_Cmp(A, _w_Pm1) != 0)
    {
        rc++;
        printf("assert expmod(I,2,p) == p-1 FAILED!!\n");
        ecp_PrintHexWords("A_4", A, 8);
    }

    ecp_CalculateY(a, ecp_BasePoint);
    ecp_BytesToWords(A, a);
    if (ecp_Cmp(A, _w_Gy) != 0)
    {
        rc++;
        printf("assert clacY(Base) == Base.y FAILED!!\n");
        ecp_PrintHexBytes("Calculated_Base.y", a, 32);
    }

    ecp_PointMultiply(a, ecp_BasePoint, _b_Om1, 32);
    if (memcmp(a, ecp_BasePoint, 32) != 0)
    {
        rc++;
        printf("assert (l-1).Base == Base FAILED!!\n");
        ecp_PrintHexBytes("A_5", a, 32);
    }

    ecp_PointMultiply(a, ecp_BasePoint, _b_O, 32);
    ecp_BytesToWords(A, a);
    if (!ecp_IsZero(A))
    {
        rc++;
        printf("assert l.Base == 0 FAILED!!\n");
        ecp_PrintHexBytes("A_6", a, 32);
    }

    // Key generation
    ecp_PointMultiply(a, ecp_BasePoint, pk1, 32);
    ecp_PointMultiply(b, ecp_BasePoint, pk2, 32);

    // ECDH - key exchange
    ecp_PointMultiply(c, b, pk1, 32);
    ecp_PointMultiply(d, a, pk2, 32);
    if (memcmp(c, d, 32) != 0)
    {
        rc++;
        printf("ECDH key exchange FAILED!!\n");
        ecp_PrintHexBytes("PublicKey1", a, 32);
        ecp_PrintHexBytes("PublicKey2", b, 32);
        ecp_PrintHexBytes("SharedKey1", c, 32);
        ecp_PrintHexBytes("SharedKey2", d, 32);
    }

    memset(a, 0x44, 32);        // our secret key
    ecp_PointMultiply(b, ecp_BasePoint, a, 32); // public key
    ecp_PointMultiply(c, b, _b_k1, 32);
    ecp_PointMultiply(d, c, _b_k2, 32);
    if (memcmp(d, b, 32) != 0)
    {
        rc++;
        printf("assert k1.k2.D == D FAILED!!\n");
        ecp_PrintHexBytes("D", d, 8);
        ecp_PrintHexBytes("C", c, 8);
        ecp_PrintHexBytes("A", a, 8);
    }

    ecp_BytesToWords(A, _b_k1);
    ecp_BytesToWords(B, _b_k2);
    eco_InvModBPO(C, A);
    if (ecp_Cmp(C, B) != 0)
    {
        rc++;
        printf("assert 1/k1 == k2 mod BPO FAILED!!\n");
        ecp_PrintHexWords("Calc", C, 8);
        ecp_PrintHexWords("Expt", B, 8);
    }

    eco_MulMod(C, A, B);
    if (ecp_Cmp(C, _w_One) != 0)
    {
        rc++;
        printf("assert k1*k2 == 1 mod BPO FAILED!!\n");
        ecp_PrintHexWords("Calc", C, 8);
    }

    //pre_compute_base_point();
    //pre_compute_base_powers();

    return rc;
}

static U8 m_6[32] = {6};
static U8 m_7[32] = {7};
static U8 m_11[32] = {11};
static U8 m_50[32] = {50};
static U8 m_127[32] = {127};

// Pre-calculate base point values
static const Affine_POINT ed25519_BasePoint = {   // y = 4/5 mod P
    W256(0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3),
    W256(0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666)
};

void ecp_PrintHexWords(IN const char *name, IN const U_WORD *data, IN U32 size);

void edp_DualPointMultiply(
    Affine_POINT *r,
    const U8 *a, const U8 *b, const Affine_POINT *q);

int ed25519_selftest()
{
    int rc = 0;
    U8 pp[32], m1[32], m2[32];
    U_WORD u[K_WORDS], v[K_WORDS];
    Affine_POINT a, b, c, d;

    ed25519_PackPoint(pp, ed25519_BasePoint.y, ed25519_BasePoint.x[0] & 1);
    ed25519_UnpackPoint(&a, pp);
    if (ecp_Cmp(a.x, ed25519_BasePoint.x) != 0)
    {
        rc++;
        printf("-- Unpack error.");
        ecp_PrintHexWords("a_x", a.x, K_WORDS);
    }

    // a = 7*B
    edp_BasePointMultiply(&a, m_7, 0);

    // b = 11*B
    edp_BasePointMultiply(&b, m_11, 0);

    // c = 50*B + 7*b = 127*B
    edp_DualPointMultiply(&c, m_50, m_7, &b);

    // d = 127*B
    edp_BasePointMultiply(&d, m_127, 0);

    // check c == d
    if (ecp_Cmp(c.y, d.y) != 0 || ecp_Cmp(c.x, d.x) != 0)
    {
        rc++;
        printf("-- edp_DualPointMultiply(1) FAILED!!");
        ecp_PrintHexWords("c_x", c.x, K_WORDS);
        ecp_PrintHexWords("c_y", c.y, K_WORDS);
        ecp_PrintHexWords("d_x", d.x, K_WORDS);
        ecp_PrintHexWords("d_y", d.y, K_WORDS);
    }

    // c = 11*b + 6*B = 127*B
    edp_DualPointMultiply(&c, m_6, m_11, &b);
    // check c == d
    if (ecp_Cmp(c.y, d.y) != 0 || ecp_Cmp(c.x, d.x) != 0)
    {
        rc++;
        printf("-- edp_DualPointMultiply(2) FAILED!!");
        ecp_PrintHexWords("c_x", c.x, K_WORDS);
        ecp_PrintHexWords("c_y", c.y, K_WORDS);
        ecp_PrintHexWords("d_x", d.x, K_WORDS);
        ecp_PrintHexWords("d_y", d.y, K_WORDS);
    }

    ecp_SetValue(u, 0x11223344);
    ecp_WordsToBytes(m1, u);
    edp_BasePointMultiply(&a, m1, 0);   // a = u*B
    eco_MulMod(v, u, u);
    ecp_Sub(v, _w_BPO, v);          // v = -u^2
    ecp_WordsToBytes(m2, v);
    edp_DualPointMultiply(&a, m2, m1, &a);  // v*B + u*A = (-u^2 + u*u)*B
    // assert a == infinty
    if (ecp_Cmp(a.x, _w_Zero) != 0 || ecp_Cmp(a.y, _w_One) != 0)
    {
        rc++;
        printf("-- edp_DualPointMultiply(3) FAILED!!");
        ecp_PrintHexWords("a_x", a.x, K_WORDS);
        ecp_PrintHexWords("a_y", a.y, K_WORDS);
    }

    return rc;
}

#endif // ECP_SELF_TEST