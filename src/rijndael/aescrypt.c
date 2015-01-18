/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

 LICENSE TERMS

 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:

  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;

  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;

  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 20/12/2007
*/

#include <stdlib.h>
#include "aesopt.h"
#include "aestab.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#if defined(NO_ENCRYPTION_TABLE) || defined(NO_DECRYPTION_TABLE)
#define cf2(x) ((x << 1) ^ (((x >> 7) & 1) * 0x11b))
#define cf3(x) (cf2(x) ^ x)
#define cf4(x) ((x << 2) ^ (((x >> 6) & 1) * 0x11b) ^ (((x >> 6) & 2) * 0x11b))
#define cf8(x) ((x << 3) ^ (((x >> 5) & 1) * 0x11b) ^ (((x >> 5) & 2) * 0x11b) ^ (((x >> 5) & 4) * 0x11b))
#define cf9(x) (cf8(x) ^ x)
#define cfb(x) (cf8(x) ^ cf2(x) ^ x)
#define cfd(x) (cf8(x) ^ cf4(x) ^ x)
#define cfe(x) (cf8(x) ^ cf4(x) ^ cf2(x))
#define u8(x)  0x0, x, x, cf3(x), cf2(x), x, x, cf3(x)
#define v8(x)  cfe(x), cf9(x), cfd(x), cfb(x), cfe(x), cf9(x), cfd(x), x

typedef unsigned char UINT8;
#endif
    
#if defined(NO_DECRYPTION_TABLE)
UINT8 dec_tab[] = { v8(0x52),v8(0x09),v8(0x6a),v8(0xd5),v8(0x30),v8(0x36),v8(0xa5),v8(0x38),
                    v8(0xbf),v8(0x40),v8(0xa3),v8(0x9e),v8(0x81),v8(0xf3),v8(0xd7),v8(0xfb),
                    v8(0x7c),v8(0xe3),v8(0x39),v8(0x82),v8(0x9b),v8(0x2f),v8(0xff),v8(0x87),
                    v8(0x34),v8(0x8e),v8(0x43),v8(0x44),v8(0xc4),v8(0xde),v8(0xe9),v8(0xcb),
                    v8(0x54),v8(0x7b),v8(0x94),v8(0x32),v8(0xa6),v8(0xc2),v8(0x23),v8(0x3d),
                    v8(0xee),v8(0x4c),v8(0x95),v8(0x0b),v8(0x42),v8(0xfa),v8(0xc3),v8(0x4e),
                    v8(0x08),v8(0x2e),v8(0xa1),v8(0x66),v8(0x28),v8(0xd9),v8(0x24),v8(0xb2),
                    v8(0x76),v8(0x5b),v8(0xa2),v8(0x49),v8(0x6d),v8(0x8b),v8(0xd1),v8(0x25),
                    v8(0x72),v8(0xf8),v8(0xf6),v8(0x64),v8(0x86),v8(0x68),v8(0x98),v8(0x16),
                    v8(0xd4),v8(0xa4),v8(0x5c),v8(0xcc),v8(0x5d),v8(0x65),v8(0xb6),v8(0x92),
                    v8(0x6c),v8(0x70),v8(0x48),v8(0x50),v8(0xfd),v8(0xed),v8(0xb9),v8(0xda),
                    v8(0x5e),v8(0x15),v8(0x46),v8(0x57),v8(0xa7),v8(0x8d),v8(0x9d),v8(0x84),
                    v8(0x90),v8(0xd8),v8(0xab),v8(0x00),v8(0x8c),v8(0xbc),v8(0xd3),v8(0x0a),
                    v8(0xf7),v8(0xe4),v8(0x58),v8(0x05),v8(0xb8),v8(0xb3),v8(0x45),v8(0x06),
                    v8(0xd0),v8(0x2c),v8(0x1e),v8(0x8f),v8(0xca),v8(0x3f),v8(0x0f),v8(0x02),
                    v8(0xc1),v8(0xaf),v8(0xbd),v8(0x03),v8(0x01),v8(0x13),v8(0x8a),v8(0x6b),
                    v8(0x3a),v8(0x91),v8(0x11),v8(0x41),v8(0x4f),v8(0x67),v8(0xdc),v8(0xea),
                    v8(0x97),v8(0xf2),v8(0xcf),v8(0xce),v8(0xf0),v8(0xb4),v8(0xe6),v8(0x73),
                    v8(0x96),v8(0xac),v8(0x74),v8(0x22),v8(0xe7),v8(0xad),v8(0x35),v8(0x85),
                    v8(0xe2),v8(0xf9),v8(0x37),v8(0xe8),v8(0x1c),v8(0x75),v8(0xdf),v8(0x6e),
                    v8(0x47),v8(0xf1),v8(0x1a),v8(0x71),v8(0x1d),v8(0x29),v8(0xc5),v8(0x89),
                    v8(0x6f),v8(0xb7),v8(0x62),v8(0x0e),v8(0xaa),v8(0x18),v8(0xbe),v8(0x1b),
                    v8(0xfc),v8(0x56),v8(0x3e),v8(0x4b),v8(0xc6),v8(0xd2),v8(0x79),v8(0x20),
                    v8(0x9a),v8(0xdb),v8(0xc0),v8(0xfe),v8(0x78),v8(0xcd),v8(0x5a),v8(0xf4),
                    v8(0x1f),v8(0xdd),v8(0xa8),v8(0x33),v8(0x88),v8(0x07),v8(0xc7),v8(0x31),
                    v8(0xb1),v8(0x12),v8(0x10),v8(0x59),v8(0x27),v8(0x80),v8(0xec),v8(0x5f),
                    v8(0x60),v8(0x51),v8(0x7f),v8(0xa9),v8(0x19),v8(0xb5),v8(0x4a),v8(0x0d),
                    v8(0x2d),v8(0xe5),v8(0x7a),v8(0x9f),v8(0x93),v8(0xc9),v8(0x9c),v8(0xef),
                    v8(0xa0),v8(0xe0),v8(0x3b),v8(0x4d),v8(0xae),v8(0x2a),v8(0xf5),v8(0xb0),
                    v8(0xc8),v8(0xeb),v8(0xbb),v8(0x3c),v8(0x83),v8(0x53),v8(0x99),v8(0x61),
                    v8(0x17),v8(0x2b),v8(0x04),v8(0x7e),v8(0xba),v8(0x77),v8(0xd6),v8(0x26),
                    v8(0xe1),v8(0x69),v8(0x14),v8(0x63),v8(0x55),v8(0x21),v8(0x0c),v8(0x7d) };
