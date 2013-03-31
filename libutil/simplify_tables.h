#ifndef LIBUTIL_SIMPLIFY_TABLES_H
#define LIBUTIL_SIMPLIFY_TABLES_H 1

#include <stddef.h>

namespace util {

namespace simplify {

/* List of "simplified" characters. Note that for numeric sorting to work,
 * the digits must simplify to themselves. It doesn't matter what anything
 * else simplifies to, as long as it's in the right order.
 */
enum {
    /* Numerals */
    N_0 = '0',
    N_1,
    N_2,
    N_3,
    N_4,
    N_5,
    N_6,
    N_7,
    N_8,
    N_9,

    /* Latin */
    L_A,
    L_B,
    L_C,
    L_D,
    L_DZ,
    L_ETH,
    L_AE,
    L_OE,
    L_E,
    L_F,
    L_G,
    L_YOGH,
    L_H,
    L_I,
    L_J,
    L_K,
    L_L,
    L_LJ,
    L_M,
    L_N,
    L_O,
    L_P,
    L_Q,
    L_R,
    L_S,
    L_SS,
    L_T,
    L_U,
    L_V,
    L_W,
    L_X,
    L_Y,
    L_IJ,
    L_Z,
    L_EZH,
    L_THORN,

    /* Greek */
    GR_ALPHA,

    /* Cyrillic */
    CY_A
};

struct Table
{
    unsigned int from;
    unsigned int to;
    const unsigned char *table;
};

static const unsigned char SimplifyAscii[] = { /* 0x0030 -- 0x007F */
N_0, N_1, N_2, N_3, N_4, N_5, N_6, N_7, N_8, N_9, 000, 000, 000, 000, 000, 000,
000, L_A, L_B, L_C, L_D, L_E, L_F, L_G, L_H, L_I, L_J, L_K, L_L, L_M, L_N, L_O,
L_P, L_Q, L_R, L_S, L_T, L_U, L_V, L_W, L_X, L_Y, L_Z, 000, 000, 000, 000, 000,
000, L_A, L_B, L_C, L_D, L_E, L_F, L_G, L_H, L_I, L_J, L_K, L_L, L_M, L_N, L_O,
L_P, L_Q, L_R, L_S, L_T, L_U, L_V, L_W, L_X, L_Y, L_Z, 000, 000, 000, 000, 000
};

static const unsigned char SimplifyLatin1[] = { /* 0x00C0 -- 0x00FF */
L_A, L_A, L_A, L_A, L_A, L_A,L_AE, L_C, L_E, L_E, L_E, L_E, L_I, L_I, L_I, L_I,
L_ETH,L_N,L_O, L_O, L_O, L_O, L_O, 000, L_O, L_U, L_U,L_U,L_U,L_Y,L_THORN,L_SS,
L_A, L_A, L_A, L_A, L_A, L_A,L_AE, L_C, L_E, L_E, L_E, L_E, L_I, L_I, L_I, L_I,
L_ETH,L_N,L_O, L_O, L_O, L_O, L_O, 000, L_O, L_U, L_U, L_U,L_U,L_Y,L_THORN,L_Y,

/* 0x0100 -- 0x017F */

L_A, L_A, L_A, L_A, L_A, L_A, L_C, L_C, L_C, L_C, L_C, L_C, L_C, L_C, L_D, L_D,
L_D, L_D, L_E, L_E, L_E, L_E, L_E, L_E, L_E, L_E, L_E, L_E, L_G, L_G, L_G, L_G,
L_G, L_G, L_G, L_G, L_H, L_H, L_H, L_H, L_I, L_I, L_I, L_I, L_I, L_I, L_I, L_I,
L_I, L_I,L_IJ,L_IJ, L_J, L_J, L_K, L_K, L_Q, L_L, L_L, L_L, L_L, L_L, L_L, L_L,
L_L, L_L, L_L, L_N, L_N, L_N, L_N, L_N, L_N, L_N, L_N, L_N, L_O, L_O, L_O, L_O,
L_O, L_O,L_OE,L_OE, L_R, L_R, L_R, L_R, L_R, L_R, L_S, L_S, L_S, L_S, L_S, L_S,
L_S, L_S, L_T, L_T, L_T, L_T, L_T, L_T, L_U, L_U, L_U, L_U, L_U, L_U, L_U, L_U,
L_U, L_U, L_U, L_U, L_W, L_W, L_Y, L_Y, L_Y, L_Z, L_Z, L_Z, L_Z, L_Z, L_Z, L_S
};

const Table tables[] =
{
    { 0x0030, 0x0080, SimplifyAscii },
    { 0x00C0, 0x0180, SimplifyLatin1 },
    { 0, 0, NULL }
};

const unsigned int tablecount = unsigned(sizeof(tables)/sizeof(*tables));

} // namespace util::simplify

} // namespace util

#endif