#endif

#if defined(NO_ENCRYPTION_TABLE)
UINT8 enc_tab[] = { u8(0x63), u8(0x7c), u8(0x77), u8(0x7b), u8(0xf2), u8(0x6b), u8(0x6f), u8(0xc5), u8(0x30),
                    u8(0x01), u8(0x67), u8(0x2b), u8(0xfe), u8(0xd7), u8(0xab), u8(0x76), u8(0xca), u8(0x82),
                    u8(0xc9), u8(0x7d), u8(0xfa), u8(0x59), u8(0x47), u8(0xf0), u8(0xad), u8(0xd4), u8(0xa2),
                    u8(0xaf), u8(0x9c), u8(0xa4), u8(0x72), u8(0xc0), u8(0xb7), u8(0xfd), u8(0x93), u8(0x26),
                    u8(0x36), u8(0x3f), u8(0xf7), u8(0xcc), u8(0x34), u8(0xa5), u8(0xe5), u8(0xf1), u8(0x71),
                    u8(0xd8), u8(0x31), u8(0x15), u8(0x04), u8(0xc7), u8(0x23), u8(0xc3), u8(0x18), u8(0x96),
                    u8(0x05), u8(0x9a), u8(0x07), u8(0x12), u8(0x80), u8(0xe2), u8(0xeb), u8(0x27), u8(0xb2),
                    u8(0x75), u8(0x09), u8(0x83), u8(0x2c), u8(0x1a), u8(0x1b), u8(0x6e), u8(0x5a), u8(0xa0),
                    u8(0x52), u8(0x3b), u8(0xd6), u8(0xb3), u8(0x29), u8(0xe3), u8(0x2f), u8(0x84), u8(0x53),
                    u8(0xd1), u8(0x00), u8(0xed), u8(0x20), u8(0xfc), u8(0xb1), u8(0x5b), u8(0x6a), u8(0xcb),
                    u8(0xbe), u8(0x39), u8(0x4a), u8(0x4c), u8(0x58), u8(0xcf), u8(0xd0), u8(0xef), u8(0xaa),
                    u8(0xfb), u8(0x43), u8(0x4d), u8(0x33), u8(0x85), u8(0x45), u8(0xf9), u8(0x02), u8(0x7f),
                    u8(0x50), u8(0x3c), u8(0x9f), u8(0xa8), u8(0x51), u8(0xa3), u8(0x40), u8(0x8f), u8(0x92),
                    u8(0x9d), u8(0x38), u8(0xf5), u8(0xbc), u8(0xb6), u8(0xda), u8(0x21), u8(0x10), u8(0xff),
                    u8(0xf3), u8(0xd2), u8(0xcd), u8(0x0c), u8(0x13), u8(0xec), u8(0x5f), u8(0x97), u8(0x44),
                    u8(0x17), u8(0xc4), u8(0xa7), u8(0x7e), u8(0x3d), u8(0x64), u8(0x5d), u8(0x19), u8(0x73),
                    u8(0x60), u8(0x81), u8(0x4f), u8(0xdc), u8(0x22), u8(0x2a), u8(0x90), u8(0x88), u8(0x46),
                    u8(0xee), u8(0xb8), u8(0x14), u8(0xde), u8(0x5e), u8(0x0b), u8(0xdb), u8(0xe0), u8(0x32),
                    u8(0x3a), u8(0x0a), u8(0x49), u8(0x06), u8(0x24), u8(0x5c), u8(0xc2), u8(0xd3), u8(0xac),
                    u8(0x62), u8(0x91), u8(0x95), u8(0xe4), u8(0x79), u8(0xe7), u8(0xc8), u8(0x37), u8(0x6d),
                    u8(0x8d), u8(0xd5), u8(0x4e), u8(0xa9), u8(0x6c), u8(0x56), u8(0xf4), u8(0xea), u8(0x65),
                    u8(0x7a), u8(0xae), u8(0x08), u8(0xba), u8(0x78), u8(0x25), u8(0x2e), u8(0x1c), u8(0xa6),
                    u8(0xb4), u8(0xc6), u8(0xe8), u8(0xdd), u8(0x74), u8(0x1f), u8(0x4b), u8(0xbd), u8(0x8b),
                    u8(0x8a), u8(0x70), u8(0x3e), u8(0xb5), u8(0x66), u8(0x48), u8(0x03), u8(0xf6), u8(0x0e),
                    u8(0x61), u8(0x35), u8(0x57), u8(0xb9), u8(0x86), u8(0xc1), u8(0x1d), u8(0x9e), u8(0xe1),
                    u8(0xf8), u8(0x98), u8(0x11), u8(0x69), u8(0xd9), u8(0x8e), u8(0x94), u8(0x9b), u8(0x1e),
                    u8(0x87), u8(0xe9), u8(0xce), u8(0x55), u8(0x28), u8(0xdf), u8(0x8c), u8(0xa1), u8(0x89),
                    u8(0x0d), u8(0xbf), u8(0xe6), u8(0x42), u8(0x68), u8(0x41), u8(0x99), u8(0x2d), u8(0x0f),
                    u8(0xb0), u8(0x54), u8(0xbb), u8(0x16) };
#endif

#define si(y,x,k,c) (s(y,c) = word_in(x, c) ^ (k)[c])
#define so(y,x,c)   word_out(y, c, s(x,c))

#if defined(ARRAYS)
#define locals(y,x)     x[4],y[4]
#else
#define locals(y,x)     x##0,x##1,x##2,x##3,y##0,y##1,y##2,y##3
#endif

#define l_copy(y, x)    s(y,0) = s(x,0); s(y,1) = s(x,1); \
                        s(y,2) = s(x,2); s(y,3) = s(x,3);
#define state_in(y,x,k) si(y,x,k,0); si(y,x,k,1); si(y,x,k,2); si(y,x,k,3)
#define state_out(y,x)  so(y,x,0); so(y,x,1); so(y,x,2); so(y,x,3)
#define round(rm,y,x,k) rm(y,x,k,0); rm(y,x,k,1); rm(y,x,k,2); rm(y,x,k,3)

#if ( FUNCS_IN_C & ENCRYPTION_IN_C )

/* Visual C++ .Net v7.1 provides the fastest encryption code when using
   Pentium optimiation with small code but this is poor for decryption
   so we need to control this with the following VC++ pragmas
*/

#if defined( _MSC_VER ) && !defined( _WIN64 )
#pragma optimize( "s", on )
#endif

/* Given the column (c) of the output state variable, the following
   macros give the input state variables which are needed in its
   computation for each row (r) of the state. All the alternative
   macros give the same end values but expand into different ways
   of calculating these values.  In particular the complex macro
   used for dynamically variable block sizes is designed to expand
   to a compile time constant whenever possible but will expand to
   conditional clauses on some branches (I am grateful to Frank
   Yellin for this construction)
*/

#define fwd_var(x,r,c)\
 ( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
 : r == 1 ? ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0))\
 : r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
 :          ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2)))

#if defined(FT4_SET)
#undef  dec_fmvars
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,n),fwd_var,rf1,c))
#elif defined(FT1_SET)
#undef  dec_fmvars
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ one_table(x,upr,t_use(f,n),fwd_var,rf1,c))
#else
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ fwd_mcol(no_table(x,t_use(s,box),fwd_var,rf1,c)))
#endif

#if defined(FL4_SET)
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,l),fwd_var,rf1,c))
#elif defined(FL1_SET)
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ one_table(x,ups,t_use(f,l),fwd_var,rf1,c))
#else
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ no_table(x,t_use(s,box),fwd_var,rf1,c))
#endif

AES_RETURN aes_encrypt(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1])
{   uint_32t         locals(b0, b1);
    const uint_32t   *kp;
#if defined( dec_fmvars )
    dec_fmvars; /* declare variables for fwd_mcol() if needed */
#endif

    if( cx->inf.b[0] != 10 * 16 && cx->inf.b[0] != 12 * 16 && cx->inf.b[0] != 14 * 16 )
        return EXIT_FAILURE;

    kp = cx->ks;
    state_in(b0, in, kp);

#if (ENC_UNROLL == FULL)

    switch(cx->inf.b[0])
    {
    case 14 * 16:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        kp += 2 * N_COLS;
    case 12 * 16:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        kp += 2 * N_COLS;
    case 10 * 16:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 3 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 4 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 5 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 6 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 7 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 8 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 9 * N_COLS);
        round(fwd_lrnd, b0, b1, kp +10 * N_COLS);
    }

#else

#if (ENC_UNROLL == PARTIAL)
    {   uint_32t    rnd;
        for(rnd = 0; rnd < (cx->inf.b[0] >> 5) - 1; ++rnd)
        {
            kp += N_COLS;
            round(fwd_rnd, b1, b0, kp);
            kp += N_COLS;
            round(fwd_rnd, b0, b1, kp);
        }
        kp += N_COLS;
        round(fwd_rnd,  b1, b0, kp);
#else
    {   uint_32t    rnd;
        for(rnd = 0; rnd < (cx->inf.b[0] >> 4) - 1; ++rnd)
        {
            kp += N_COLS;
            round(fwd_rnd, b1, b0, kp);
            l_copy(b0, b1);
        }
#endif
        kp += N_COLS;
        round(fwd_lrnd, b0, b1, kp);
    }
#endif

    state_out(out, b0);
    return EXIT_SUCCESS;
}

#endif

#if ( FUNCS_IN_C & DECRYPTION_IN_C)

/* Visual C++ .Net v7.1 provides the fastest encryption code when using
   Pentium optimiation with small code but this is poor for decryption
   so we need to control this with the following VC++ pragmas
*/

#if defined( _MSC_VER ) && !defined( _WIN64 )
#pragma optimize( "t", on )
#endif

/* Given the column (c) of the output state variable, the following
   macros give the input state variables which are needed in its
   computation for each row (r) of the state. All the alternative
   macros give the same end values but expand into different ways
   of calculating these values.  In particular the complex macro
   used for dynamically variable block sizes is designed to expand
   to a compile time constant whenever possible but will expand to
   conditional clauses on some branches (I am grateful to Frank
   Yellin for this construction)
*/

#define inv_var(x,r,c)\
 ( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
 : r == 1 ? ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2))\
 : r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
 :          ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0)))

#if defined(IT4_SET)
#undef  dec_imvars
#define inv_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,n),inv_var,rf1,c))
#elif defined(IT1_SET)
#undef  dec_imvars
#define inv_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ one_table(x,upr,t_use(i,n),inv_var,rf1,c))
#else
#define inv_rnd(y,x,k,c)    (s(y,c) = inv_mcol((k)[c] ^ no_table(x,t_use(i,box),inv_var,rf1,c)))
#endif

#if defined(IL4_SET)
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,l),inv_var,rf1,c))
#elif defined(IL1_SET)
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ one_table(x,ups,t_use(i,l),inv_var,rf1,c))
#else
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ no_table(x,t_use(i,box),inv_var,rf1,c))
#endif

/* This code can work with the decryption key schedule in the   */
/* order that is used for encrytpion (where the 1st decryption  */
/* round key is at the high end ot the schedule) or with a key  */
/* schedule that has been reversed to put the 1st decryption    */
/* round key at the low end of the schedule in memory (when     */
/* AES_REV_DKS is defined)                                      */

#ifdef AES_REV_DKS
#define key_ofs     0
#define rnd_key(n)  (kp + n * N_COLS)
#else
#define key_ofs     1
#define rnd_key(n)  (kp - n * N_COLS)
#endif

AES_RETURN aes_decrypt(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1])
{   uint_32t        locals(b0, b1);
#if defined( dec_imvars )
    dec_imvars; /* declare variables for inv_mcol() if needed */
#endif
    const uint_32t *kp;

    if( cx->inf.b[0] != 10 * 16 && cx->inf.b[0] != 12 * 16 && cx->inf.b[0] != 14 * 16 )
        return EXIT_FAILURE;

    kp = cx->ks + (key_ofs ? (cx->inf.b[0] >> 2) : 0);
    state_in(b0, in, kp);

#if (DEC_UNROLL == FULL)

    kp = cx->ks + (key_ofs ? 0 : (cx->inf.b[0] >> 2));
    switch(cx->inf.b[0])
    {
    case 14 * 16:
        round(inv_rnd,  b1, b0, rnd_key(-13));
        round(inv_rnd,  b0, b1, rnd_key(-12));
    case 12 * 16:
        round(inv_rnd,  b1, b0, rnd_key(-11));
        round(inv_rnd,  b0, b1, rnd_key(-10));
    case 10 * 16:
        round(inv_rnd,  b1, b0, rnd_key(-9));
        round(inv_rnd,  b0, b1, rnd_key(-8));
        round(inv_rnd,  b1, b0, rnd_key(-7));
        round(inv_rnd,  b0, b1, rnd_key(-6));
        round(inv_rnd,  b1, b0, rnd_key(-5));
        round(inv_rnd,  b0, b1, rnd_key(-4));
        round(inv_rnd,  b1, b0, rnd_key(-3));
        round(inv_rnd,  b0, b1, rnd_key(-2));
        round(inv_rnd,  b1, b0, rnd_key(-1));
        round(inv_lrnd, b0, b1, rnd_key( 0));
    }

#else

#if (DEC_UNROLL == PARTIAL)
    {   uint_32t    rnd;
        for(rnd = 0; rnd < (cx->inf.b[0] >> 5) - 1; ++rnd)
        {
            kp = rnd_key(1);
            round(inv_rnd, b1, b0, kp);
            kp = rnd_key(1);
            round(inv_rnd, b0, b1, kp);
        }
        kp = rnd_key(1);
        round(inv_rnd, b1, b0, kp);
#else
    {   uint_32t    rnd;
        for(rnd = 0; rnd < (cx->inf.b[0] >> 4) - 1; ++rnd)
        {
            kp = rnd_key(1);
            round(inv_rnd, b1, b0, kp);
            l_copy(b0, b1);
        }
#endif
        kp = rnd_key(1);
        round(inv_lrnd, b0, b1, kp);
        }
#endif

    state_out(out, b0);
    return EXIT_SUCCESS;
}

#endif

#if defined(__cplusplus)
}
#endif
