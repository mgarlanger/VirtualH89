/// \file z80.cpp
///
/// \brief Implements a virtual Zilog Z80 CPU.
///
/// \date Mar 7, 2009
///
/// \author Mark Garlanger
///

#include "z80.h"

#include <ctime>
#include <cassert>
#include <strings.h>
#include <unistd.h>

#include "logger.h"
#include "H89.h"
#include "WallClock.h"
#include "disasm.h"
#include "h89-io.h"
#include "propertyutil.h"

// typedef int (Z80::*opCodeMethod)(void);

#if PARITY_TABLE
/// Precalculated Parity table based on byte value.
///
const BYTE Z80::parity[256] =
{
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};
#endif

/// Precalculated flag values for Z, S, & P
///
/// Since 8 bit values are all we have to check, setting up a table
/// is much more efficient than recalculating these values
/// up to 500,000 times a sec (at 2Mhz).
const BYTE              Z80::ZSP[256] =
{
    0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84
};

const Z80::opCodeMethod Z80::op_code[256] =
{
    &Z80::op_nop,       /* 0x00 - 000 */
    &Z80::op_ld_xx_nn,  /* 0x01 - 001 */
    &Z80::op_ld_ibc_a,  /* 0x02 - 002 */
    &Z80::op_inc_xx,    /* 0x03 - 003 */
    &Z80::op_inc_x,     /* 0x04 - 004 */
    &Z80::op_dec_x,     /* 0x05 - 005 */
    &Z80::op_ld_x_n,    /* 0x06 - 006 */
    &Z80::op_rlc_a,     /* 0x07 - 007 */
    &Z80::op_ex_af_af,  /* 0x08 - 010 */
    &Z80::op_add_hl_xx, /* 0x09 - 011 */
    &Z80::op_ld_a_ibc,  /* 0x0a - 012 */
    &Z80::op_dec_xx,    /* 0x0b - 013 */
    &Z80::op_inc_x,     /* 0x0c - 014 */
    &Z80::op_dec_x,     /* 0x0d - 015 */
    &Z80::op_ld_x_n,    /* 0x0e - 016 */
    &Z80::op_rrc_a,     /* 0x0f - 017 */
    &Z80::op_djnz,      /* 0x10 - 020 */
    &Z80::op_ld_xx_nn,  /* 0x11 - 021 */
    &Z80::op_ld_ide_a,  /* 0x12 - 022 */
    &Z80::op_inc_xx,    /* 0x13 - 023 */
    &Z80::op_inc_x,     /* 0x14 - 024 */
    &Z80::op_dec_x,     /* 0x15 - 025 */
    &Z80::op_ld_x_n,    /* 0x16 - 026 */
    &Z80::op_rl_a,      /* 0x17 - 027 */
    &Z80::op_jr,        /* 0x18 - 030 */
    &Z80::op_add_hl_xx, /* 0x19 - 031 */
    &Z80::op_ld_a_ide,  /* 0x1a - 032 */
    &Z80::op_dec_xx,    /* 0x1b - 033 */
    &Z80::op_inc_x,     /* 0x1c - 034 */
    &Z80::op_dec_x,     /* 0x1d - 035 */
    &Z80::op_ld_x_n,    /* 0x1e - 036 */
    &Z80::op_rr_a,      /* 0x1f - 037 */
    &Z80::op_jr_nz,     /* 0x20 - 040 */
    &Z80::op_ld_xx_nn,  /* 0x21 - 041 */
    &Z80::op_ld_inn_xx, /* 0x22 - 042 */
    &Z80::op_inc_xx,    /* 0x23 - 043 */
    &Z80::op_inc_x,     /* 0x24 - 044 */
    &Z80::op_dec_x,     /* 0x25 - 045 */
    &Z80::op_ld_x_n,    /* 0x26 - 046 */
    &Z80::op_daa,       /* 0x27 - 047 */
    &Z80::op_jr_z,      /* 0x28 - 050 */
    &Z80::op_add_hl_xx, /* 0x29 - 051 */
    &Z80::op_ld_xx_inn, /* 0x2a - 052 */
    &Z80::op_dec_xx,    /* 0x2b - 053 */
    &Z80::op_inc_x,     /* 0x2c - 054 */
    &Z80::op_dec_x,     /* 0x2d - 055 */
    &Z80::op_ld_x_n,    /* 0x2e - 056 */
    &Z80::op_cpl,       /* 0x2f - 057 */
    &Z80::op_jr_nc,     /* 0x30 - 060 */
    &Z80::op_ld_xx_nn,  /* 0x31 - 061 */
    &Z80::op_ld_inn_a,  /* 0x32 - 062 */
    &Z80::op_inc_xx,    /* 0x33 - 063 */
    &Z80::op_inc_ihl,   /* 0x34 - 064 */
    &Z80::op_dec_ihl,   /* 0x35 - 065 */
    &Z80::op_ld_ihl_n,  /* 0x36 - 066 */
    &Z80::op_scf,       /* 0x37 - 067 */
    &Z80::op_jr_c,      /* 0x38 - 070 */
    &Z80::op_add_hl_xx, /* 0x39 - 071 */
    &Z80::op_ld_a_inn,  /* 0x3a - 072 */
    &Z80::op_dec_xx,    /* 0x3b - 073 */
    &Z80::op_inc_x,     /* 0x3c - 074 */
    &Z80::op_dec_x,     /* 0x3d - 075 */
    &Z80::op_ld_x_n,    /* 0x3e - 076 */
    &Z80::op_ccf,       /* 0x3f - 077 */
    &Z80::op_ld_x_x,    /* 0x40 - 100 */
    &Z80::op_ld_x_x,    /* 0x41 - 101 */
    &Z80::op_ld_x_x,    /* 0x42 - 102 */
    &Z80::op_ld_x_x,    /* 0x43 - 103 */
    &Z80::op_ld_x_x,    /* 0x44 - 104 */
    &Z80::op_ld_x_x,    /* 0x45 - 105 */
    &Z80::op_ld_x_ihl,  /* 0x46 - 106 */
    &Z80::op_ld_x_x,    /* 0x47 - 107 */
    &Z80::op_ld_x_x,    /* 0x48 - 110 */
    &Z80::op_ld_x_x,    /* 0x49 - 111 */
    &Z80::op_ld_x_x,    /* 0x4a - 112 */
    &Z80::op_ld_x_x,    /* 0x4b - 113 */
    &Z80::op_ld_x_x,    /* 0x4c - 114 */
    &Z80::op_ld_x_x,    /* 0x4d - 115 */
    &Z80::op_ld_x_ihl,  /* 0x4e - 116 */
    &Z80::op_ld_x_x,    /* 0x4f - 117 */
    &Z80::op_ld_x_x,    /* 0x50 - 120 */
    &Z80::op_ld_x_x,    /* 0x51 - 121 */
    &Z80::op_ld_x_x,    /* 0x52 - 122 */
    &Z80::op_ld_x_x,    /* 0x53 - 123 */
    &Z80::op_ld_x_x,    /* 0x54 - 124 */
    &Z80::op_ld_x_x,    /* 0x55 - 125 */
    &Z80::op_ld_x_ihl,  /* 0x56 - 126 */
    &Z80::op_ld_x_x,    /* 0x57 - 127 */
    &Z80::op_ld_x_x,    /* 0x58 - 130 */
    &Z80::op_ld_x_x,    /* 0x59 - 131 */
    &Z80::op_ld_x_x,    /* 0x5a - 132 */
    &Z80::op_ld_x_x,    /* 0x5b - 133 */
    &Z80::op_ld_x_x,    /* 0x5c - 134 */
    &Z80::op_ld_x_x,    /* 0x5d - 135 */
    &Z80::op_ld_x_ihl,  /* 0x5e - 136 */
    &Z80::op_ld_x_x,    /* 0x5f - 137 */
    &Z80::op_ld_x_x,    /* 0x60 - 140 */
    &Z80::op_ld_x_x,    /* 0x61 - 141 */
    &Z80::op_ld_x_x,    /* 0x62 - 142 */
    &Z80::op_ld_x_x,    /* 0x63 - 143 */
    &Z80::op_ld_x_x,    /* 0x64 - 144 */
    &Z80::op_ld_x_x,    /* 0x65 - 145 */
    &Z80::op_ld_x_ihl,  /* 0x66 - 146 */
    &Z80::op_ld_x_x,    /* 0x67 - 147 */
    &Z80::op_ld_x_x,    /* 0x68 - 150 */
    &Z80::op_ld_x_x,    /* 0x69 - 151 */
    &Z80::op_ld_x_x,    /* 0x6a - 152 */
    &Z80::op_ld_x_x,    /* 0x6b - 153 */
    &Z80::op_ld_x_x,    /* 0x6c - 154 */
    &Z80::op_ld_x_x,    /* 0x6d - 155 */
    &Z80::op_ld_x_ihl,  /* 0x6e - 156 */
    &Z80::op_ld_x_x,    /* 0x6f - 157 */
    &Z80::op_ld_ihl_x,  /* 0x70 - 160 */
    &Z80::op_ld_ihl_x,  /* 0x71 - 161 */
    &Z80::op_ld_ihl_x,  /* 0x72 - 162 */
    &Z80::op_ld_ihl_x,  /* 0x73 - 163 */
    &Z80::op_ld_ihl_x,  /* 0x74 - 164 */
    &Z80::op_ld_ihl_x,  /* 0x75 - 165 */
    &Z80::op_halt,      /* 0x76 - 166 */
    &Z80::op_ld_ihl_x,  /* 0x77 - 167 */
    &Z80::op_ld_x_x,    /* 0x78 - 170 */
    &Z80::op_ld_x_x,    /* 0x79 - 171 */
    &Z80::op_ld_x_x,    /* 0x7a - 172 */
    &Z80::op_ld_x_x,    /* 0x7b - 173 */
    &Z80::op_ld_x_x,    /* 0x7c - 174 */
    &Z80::op_ld_x_x,    /* 0x7d - 175 */
    &Z80::op_ld_x_ihl,  /* 0x7e - 176 */
    &Z80::op_ld_x_x,    /* 0x7f - 177 */
    &Z80::op_add_x,     /* 0x80 - 200 */
    &Z80::op_add_x,     /* 0x81 - 201 */
    &Z80::op_add_x,     /* 0x82 - 202 */
    &Z80::op_add_x,     /* 0x83 - 203 */
    &Z80::op_add_x,     /* 0x84 - 204 */
    &Z80::op_add_x,     /* 0x85 - 205 */
    &Z80::op_add_ihl,   /* 0x86 - 206 */
    &Z80::op_add_x,     /* 0x87 - 207 */
    &Z80::op_adc_x,     /* 0x88 - 210 */
    &Z80::op_adc_x,     /* 0x89 - 211 */
    &Z80::op_adc_x,     /* 0x8a - 212 */
    &Z80::op_adc_x,     /* 0x8b - 213 */
    &Z80::op_adc_x,     /* 0x8c - 214 */
    &Z80::op_adc_x,     /* 0x8d - 215 */
    &Z80::op_adc_ihl,   /* 0x8e - 216 */
    &Z80::op_adc_x,     /* 0x8f - 217 */
    &Z80::op_sub_x,     /* 0x90 - 220 */
    &Z80::op_sub_x,     /* 0x91 - 221 */
    &Z80::op_sub_x,     /* 0x92 - 222 */
    &Z80::op_sub_x,     /* 0x93 - 223 */
    &Z80::op_sub_x,     /* 0x94 - 224 */
    &Z80::op_sub_x,     /* 0x95 - 225 */
    &Z80::op_sub_ihl,   /* 0x96 - 226 */
    &Z80::op_sub_x,     /* 0x97 - 227 */
    &Z80::op_sbc_x,     /* 0x98 - 230 */
    &Z80::op_sbc_x,     /* 0x99 - 231 */
    &Z80::op_sbc_x,     /* 0x9a - 232 */
    &Z80::op_sbc_x,     /* 0x9b - 233 */
    &Z80::op_sbc_x,     /* 0x9c - 234 */
    &Z80::op_sbc_x,     /* 0x9d - 235 */
    &Z80::op_sbc_ihl,   /* 0x9e - 236 */
    &Z80::op_sbc_x,     /* 0x9f - 237 */
    &Z80::op_and_x,     /* 0xa0 - 240 */
    &Z80::op_and_x,     /* 0xa1 - 241 */
    &Z80::op_and_x,     /* 0xa2 - 242 */
    &Z80::op_and_x,     /* 0xa3 - 243 */
    &Z80::op_and_x,     /* 0xa4 - 244 */
    &Z80::op_and_x,     /* 0xa5 - 245 */
    &Z80::op_and_ihl,   /* 0xa6 - 246 */
    &Z80::op_and_x,     /* 0xa7 - 247 */
    &Z80::op_xor_x,     /* 0xa8 - 250 */
    &Z80::op_xor_x,     /* 0xa9 - 251 */
    &Z80::op_xor_x,     /* 0xaa - 252 */
    &Z80::op_xor_x,     /* 0xab - 253 */
    &Z80::op_xor_x,     /* 0xac - 254 */
    &Z80::op_xor_x,     /* 0xad - 255 */
    &Z80::op_xor_ihl,   /* 0xae - 256 */
    &Z80::op_xor_x,     /* 0xaf - 257 */
    &Z80::op_or_x,      /* 0xb0 - 260 */
    &Z80::op_or_x,      /* 0xb1 - 261 */
    &Z80::op_or_x,      /* 0xb2 - 262 */
    &Z80::op_or_x,      /* 0xb3 - 263 */
    &Z80::op_or_x,      /* 0xb4 - 264 */
    &Z80::op_or_x,      /* 0xb5 - 265 */
    &Z80::op_or_ihl,    /* 0xb6 - 266 */
    &Z80::op_or_x,      /* 0xb7 - 267 */
    &Z80::op_cp_x,      /* 0xb8 - 270 */
    &Z80::op_cp_x,      /* 0xb9 - 271 */
    &Z80::op_cp_x,      /* 0xba - 272 */
    &Z80::op_cp_x,      /* 0xbb - 273 */
    &Z80::op_cp_x,      /* 0xbc - 274 */
    &Z80::op_cp_x,      /* 0xbd - 275 */
    &Z80::op_cp_ihl,    /* 0xbe - 276 */
    &Z80::op_cp_x,      /* 0xbf - 277 */
    &Z80::op_ret_cc,    /* 0xc0 - 300 */
    &Z80::op_pop_xx,    /* 0xc1 - 301 */
    &Z80::op_jp_cc,     /* 0xc2 - 302 */
    &Z80::op_jp,        /* 0xc3 - 303 */
    &Z80::op_call_cc,   /* 0xc4 - 304 */
    &Z80::op_push_xx,   /* 0xc5 - 305 */
    &Z80::op_add_n,     /* 0xc6 - 306 */
    &Z80::op_rst,       /* 0xc7 - 307 */
    &Z80::op_ret_cc,    /* 0xc8 - 310 */
    &Z80::op_ret,       /* 0xc9 - 311 */
    &Z80::op_jp_cc,     /* 0xca - 312 */
    &Z80::op_cb_handle, /* 0xcb - 313 - prefix*/
    &Z80::op_call_cc,   /* 0xcc - 314 */
    &Z80::op_call,      /* 0xcd - 315 */
    &Z80::op_adc_n,     /* 0xce - 316 */
    &Z80::op_rst,       /* 0xcf - 317 */
    &Z80::op_ret_cc,    /* 0xd0 - 320 */
    &Z80::op_pop_xx,    /* 0xd1 - 321 */
    &Z80::op_jp_cc,     /* 0xd2 - 322 */
    &Z80::op_out,       /* 0xd3 - 323 */
    &Z80::op_call_cc,   /* 0xd4 - 324 */
    &Z80::op_push_xx,   /* 0xd5 - 325 */
    &Z80::op_sub_n,     /* 0xd6 - 326 */
    &Z80::op_rst,       /* 0xd7 - 327 */
    &Z80::op_ret_cc,    /* 0xd8 - 330 */
    &Z80::op_exx,       /* 0xd9 - 331 */
    &Z80::op_jp_cc,     /* 0xda - 332 */
    &Z80::op_in,        /* 0xdb - 333 */
    &Z80::op_call_cc,   /* 0xdc - 334 */
    &Z80::op_dd_handle, /* 0xdd - 335 - prefix */
    &Z80::op_sbc_n,     /* 0xde - 336 */
    &Z80::op_rst,       /* 0xdf - 337 */
    &Z80::op_ret_cc,    /* 0xe0 - 340 */
    &Z80::op_pop_xx,    /* 0xe1 - 341 */
    &Z80::op_jp_cc,     /* 0xe2 - 342 */
    &Z80::op_ex_isp_hl, /* 0xe3 - 343 */
    &Z80::op_call_cc,   /* 0xe4 - 344 */
    &Z80::op_push_xx,   /* 0xe5 - 345 */
    &Z80::op_and_n,     /* 0xe6 - 346 */
    &Z80::op_rst,       /* 0xe7 - 347 */
    &Z80::op_ret_cc,    /* 0xe8 - 350 */
    &Z80::op_jp_hl,     /* 0xe9 - 351 */
    &Z80::op_jp_cc,     /* 0xea - 352 */
    &Z80::op_ex_de_hl,  /* 0xeb - 353 */
    &Z80::op_call_cc,   /* 0xec - 354 */
    &Z80::op_ed_handle, /* 0xed - 355 - prefix*/
    &Z80::op_xor_n,     /* 0xee - 356 */
    &Z80::op_rst,       /* 0xef - 357 */
    &Z80::op_ret_cc,    /* 0xf0 - 360 */
    &Z80::op_pop_xx,    /* 0xf1 - 361 */
    &Z80::op_jp_cc,     /* 0xf2 - 362 */
    &Z80::op_di,        /* 0xf3 - 363 */
    &Z80::op_call_cc,   /* 0xf4 - 364 */
    &Z80::op_push_xx,   /* 0xf5 - 365 */
    &Z80::op_or_n,      /* 0xf6 - 366 */
    &Z80::op_rst,       /* 0xf7 - 367 */
    &Z80::op_ret_cc,    /* 0xf8 - 370 */
    &Z80::op_ld_sp_hl,  /* 0xf9 - 371 */
    &Z80::op_jp_cc,     /* 0xfa - 372 */
    &Z80::op_ei,        /* 0xfb - 373 */
    &Z80::op_call_cc,   /* 0xfc - 374 */
    &Z80::op_fd_handle, /* 0xfd - 375 - prefix */
    &Z80::op_cp_n,      /* 0xfe - 376 */
    &Z80::op_rst        /* 0xff - 377 */
};

const Z80::opCodeMethod Z80::op_cb[256] =
{
    &Z80::op_rlc_x,    /* 0x00 */
    &Z80::op_rlc_x,    /* 0x01 */
    &Z80::op_rlc_x,    /* 0x02 */
    &Z80::op_rlc_x,    /* 0x03 */
    &Z80::op_rlc_x,    /* 0x04 */
    &Z80::op_rlc_x,    /* 0x05 */
    &Z80::op_rlc_ihl,  /* 0x06 */
    &Z80::op_rlc_x,    /* 0x07 */
    &Z80::op_rrc_x,    /* 0x08 */
    &Z80::op_rrc_x,    /* 0x09 */
    &Z80::op_rrc_x,    /* 0x0a */
    &Z80::op_rrc_x,    /* 0x0b */
    &Z80::op_rrc_x,    /* 0x0c */
    &Z80::op_rrc_x,    /* 0x0d */
    &Z80::op_rrc_ihl,  /* 0x0e */
    &Z80::op_rrc_x,    /* 0x0f */
    &Z80::op_rl_x,     /* 0x10 */
    &Z80::op_rl_x,     /* 0x11 */
    &Z80::op_rl_x,     /* 0x12 */
    &Z80::op_rl_x,     /* 0x13 */
    &Z80::op_rl_x,     /* 0x14 */
    &Z80::op_rl_x,     /* 0x15 */
    &Z80::op_rl_ihl,   /* 0x16 */
    &Z80::op_rl_x,     /* 0x17 */
    &Z80::op_rr_x,     /* 0x18 */
    &Z80::op_rr_x,     /* 0x19 */
    &Z80::op_rr_x,     /* 0x1a */
    &Z80::op_rr_x,     /* 0x1b */
    &Z80::op_rr_x,     /* 0x1c */
    &Z80::op_rr_x,     /* 0x1d */
    &Z80::op_rr_ihl,   /* 0x1e */
    &Z80::op_rr_x,     /* 0x1f */
    &Z80::op_sla_x,    /* 0x20 */
    &Z80::op_sla_x,    /* 0x21 */
    &Z80::op_sla_x,    /* 0x22 */
    &Z80::op_sla_x,    /* 0x23 */
    &Z80::op_sla_x,    /* 0x24 */
    &Z80::op_sla_x,    /* 0x25 */
    &Z80::op_sla_ihl,  /* 0x26 */
    &Z80::op_sla_x,    /* 0x27 */
    &Z80::op_sra_x,    /* 0x28 */
    &Z80::op_sra_x,    /* 0x29 */
    &Z80::op_sra_x,    /* 0x2a */
    &Z80::op_sra_x,    /* 0x2b */
    &Z80::op_sra_x,    /* 0x2c */
    &Z80::op_sra_x,    /* 0x2d */
    &Z80::op_sra_ihl,  /* 0x2e */
    &Z80::op_sra_x,    /* 0x2f */
    &Z80::op_sll_x,    /* 0x30 - undoc */
    &Z80::op_sll_x,    /* 0x31 - undoc */
    &Z80::op_sll_x,    /* 0x32 - undoc */
    &Z80::op_sll_x,    /* 0x33 - undoc */
    &Z80::op_sll_x,    /* 0x34 - undoc */
    &Z80::op_sll_x,    /* 0x35 - undoc */
    &Z80::op_sll_ihl,  /* 0x36 - undoc */
    &Z80::op_sll_x,    /* 0x37 - undoc */
    &Z80::op_srl_x,    /* 0x38 */
    &Z80::op_srl_x,    /* 0x39 */
    &Z80::op_srl_x,    /* 0x3a */
    &Z80::op_srl_x,    /* 0x3b */
    &Z80::op_srl_x,    /* 0x3c */
    &Z80::op_srl_x,    /* 0x3d */
    &Z80::op_srl_ihl,  /* 0x3e */
    &Z80::op_srl_x,    /* 0x3f */
    &Z80::op_tb_n_x,   /* 0x40 */
    &Z80::op_tb_n_x,   /* 0x41 */
    &Z80::op_tb_n_x,   /* 0x42 */
    &Z80::op_tb_n_x,   /* 0x43 */
    &Z80::op_tb_n_x,   /* 0x44 */
    &Z80::op_tb_n_x,   /* 0x45 */
    &Z80::op_tb_n_ihl, /* 0x46 */
    &Z80::op_tb_n_x,   /* 0x47 */
    &Z80::op_tb_n_x,   /* 0x48 */
    &Z80::op_tb_n_x,   /* 0x49 */
    &Z80::op_tb_n_x,   /* 0x4a */
    &Z80::op_tb_n_x,   /* 0x4b */
    &Z80::op_tb_n_x,   /* 0x4c */
    &Z80::op_tb_n_x,   /* 0x4d */
    &Z80::op_tb_n_ihl, /* 0x4e */
    &Z80::op_tb_n_x,   /* 0x4f */
    &Z80::op_tb_n_x,   /* 0x50 */
    &Z80::op_tb_n_x,   /* 0x51 */
    &Z80::op_tb_n_x,   /* 0x52 */
    &Z80::op_tb_n_x,   /* 0x53 */
    &Z80::op_tb_n_x,   /* 0x54 */
    &Z80::op_tb_n_x,   /* 0x55 */
    &Z80::op_tb_n_ihl, /* 0x56 */
    &Z80::op_tb_n_x,   /* 0x57 */
    &Z80::op_tb_n_x,   /* 0x58 */
    &Z80::op_tb_n_x,   /* 0x59 */
    &Z80::op_tb_n_x,   /* 0x5a */
    &Z80::op_tb_n_x,   /* 0x5b */
    &Z80::op_tb_n_x,   /* 0x5c */
    &Z80::op_tb_n_x,   /* 0x5d */
    &Z80::op_tb_n_ihl, /* 0x5e */
    &Z80::op_tb_n_x,   /* 0x5f */
    &Z80::op_tb_n_x,   /* 0x60 */
    &Z80::op_tb_n_x,   /* 0x61 */
    &Z80::op_tb_n_x,   /* 0x62 */
    &Z80::op_tb_n_x,   /* 0x63 */
    &Z80::op_tb_n_x,   /* 0x64 */
    &Z80::op_tb_n_x,   /* 0x65 */
    &Z80::op_tb_n_ihl, /* 0x66 */
    &Z80::op_tb_n_x,   /* 0x67 */
    &Z80::op_tb_n_x,   /* 0x68 */
    &Z80::op_tb_n_x,   /* 0x69 */
    &Z80::op_tb_n_x,   /* 0x6a */
    &Z80::op_tb_n_x,   /* 0x6b */
    &Z80::op_tb_n_x,   /* 0x6c */
    &Z80::op_tb_n_x,   /* 0x6d */
    &Z80::op_tb_n_ihl, /* 0x6e */
    &Z80::op_tb_n_x,   /* 0x6f */
    &Z80::op_tb_n_x,   /* 0x70 */
    &Z80::op_tb_n_x,   /* 0x71 */
    &Z80::op_tb_n_x,   /* 0x72 */
    &Z80::op_tb_n_x,   /* 0x73 */
    &Z80::op_tb_n_x,   /* 0x74 */
    &Z80::op_tb_n_x,   /* 0x75 */
    &Z80::op_tb_n_ihl, /* 0x76 */
    &Z80::op_tb_n_x,   /* 0x77 */
    &Z80::op_tb_n_x,   /* 0x78 */
    &Z80::op_tb_n_x,   /* 0x79 */
    &Z80::op_tb_n_x,   /* 0x7a */
    &Z80::op_tb_n_x,   /* 0x7b */
    &Z80::op_tb_n_x,   /* 0x7c */
    &Z80::op_tb_n_x,   /* 0x7d */
    &Z80::op_tb_n_ihl, /* 0x7e */
    &Z80::op_tb_n_x,   /* 0x7f */
    &Z80::op_rb_n_x,   /* 0x80 */
    &Z80::op_rb_n_x,   /* 0x81 */
    &Z80::op_rb_n_x,   /* 0x82 */
    &Z80::op_rb_n_x,   /* 0x83 */
    &Z80::op_rb_n_x,   /* 0x84 */
    &Z80::op_rb_n_x,   /* 0x85 */
    &Z80::op_rb_n_ihl, /* 0x86 */
    &Z80::op_rb_n_x,   /* 0x87 */
    &Z80::op_rb_n_x,   /* 0x88 */
    &Z80::op_rb_n_x,   /* 0x89 */
    &Z80::op_rb_n_x,   /* 0x8a */
    &Z80::op_rb_n_x,   /* 0x8b */
    &Z80::op_rb_n_x,   /* 0x8c */
    &Z80::op_rb_n_x,   /* 0x8d */
    &Z80::op_rb_n_ihl, /* 0x8e */
    &Z80::op_rb_n_x,   /* 0x8f */
    &Z80::op_rb_n_x,   /* 0x90 */
    &Z80::op_rb_n_x,   /* 0x91 */
    &Z80::op_rb_n_x,   /* 0x92 */
    &Z80::op_rb_n_x,   /* 0x93 */
    &Z80::op_rb_n_x,   /* 0x94 */
    &Z80::op_rb_n_x,   /* 0x95 */
    &Z80::op_rb_n_ihl, /* 0x96 */
    &Z80::op_rb_n_x,   /* 0x97 */
    &Z80::op_rb_n_x,   /* 0x98 */
    &Z80::op_rb_n_x,   /* 0x99 */
    &Z80::op_rb_n_x,   /* 0x9a */
    &Z80::op_rb_n_x,   /* 0x9b */
    &Z80::op_rb_n_x,   /* 0x9c */
    &Z80::op_rb_n_x,   /* 0x9d */
    &Z80::op_rb_n_ihl, /* 0x9e */
    &Z80::op_rb_n_x,   /* 0x9f */
    &Z80::op_rb_n_x,   /* 0xa0 */
    &Z80::op_rb_n_x,   /* 0xa1 */
    &Z80::op_rb_n_x,   /* 0xa2 */
    &Z80::op_rb_n_x,   /* 0xa3 */
    &Z80::op_rb_n_x,   /* 0xa4 */
    &Z80::op_rb_n_x,   /* 0xa5 */
    &Z80::op_rb_n_ihl, /* 0xa6 */
    &Z80::op_rb_n_x,   /* 0xa7 */
    &Z80::op_rb_n_x,   /* 0xa8 */
    &Z80::op_rb_n_x,   /* 0xa9 */
    &Z80::op_rb_n_x,   /* 0xaa */
    &Z80::op_rb_n_x,   /* 0xab */
    &Z80::op_rb_n_x,   /* 0xac */
    &Z80::op_rb_n_x,   /* 0xad */
    &Z80::op_rb_n_ihl, /* 0xae */
    &Z80::op_rb_n_x,   /* 0xaf */
    &Z80::op_rb_n_x,   /* 0xb0 */
    &Z80::op_rb_n_x,   /* 0xb1 */
    &Z80::op_rb_n_x,   /* 0xb2 */
    &Z80::op_rb_n_x,   /* 0xb3 */
    &Z80::op_rb_n_x,   /* 0xb4 */
    &Z80::op_rb_n_x,   /* 0xb5 */
    &Z80::op_rb_n_ihl, /* 0xb6 */
    &Z80::op_rb_n_x,   /* 0xb7 */
    &Z80::op_rb_n_x,   /* 0xb8 */
    &Z80::op_rb_n_x,   /* 0xb9 */
    &Z80::op_rb_n_x,   /* 0xba */
    &Z80::op_rb_n_x,   /* 0xbb */
    &Z80::op_rb_n_x,   /* 0xbc */
    &Z80::op_rb_n_x,   /* 0xbd */
    &Z80::op_rb_n_ihl, /* 0xbe */
    &Z80::op_rb_n_x,   /* 0xbf */
    &Z80::op_sb_n_x,   /* 0xc0 */
    &Z80::op_sb_n_x,   /* 0xc1 */
    &Z80::op_sb_n_x,   /* 0xc2 */
    &Z80::op_sb_n_x,   /* 0xc3 */
    &Z80::op_sb_n_x,   /* 0xc4 */
    &Z80::op_sb_n_x,   /* 0xc5 */
    &Z80::op_sb_n_ihl, /* 0xc6 */
    &Z80::op_sb_n_x,   /* 0xc7 */
    &Z80::op_sb_n_x,   /* 0xc8 */
    &Z80::op_sb_n_x,   /* 0xc9 */
    &Z80::op_sb_n_x,   /* 0xca */
    &Z80::op_sb_n_x,   /* 0xcb */
    &Z80::op_sb_n_x,   /* 0xcc */
    &Z80::op_sb_n_x,   /* 0xcd */
    &Z80::op_sb_n_ihl, /* 0xce */
    &Z80::op_sb_n_x,   /* 0xcf */
    &Z80::op_sb_n_x,   /* 0xd0 */
    &Z80::op_sb_n_x,   /* 0xd1 */
    &Z80::op_sb_n_x,   /* 0xd2 */
    &Z80::op_sb_n_x,   /* 0xd3 */
    &Z80::op_sb_n_x,   /* 0xd4 */
    &Z80::op_sb_n_x,   /* 0xd5 */
    &Z80::op_sb_n_ihl, /* 0xd6 */
    &Z80::op_sb_n_x,   /* 0xd7 */
    &Z80::op_sb_n_x,   /* 0xd8 */
    &Z80::op_sb_n_x,   /* 0xd9 */
    &Z80::op_sb_n_x,   /* 0xda */
    &Z80::op_sb_n_x,   /* 0xdb */
    &Z80::op_sb_n_x,   /* 0xdc */
    &Z80::op_sb_n_x,   /* 0xdd */
    &Z80::op_sb_n_ihl, /* 0xde */
    &Z80::op_sb_n_x,   /* 0xdf */
    &Z80::op_sb_n_x,   /* 0xe0 */
    &Z80::op_sb_n_x,   /* 0xe1 */
    &Z80::op_sb_n_x,   /* 0xe2 */
    &Z80::op_sb_n_x,   /* 0xe3 */
    &Z80::op_sb_n_x,   /* 0xe4 */
    &Z80::op_sb_n_x,   /* 0xe5 */
    &Z80::op_sb_n_ihl, /* 0xe6 */
    &Z80::op_sb_n_x,   /* 0xe7 */
    &Z80::op_sb_n_x,   /* 0xe8 */
    &Z80::op_sb_n_x,   /* 0xe9 */
    &Z80::op_sb_n_x,   /* 0xea */
    &Z80::op_sb_n_x,   /* 0xeb */
    &Z80::op_sb_n_x,   /* 0xec */
    &Z80::op_sb_n_x,   /* 0xed */
    &Z80::op_sb_n_ihl, /* 0xee */
    &Z80::op_sb_n_x,   /* 0xef */
    &Z80::op_sb_n_x,   /* 0xf0 */
    &Z80::op_sb_n_x,   /* 0xf1 */
    &Z80::op_sb_n_x,   /* 0xf2 */
    &Z80::op_sb_n_x,   /* 0xf3 */
    &Z80::op_sb_n_x,   /* 0xf4 */
    &Z80::op_sb_n_x,   /* 0xf5 */
    &Z80::op_sb_n_ihl, /* 0xf6 */
    &Z80::op_sb_n_x,   /* 0xf7 */
    &Z80::op_sb_n_x,   /* 0xf8 */
    &Z80::op_sb_n_x,   /* 0xf9 */
    &Z80::op_sb_n_x,   /* 0xfa */
    &Z80::op_sb_n_x,   /* 0xfb */
    &Z80::op_sb_n_x,   /* 0xfc */
    &Z80::op_sb_n_x,   /* 0xfd */
    &Z80::op_sb_n_ihl, /* 0xfe */
    &Z80::op_sb_n_x    /* 0xff */
};


const Z80::opCodeMethod Z80::op_ed[256] =
{
    &Z80::op_ed_nop,    /* 0x00 */
    &Z80::op_ed_nop,    /* 0x01 */
    &Z80::op_ed_nop,    /* 0x02 */
    &Z80::op_ed_nop,    /* 0x03 */
    &Z80::op_ed_nop,    /* 0x04 */
    &Z80::op_ed_nop,    /* 0x05 */
    &Z80::op_ed_nop,    /* 0x06 */
    &Z80::op_ed_nop,    /* 0x07 */
    &Z80::op_ed_nop,    /* 0x08 */
    &Z80::op_ed_nop,    /* 0x09 */
    &Z80::op_ed_nop,    /* 0x0a */
    &Z80::op_ed_nop,    /* 0x0b */
    &Z80::op_ed_nop,    /* 0x0c */
    &Z80::op_ed_nop,    /* 0x0d */
    &Z80::op_ed_nop,    /* 0x0e */
    &Z80::op_ed_nop,    /* 0x0f */
    &Z80::op_ed_nop,    /* 0x10 */
    &Z80::op_ed_nop,    /* 0x11 */
    &Z80::op_ed_nop,    /* 0x12 */
    &Z80::op_ed_nop,    /* 0x13 */
    &Z80::op_ed_nop,    /* 0x14 */
    &Z80::op_ed_nop,    /* 0x15 */
    &Z80::op_ed_nop,    /* 0x16 */
    &Z80::op_ed_nop,    /* 0x17 */
    &Z80::op_ed_nop,    /* 0x18 */
    &Z80::op_ed_nop,    /* 0x19 */
    &Z80::op_ed_nop,    /* 0x1a */
    &Z80::op_ed_nop,    /* 0x1b */
    &Z80::op_ed_nop,    /* 0x1c */
    &Z80::op_ed_nop,    /* 0x1d */
    &Z80::op_ed_nop,    /* 0x1e */
    &Z80::op_ed_nop,    /* 0x1f */
    &Z80::op_ed_nop,    /* 0x20 */
    &Z80::op_ed_nop,    /* 0x21 */
    &Z80::op_ed_nop,    /* 0x22 */
    &Z80::op_ed_nop,    /* 0x23 */
    &Z80::op_ed_nop,    /* 0x24 */
    &Z80::op_ed_nop,    /* 0x25 */
    &Z80::op_ed_nop,    /* 0x26 */
    &Z80::op_ed_nop,    /* 0x27 */
    &Z80::op_ed_nop,    /* 0x28 */
    &Z80::op_ed_nop,    /* 0x29 */
    &Z80::op_ed_nop,    /* 0x2a */
    &Z80::op_ed_nop,    /* 0x2b */
    &Z80::op_ed_nop,    /* 0x2c */
    &Z80::op_ed_nop,    /* 0x2d */
    &Z80::op_ed_nop,    /* 0x2e */
    &Z80::op_ed_nop,    /* 0x2f */
    &Z80::op_ed_nop,    /* 0x30 */
    &Z80::op_ed_nop,    /* 0x31 */
    &Z80::op_ed_nop,    /* 0x32 */
    &Z80::op_ed_nop,    /* 0x33 */
    &Z80::op_ed_nop,    /* 0x34 */
    &Z80::op_ed_nop,    /* 0x35 */
    &Z80::op_ed_nop,    /* 0x36 */
    &Z80::op_ed_nop,    /* 0x37 */
    &Z80::op_ed_nop,    /* 0x38 */
    &Z80::op_ed_nop,    /* 0x39 */
    &Z80::op_ed_nop,    /* 0x3a */
    &Z80::op_ed_nop,    /* 0x3b */
    &Z80::op_ed_nop,    /* 0x3c */
    &Z80::op_ed_nop,    /* 0x3d */
    &Z80::op_ed_nop,    /* 0x3e */
    &Z80::op_ed_nop,    /* 0x3f */
    &Z80::op_in_x_c,    /* 0x40 -- */
    &Z80::op_out_c_x,   /* 0x41 -- */
    &Z80::op_sbc_hl_xx, /* 0x42 -- */
    &Z80::op_ld_inn_xx, /* 0x43 -- */
    &Z80::op_neg,       /* 0x44 -- */
    &Z80::op_retn,      /* 0x45 -- */
    &Z80::op_im0,       /* 0x46 -- */
    &Z80::op_ld_i_a,    /* 0x47 -- */
    &Z80::op_in_x_c,    /* 0x48 -- */
    &Z80::op_out_c_x,   /* 0x49 -- */
    &Z80::op_adc_hl_xx, /* 0x4a -- */
    &Z80::op_ld_xx_inn, /* 0x4b -- */
    &Z80::op_neg,       /* 0x4c undoc */
    &Z80::op_reti,      /* 0x4d -- */
    &Z80::op_im0,       /* 0x4e undoc */
    &Z80::op_ld_r_a,    /* 0x4f -- */
    &Z80::op_in_x_c,    /* 0x50 -- */
    &Z80::op_out_c_x,   /* 0x51 -- */
    &Z80::op_sbc_hl_xx, /* 0x52 -- */
    &Z80::op_ld_inn_xx, /* 0x53 -- */
    &Z80::op_neg,       /* 0x54 undoc */
    &Z80::op_retn,      /* 0x55 undoc */
    &Z80::op_im1,       /* 0x56 -- */
    &Z80::op_ld_a_i,    /* 0x57 -- */
    &Z80::op_in_x_c,    /* 0x58 -- */
    &Z80::op_out_c_x,   /* 0x59 -- */
    &Z80::op_adc_hl_xx, /* 0x5a -- */
    &Z80::op_ld_xx_inn, /* 0x5b -- */
    &Z80::op_neg,       /* 0x5c undoc */
    &Z80::op_retn,      /* 0x5d undoc */
    &Z80::op_im2,       /* 0x5e -- */
    &Z80::op_ld_a_r,    /* 0x5f -- */
    &Z80::op_in_x_c,    /* 0x60 -- */
    &Z80::op_out_c_x,   /* 0x61 -- */
    &Z80::op_sbc_hl_xx, /* 0x62 -- */
    &Z80::op_ld_inn_xx, /* 0x63 -- */
    &Z80::op_neg,       /* 0x64 undoc */
    &Z80::op_retn,      /* 0x65 undoc */
    &Z80::op_im0,       /* 0x66 undoc */
    &Z80::op_rrd_ihl,   /* 0x67 -- */
    &Z80::op_in_x_c,    /* 0x68 -- */
    &Z80::op_out_c_x,   /* 0x69 -- */
    &Z80::op_adc_hl_xx, /* 0x6a -- */
    &Z80::op_ld_xx_inn, /* 0x6b -- */
    &Z80::op_neg,       /* 0x6c undoc */
    &Z80::op_retn,      /* 0x6d undoc */
    &Z80::op_im0,       /* 0x6e undoc */
    &Z80::op_rld_ihl,   /* 0x6f -- */
    &Z80::op_in_f_ic,   /* 0x70 -- */
    &Z80::op_out_c_0,   /* 0x71 undoc */
    &Z80::op_sbc_hl_xx, /* 0x72 -- */
    &Z80::op_ld_inn_xx, /* 0x73 -- */
    &Z80::op_neg,       /* 0x74 undoc */
    &Z80::op_retn,      /* 0x75 undoc */
    &Z80::op_im1,       /* 0x76 undoc */
    &Z80::op_ed_nop,    /* 0x77 undoc */
    &Z80::op_in_x_c,    /* 0x78 -- */
    &Z80::op_out_c_x,   /* 0x79 -- */
    &Z80::op_adc_hl_xx, /* 0x7a -- */
    &Z80::op_ld_xx_inn, /* 0x7b -- */
    &Z80::op_neg,       /* 0x7c undoc */
    &Z80::op_retn,      /* 0x7d undoc */
    &Z80::op_im2,       /* 0x7e undoc */
    &Z80::op_ed_nop,    /* 0x7f undoc */
    &Z80::op_ed_nop,    /* 0x80 */
    &Z80::op_ed_nop,    /* 0x81 */
    &Z80::op_ed_nop,    /* 0x82 */
    &Z80::op_ed_nop,    /* 0x83 */
    &Z80::op_ed_nop,    /* 0x84 */
    &Z80::op_ed_nop,    /* 0x85 */
    &Z80::op_ed_nop,    /* 0x86 */
    &Z80::op_ed_nop,    /* 0x87 */
    &Z80::op_ed_nop,    /* 0x88 */
    &Z80::op_ed_nop,    /* 0x89 */
    &Z80::op_ed_nop,    /* 0x8a */
    &Z80::op_ed_nop,    /* 0x8b */
    &Z80::op_ed_nop,    /* 0x8c */
    &Z80::op_ed_nop,    /* 0x8d */
    &Z80::op_ed_nop,    /* 0x8e */
    &Z80::op_ed_nop,    /* 0x8f */
    &Z80::op_ed_nop,    /* 0x90 */
    &Z80::op_ed_nop,    /* 0x91 */
    &Z80::op_ed_nop,    /* 0x92 */
    &Z80::op_ed_nop,    /* 0x93 */
    &Z80::op_ed_nop,    /* 0x94 */
    &Z80::op_ed_nop,    /* 0x95 */
    &Z80::op_ed_nop,    /* 0x96 */
    &Z80::op_ed_nop,    /* 0x97 */
    &Z80::op_ed_nop,    /* 0x98 */
    &Z80::op_ed_nop,    /* 0x99 */
    &Z80::op_ed_nop,    /* 0x9a */
    &Z80::op_ed_nop,    /* 0x9b */
    &Z80::op_ed_nop,    /* 0x9c */
    &Z80::op_ed_nop,    /* 0x9d */
    &Z80::op_ed_nop,    /* 0x9e */
    &Z80::op_ed_nop,    /* 0x9f */
    &Z80::op_ldi,       /* 0xa0 -- */
    &Z80::op_cpi,       /* 0xa1 -- */
    &Z80::op_ini,       /* 0xa2 -- */
    &Z80::op_outi,      /* 0xa3 -- */
    &Z80::op_ed_nop,    /* 0xa4 */
    &Z80::op_ed_nop,    /* 0xa5 */
    &Z80::op_ed_nop,    /* 0xa6 */
    &Z80::op_ed_nop,    /* 0xa7 */
    &Z80::op_ldd,       /* 0xa8 -- */
    &Z80::op_cpd,       /* 0xa9 -- */
    &Z80::op_ind,       /* 0xaa -- */
    &Z80::op_outd,      /* 0xab -- */
    &Z80::op_ed_nop,    /* 0xac */
    &Z80::op_ed_nop,    /* 0xad */
    &Z80::op_ed_nop,    /* 0xae */
    &Z80::op_ed_nop,    /* 0xaf */
    &Z80::op_ldir,      /* 0xb0 -- */
    &Z80::op_cpir,      /* 0xb1 -- */
    &Z80::op_inir,      /* 0xb2 -- */
    &Z80::op_otir,      /* 0xb3 -- */
    &Z80::op_ed_nop,    /* 0xb4 */
    &Z80::op_ed_nop,    /* 0xb5 */
    &Z80::op_ed_nop,    /* 0xb6 */
    &Z80::op_ed_nop,    /* 0xb7 */
    &Z80::op_lddr,      /* 0xb8 -- */
    &Z80::op_cpdr,      /* 0xb9 -- */
    &Z80::op_indr,      /* 0xba -- */
    &Z80::op_otdr,      /* 0xbb -- */
    &Z80::op_ed_nop,    /* 0xbc */
    &Z80::op_ed_nop,    /* 0xbd */
    &Z80::op_ed_nop,    /* 0xbe */
    &Z80::op_ed_nop,    /* 0xbf */
    &Z80::op_ed_nop,    /* 0xc0 */
    &Z80::op_ed_nop,    /* 0xc1 */
    &Z80::op_ed_nop,    /* 0xc2 */
    &Z80::op_ed_nop,    /* 0xc3 */
    &Z80::op_ed_nop,    /* 0xc4 */
    &Z80::op_ed_nop,    /* 0xc5 */
    &Z80::op_ed_nop,    /* 0xc6 */
    &Z80::op_ed_nop,    /* 0xc7 */
    &Z80::op_ed_nop,    /* 0xc8 */
    &Z80::op_ed_nop,    /* 0xc9 */
    &Z80::op_ed_nop,    /* 0xca */
    &Z80::op_ed_nop,    /* 0xcb */
    &Z80::op_ed_nop,    /* 0xcc */
    &Z80::op_ed_nop,    /* 0xcd */
    &Z80::op_ed_nop,    /* 0xce */
    &Z80::op_ed_nop,    /* 0xcf */
    &Z80::op_ed_nop,    /* 0xd0 */
    &Z80::op_ed_nop,    /* 0xd1 */
    &Z80::op_ed_nop,    /* 0xd2 */
    &Z80::op_ed_nop,    /* 0xd3 */
    &Z80::op_ed_nop,    /* 0xd4 */
    &Z80::op_ed_nop,    /* 0xd5 */
    &Z80::op_ed_nop,    /* 0xd6 */
    &Z80::op_ed_nop,    /* 0xd7 */
    &Z80::op_ed_nop,    /* 0xd8 */
    &Z80::op_ed_nop,    /* 0xd9 */
    &Z80::op_ed_nop,    /* 0xda */
    &Z80::op_ed_nop,    /* 0xdb */
    &Z80::op_ed_nop,    /* 0xdc */
    &Z80::op_ed_nop,    /* 0xdd */
    &Z80::op_ed_nop,    /* 0xde */
    &Z80::op_ed_nop,    /* 0xdf */
    &Z80::op_ed_nop,    /* 0xe0 */
    &Z80::op_ed_nop,    /* 0xe1 */
    &Z80::op_ed_nop,    /* 0xe2 */
    &Z80::op_ed_nop,    /* 0xe3 */
    &Z80::op_ed_nop,    /* 0xe4 */
    &Z80::op_ed_nop,    /* 0xe5 */
    &Z80::op_ed_nop,    /* 0xe6 */
    &Z80::op_ed_nop,    /* 0xe7 */
    &Z80::op_ed_nop,    /* 0xe8 */
    &Z80::op_ed_nop,    /* 0xe9 */
    &Z80::op_ed_nop,    /* 0xea */
    &Z80::op_ed_nop,    /* 0xeb */
    &Z80::op_ed_nop,    /* 0xec */
    &Z80::op_ed_nop,    /* 0xed */
    &Z80::op_ed_nop,    /* 0xee */
    &Z80::op_ed_nop,    /* 0xef */
    &Z80::op_ed_nop,    /* 0xf0 */
    &Z80::op_ed_nop,    /* 0xf1 */
    &Z80::op_ed_nop,    /* 0xf2 */
    &Z80::op_ed_nop,    /* 0xf3 */
    &Z80::op_ed_nop,    /* 0xf4 */
    &Z80::op_ed_nop,    /* 0xf5 */
    &Z80::op_ed_nop,    /* 0xf6 */
    &Z80::op_ed_nop,    /* 0xf7 */
    &Z80::op_ed_nop,    /* 0xf8 */
    &Z80::op_ed_nop,    /* 0xf9 */
    &Z80::op_ed_nop,    /* 0xfa */
    &Z80::op_ed_nop,    /* 0xfb */
    &Z80::op_ed_nop,    /* 0xfc */
    &Z80::op_ed_nop,    /* 0xfd */
    &Z80::op_ed_nop,    /* 0xfe */
    &Z80::op_ed_nop     /* 0xff */
};

const Z80::opCodeMethod Z80::op_xxcb[32] =
{
    &Z80::op_rlc_xx_d,  /* 0x06 */
    &Z80::op_rrc_xx_d,  /* 0x0e */
    &Z80::op_rl_xx_d,   /* 0x16 */
    &Z80::op_rr_xx_d,   /* 0x1e */
    &Z80::op_sla_xx_d,  /* 0x26 */
    &Z80::op_sra_xx_d,  /* 0x2e */
    &Z80::op_sll_xx_d,  /* 0x36 */
    &Z80::op_srl_xx_d,  /* 0x3e */
    &Z80::op_tb_n_xx_d, /* 0x46 */
    &Z80::op_tb_n_xx_d, /* 0x4e */
    &Z80::op_tb_n_xx_d, /* 0x56 */
    &Z80::op_tb_n_xx_d, /* 0x5e */
    &Z80::op_tb_n_xx_d, /* 0x66 */
    &Z80::op_tb_n_xx_d, /* 0x6e */
    &Z80::op_tb_n_xx_d, /* 0x76 */
    &Z80::op_tb_n_xx_d, /* 0x7e */
    &Z80::op_rb_n_xx_d, /* 0x86 */
    &Z80::op_rb_n_xx_d, /* 0x8e */
    &Z80::op_rb_n_xx_d, /* 0x96 */
    &Z80::op_rb_n_xx_d, /* 0x9e */
    &Z80::op_rb_n_xx_d, /* 0xa6 */
    &Z80::op_rb_n_xx_d, /* 0xae */
    &Z80::op_rb_n_xx_d, /* 0xb6 */
    &Z80::op_rb_n_xx_d, /* 0xbe */
    &Z80::op_sb_n_xx_d, /* 0xc6 */
    &Z80::op_sb_n_xx_d, /* 0xce */
    &Z80::op_sb_n_xx_d, /* 0xd6 */
    &Z80::op_sb_n_xx_d, /* 0xde */
    &Z80::op_sb_n_xx_d, /* 0xe6 */
    &Z80::op_sb_n_xx_d, /* 0xee */
    &Z80::op_sb_n_xx_d, /* 0xf6 */
    &Z80::op_sb_n_xx_d  /* 0xfe */
};

#define COMMON_GET 1

inline BYTE&
Z80::getReg8(BYTE val)
{
    switch (val & 0x7)
    {
        case 0:
            return (B);

        case 1:
            return (C);

        case 2:
            return (D);

        case 3:
            return (E);

        case 4:
            switch (prefix)
            {
                case ip_dd:
                    return (IXh);

                case ip_fd:
                    return (IYh);

                case ip_none:
                default:
                    return (H);

            }

            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
            assert(0);
            return (W);

        case 5:
            switch (prefix)
            {
                case ip_dd:
                    return (IXl);

                case ip_fd:
                    return (IYl);

                case ip_none:
                default:
                    return (L);

            }

            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
            assert(0);
            return (Z);

        case 6:
            // Should never be called in with this value.
            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
            assert(0);
            return (Z);

        case 7:
            return (A);
    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
    assert(0);
    return (Z);
}

inline BYTE&
Z80::getCoreReg8(BYTE val)
{
    switch (val & 0x7)
    {
        case 0:
            return (B);

        case 1:
            return (C);

        case 2:
            return (D);

        case 3:
            return (E);

        case 4:
            return (H);

        case 5:
            return (L);

        case 6:
            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
            assert(0);
            return (Z);

        case 7:
            return (A);

    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x7)
    assert(0);
    return (Z);
}

inline BYTE
Z80::getReg8Val(BYTE val)
{
    return (getReg8(val));
}

inline BYTE
Z80::getCoreReg8Val(BYTE val)
{
    return (getCoreReg8(val));
}

// Register related
inline WORD&
Z80::getReg16(BYTE val)
{
    switch (val & 0x3)
    {
        case 0:
            return (BC);

        case 1:
            return (DE);

        case 2:
            switch (prefix)
            {
                case ip_dd:
                    return (IX);

                case ip_fd:
                    return (IY);

                case ip_none:
                default:
                    return (HL);
            }

            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x3)
            assert(0);
            return (WZ);

        case 3:
            return (SP);
    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x3)
    assert(0);
    return (WZ);
}

inline WORD&
Z80::getCoreReg16(BYTE val)
{
    switch (val & 0x3)
    {
        case 0:
            return (BC);

        case 1:
            return (DE);

        case 2:
            return (HL);

        case 3:
            return (SP);
    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 0x3)
    assert(0);
    return (WZ);
}

inline WORD&
Z80::getHLReg16(void)
{
    switch (prefix)
    {
        case ip_dd:
            return (IX);

        case ip_fd:
            return (IY);

        case ip_none:
        default:
            return (HL);
    }
}

inline WORD
Z80::getIxReg16Val(void)
{
    switch (prefix)
    {
        case ip_dd:
            return (IX);

        case ip_fd:
            return (IY);

        case ip_none:
            break;
    }

    // should not have been called.
    debugss(ssZ80, FATAL, "%s: Invalid prefix mode", __FUNCTION__)
    assert(0);
    return (WZ);

}

inline WORD
Z80::getIndirectAddr(void)
{
    switch (prefix)
    {
        case ip_dd:
            return (IX + sREADn());

        case ip_fd:
            return (IY + sREADn());

        case ip_none:
        default:
            return (HL);
    }
}

inline WORD
Z80::getHLReg16Val(void)
{
    return (getHLReg16());
}

inline WORD
Z80::getReg16Val(BYTE val)
{
    return (getReg16(val));
}

inline WORD
Z80::getCoreReg16Val(BYTE val)
{
    return (getCoreReg16(val));
}


inline WORD&
Z80::getReg16qq(BYTE val)
{
    switch (val & 0x3)
    {
        case 0:
            return (BC);

        case 1:
            return (DE);

        case 2:
            switch (prefix)
            {
                case ip_dd:
                    return (IX);

                case ip_fd:
                    return (IY);

                case ip_none:
                default:
                    return (HL);
            }

            // All cases are handled above, can never get to this point, but to quiet the compiler
            debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 3)
            assert(0);
            return (WZ);

        case 3:
            return (AF);
    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 3)
    assert(0);
    return (WZ);
}

inline WORD&
Z80::getCoreReg16qq(BYTE val)
{
    switch (val & 0x3)
    {
        case 0:
            return (BC);

        case 1:
            return (DE);

        case 2:
            return (HL);

        case 3:
            return (AF);
    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown reg: %d", __FUNCTION__, val & 3)
    assert(0);
    return (WZ);
}

inline WORD
Z80::getReg16qqVal(BYTE val)
{
    return (getReg16qq(val));
}

inline WORD
Z80::getCoreReg16qqVal(BYTE val)
{
    return (getCoreReg16qq(val));
}


inline BYTE
Z80::getBit(BYTE val)
{
    return (1 << ((val >> 3) & 0x7));
}

inline bool
Z80::checkCondition(BYTE val)
{
    switch ((val >> 3) & 0x7)
    {
        case 0: // NZ non-zero
            return ((F & Z_FLAG) == 0);

        case 1: // Z zero
            return ((F & Z_FLAG) != 0);

        case 2: // NC no carry
            return ((F & C_FLAG) == 0);

        case 3: //  C carry
            return ((F & C_FLAG) != 0);

        case 4: // PO parity odd
            return ((F & P_FLAG) == 0);

        case 5: // PE parity even
            return ((F & P_FLAG) != 0);

        case 6: // P sign positive
            return ((F & S_FLAG) == 0);

        case 7: // M sign negative
            return ((F & S_FLAG) != 0);

    }

    // All cases are handled above, can never get to this point, but to quiet the compiler
    debugss(ssZ80, FATAL, "%s: Unknown cond: %d", __FUNCTION__, (val >> 3) & 7);
    assert(0);
    return (false);
}

// Routines related to the stack.

inline void
Z80::PUSH(WORD x)
{
    writeMEM(--SP, x >> 8);
    writeMEM(--SP, x & 0xff);
};

inline void
Z80::POP(WORD& x)
{
    x   = (readMEM(SP + 1) << 8) | readMEM(SP);
    SP += 2;
};

// Routines related to FLAGS.

inline void
Z80::SET_FLAGS(BYTE flags)
{
    F |= flags;
};

inline void
Z80::CLEAR_FLAGS(BYTE flags)
{
    F &= ~flags;
};

inline bool
Z80::CHECK_FLAGS(BYTE flags)
{
    return ((F & flags) != 0);
};

inline void
Z80::COND_FLAGS(bool cond,
                BYTE flags)
{
    if (cond)
    {
        SET_FLAGS(flags);
    }
    else
    {
        CLEAR_FLAGS(flags);
    }
};

/// \todo - determine which way to keep.
inline void
Z80::SET_ZSP_FLAGS(BYTE val)
{
#if USE_TABLE
    // Mask off the bits.
    CLEAR_FLAGS(Z_FLAG | S_FLAG | P_FLAG);
    // Set bits from pre-generated table
    F |= ZSP[val];
#else
    COND_FLAGS((!val), Z_FLAG);
    COND_FLAGS((val & 0x80), S_FLAG);
    // destructive, so have to do this check last
    val ^= val >> 1;
    val ^= val >> 2;
    val ^= val >> 4;
    val ^= 1;
    COND_FLAGS(val & 0x01, P_FLAG);
#endif
};

///
/// @param clockRate Speed of the Z80 in cycles per second.
/// @param ticksPerSecond Number of interrupts per second for the clock
///
Z80::Z80(int clockRate,
         int ticksPerSecond): CPU(),
                              GppListener(z80_gppSpeedSelBit_c),
                              A(af.hi),
                              sA(af.shi),
                              F(af.lo),
                              AF(af.val),
                              B(bc.hi),
                              C(bc.lo),
                              BC(bc.val),
                              D(de.hi),
                              E(de.lo),
                              DE(de.val),
                              H(hl.hi),
                              L(hl.lo),
                              HL(hl.val),
                              sHL(hl.sval),
                              W(wz.hi),
                              sW(wz.shi),
                              Z(wz.lo),
                              WZ(wz.val),
                              IXl(ix.lo),
                              IXh(ix.hi),
                              IX(ix.val),
                              IYl(iy.lo),
                              IYh(iy.hi),
                              IY(iy.val),
                              SP(sp.val),
                              INT_Line(false), // never used
                              intLevel_m(0),   // never used
                              processingIntr(false),
                              ticks(0),
                              lastInstTicks(0),
                              curInstByte(0),
                              mode(cm_reset),
                              prefix(ip_none),
                              resetReq(false), // never used
                              speedUpFactor_m(40),
                              fast_m(false),
                              IM(0)

{
    debugss(ssZ80, INFO, "%s: Creating Z80 proc, clock (%d), ticks(%d)\n", __FUNCTION__,
            clockRate,
            ticksPerSecond);

    if ((clockRate) && (ticksPerSecond))
    {
        ticksPerClock_m  = clockRate / ticksPerSecond;
        ClockRate_m      = clockRate;
        ticksPerSecond_m = ticksPerSecond;
    }
    else
    {
        // Assume a 4MHz processor with a 10ms clock interrupt.
        debugss(ssZ80, WARNING, "%s: invalid parameters setting default\n", __FUNCTION__);
        ClockRate_m      = 4000000;
        ticksPerSecond_m = 100;
        ticksPerClock_m  = 40000;
    }

    WallClock::instance()->updateTicksPerSecond(ClockRate_m);

    reset();

    mode = cm_running;

}

///
///
///
Z80::~Z80()
{
    debugss(ssZ80, INFO, "%s\n", __FUNCTION__);
}

///
/// Reset the state of the Z80 CPU.
///
/// \todo - Complete the reset implementation.
///
void
Z80::reset(void)
{
    debugss(ssZ80, INFO, "%s\n", __FUNCTION__);

    PC            = 0;
    R             = 0;
    I             = 0;
    IFF0          = IFF1 = IFF2 = false;
    IM            = 0;
    // according to (modern) Z80 documentation, only the above registers
    // are affected by RESET.
    AF            = SP = 0xffff;
    prefix        = ip_none;
    curInstByte   = 0;
    lastInstTicks = 0;
    resetReq      = true; // never used
    int_type      = 0;
    mode          = cm_reset;
    ticks         = 0;
    fast_m        = false;
    // TODO: reset speedup...
    addClockTicks();
}

///
/// Provides the CPU with an addition amount of cpu cycles.
///
void
Z80::addClockTicks()
{
    // debugss(ssZ80, ALL, "%s\n", __FUNCTION__);

    // if ticks is negative, add the new quota.
    if (ticks < 0)
    {
        debugss(ssZ80, ALL, "%s: ticks(%d) += ticksPerClock(%lu)\n", __FUNCTION__, ticks,
                ticksPerClock_m);
        ticks += ticksPerClock_m;
    }
    else
    {
        // else just set it, otherwise we run the risk of accumulating too many ticks if
        // the CPU was idle waiting for input.
        debugss(ssZ80, ALL, "%s: ticks(%d) = ticksPerClock(%lu)\n", __FUNCTION__,
                ticks, ticksPerClock_m);
        ticks = (sig_atomic_t) ticksPerClock_m;
    }

    lastInstTicks = ticks;
}

void
Z80::setSpeedup(int factor) {
    speedUpFactor_m = factor;
}

void
Z80::enableFast() {
    GppListener::addListener(this);
}

void
Z80::gppNewValue(BYTE gpo) {
    fast_m = ((gpo & z80_gppSpeedSelBit_c) != 0);
    debugss(ssZ80, WARNING, "%s: fast_m = %d\n", __FUNCTION__, fast_m);

    if (fast_m)
    {
        if (ticks > 0)
        {
            ticks        *= speedUpFactor_m;
            lastInstTicks = ticks;
        }
        ClockRate_m     = 2048000 * speedUpFactor_m;
        ticksPerClock_m = ClockRate_m / ticksPerSecond_m;
    }
    else
    {
        if (ticks > 0)
        {
            ticks        /= speedUpFactor_m;
            lastInstTicks = ticks;
        }
        ClockRate_m     = 2048000;
        ticksPerClock_m = ClockRate_m / ticksPerSecond_m;
    }

    WallClock::instance()->updateTicksPerSecond(ClockRate_m);
}

///
/// Raise the non-maskable interrupt.
///
void
Z80::raiseNMI(void)
{
    debugss(ssZ80, VERBOSE, "%s\n", __FUNCTION__);

    int_type |= Intr_NMI;
}

///
/// Raise an interrupt.
///
/// @param level The interrupt level to raise.
///
void
Z80::raiseINT()
{
    debugss(ssZ80, VERBOSE, "%s\n", __FUNCTION__);

    INT_Line  = true;
    int_type |= Intr_INT;
}

///
/// Lower an interrupt.
///
/// @param level The interrupt level to lower.
///
void
Z80::lowerINT()
{
    debugss(ssZ80, VERBOSE, "%s\n", __FUNCTION__);

    int_type &= ~Intr_INT;
    INT_Line  = false;

}

void
Z80::continueRunning(void)
{
    mode = cm_running;
}

void
Z80::waitState(void)
{
    WallClock::instance()->addTicks(1);
    // TODO: anything else needs to make progress?
}

///
/// Links the address bus object to the virtual CPU.
///
/// @param ab  pointer to the address bus object.
///
void
Z80::setAddressBus(AddressBus* ab)
{
    debugss(ssZ80, INFO, "%s: Entering\n", __FUNCTION__);

    ab_m = ab;
}

#if 0
void
Z80::setCPUStates(BYTE error, BYTE state)
{
    // printf("Setting ERROR/STATES\n\n\n");
    cpu_error = error;
    cpu_state = state;

}
#endif

void
Z80::traceInstructions(void)
{
#if 0
#if 0
    static int tks = 0;

    // if (tks != ticks)
    {
        fprintf(opcode_out, " ticks = %d\n", ticks);
        //  fprintf(opcode_out, " ticksPerSecond = %d\n", ticksPerSecond_m);
        tks = ticks;
    }
    return;
#endif

    unsigned char* p;

    p = &mem[PC];
//  fprintf(opcode_out, "%04x %04x %04x %04x %04x %04x ",
//         PC, AF, BC, DE, HL, SP);
//  fprintf(opcode_out, "%04x: ", PC);
    fprintf(opcode_out, "%04x %04x %04x %04x %04x\n",
            AF, BC, DE, HL, SP);
    fprintf(opcode_out, "%03o.%03o (%02x.%02x): ", (PC >> 8), PC & 0xff, (PC >> 8),
            PC & 0xff);

    disass(&p, PC);
#endif
#if 0
    fprintf(opcode_out, "%04x %04x %04x %04x %04x %04x ", PC, AF, BC, DE, HL, SP);
    fprintf(opcode_out, "PC: %03o.%03o (%02x.%02x): %03o %03o %03o\n", (PC >> 8), PC & 0xff,
            (PC >> 8), PC & 0xff,
            readMEM(PC),
            readMEM(PC + 1),
            readMEM(PC + 2));
#else
//  fprintf(opcode_out, "%03o.%03o %04x %04x %04x %04x %04x %04x ", PC >> 8, PC & 0xff,
//    PC, AF, BC, DE, HL, SP);
//  debug(" %04x %04x %04x %04x %04x %04x : ", PC, AF, BC, DE, HL, SP);
#if 1

    // debugss(ssZ80, ALL, "z80: %04x(%03o.%03o) %04x %04x %04x %04x %04x : ",
    //          PC, (PC >> 8) & 0xff, (PC & 0xff), AF, BC, DE, HL, SP);
    if (chkdebuglevel(ssZ80, ALL))
    {
#if 0
        debugss(ssZ80, ALL, "z80: %04x(%03o.%03o) %04x %04x %04x %04x %04x\n",
                PC, (PC >> 8) & 0xff, (PC & 0xff), AF, BC, DE, HL, SP);
#else

        if (1)
        {
            debugss(ssZ80, ALL, "z80: %04x(%03o.%03o) %04x %04x %04x %04x %04x : ",
                    PC, (PC >> 8) & 0xff, (PC & 0xff), AF, BC, DE, HL, SP);
            disass(PC);
        }

#endif
    }

#endif
#endif
}

/// Single step the virtual CPU
///
/// \retval result of the executed instruction.
///
BYTE
Z80::step(void)
{
    //
    cpu_state = SINGLE_STEP_C;
    return (execute(1));
}

// This is only used by the execute() thread.
void
Z80::systemMutexCycle()
{
    h89.systemMutexRelease();
    // If someone else is waiting, they should get the mutex now.
    h89.systemMutexAcquire();
}

std::string
Z80::dumpDebug()
{
    std::string ret = PropertyUtil::sprintf(
        "A=%02x PSW=%s %s %s %s %s %s %s %s    AF'=%04x\n"
        "BC=%04x    BC'=%04x\n"
        "DE=%04x    DE'=%04x\n"
        "HL=%04x    HL'=%04x\n"
        "PC=%04x SP=%04x\n"
        "    executing: %02x %02x %02x %02x\n"
        "IX=%04x IY=%04x mode=%s\n"
        "R=%02x I=%02x IFF0=%d IFF1=%d IFF2=%d INT=%d NMI=%d\n",
        A,
        (F & S_FLAG) ? "S" : "s",
        (F & Z_FLAG) ? "Z" : "z",
        (F & N2_FLAG) ? "N2" : "n2",
        (F & H_FLAG) ? "H" : "h",
        (F & N1_FLAG) ? "N1" : "n1",
        (F & P_FLAG) ? "P" : "p",
        (F & N_FLAG) ? "N" : "n",
        (F & C_FLAG) ? "C" : "c",
        _af,
        BC, _bc, DE, _de, HL, _hl,
        PC, SP,
        ab_m->readByte(PC + 0),
        ab_m->readByte(PC + 1),
        ab_m->readByte(PC + 2),
        ab_m->readByte(PC + 3),
        IX, IY,
        (mode == cm_halt ? "halt" :
         (mode == cm_running ? "run" :
          (mode == cm_reset ? "reset" :
           (mode == cm_singleStep ? "sstep" : "?")))),
        R & 0xff, I, IFF0, IFF1, IFF2,
        ((int_type & Intr_INT) != 0),
        ((int_type & Intr_NMI) != 0));
    return ret;
}

///
///  This function builds the Z80 central processing unit.
///  The opcode where PC points to is fetched from the memory
///  and PC incremented by one. The opcode is used as an
///  index to an array with function pointers, to execute a
///  function which emulates this Z80 opcode.
///
/// @param numInst Number of instructions to run. A value of zero implies
///                infinite number of instructions.
///
/// \retval number of cycles instruction took to execute.
///
BYTE
Z80::execute(WORD numInst)
{

    bool limited = (numInst != 0);

    cpu_state = RUN_C;
    h89.systemMutexAcquire();

    do
    {
        systemMutexCycle();

        if (mode == cm_reset)
        {
            // any local variables need resetting?
            mode = cm_running;
        }

        prefix         = ip_none;
        curInstByte    = 0;
        lastInstByte   = 0;

#ifdef FRONTPANEL /* update frontpanel */
        fp_led_address = PC;
        fp_led_data    = ram[PC];
        fp_sampleData();
#endif


#ifdef HISIZE /* write history */
        his[h_next].h_adr = PC;
        his[h_next].h_af  = REG_PAIR(A, F);
        his[h_next].h_bc  = REG_PAIR(B, C);
        his[h_next].h_de  = REG_PAIR(D, E);
        his[h_next].h_hl  = REG_PAIR(H, L);
        his[h_next].h_ix  = IX;
        his[h_next].h_iy  = IY;
        his[h_next].h_sp  = SP;
        h_next++;

        if (h_next == HISIZE)
        {
            h_flag = 1;
            h_next = 0;
        }

#endif

        /// \todo fix interrupt timings see http://www.z80.info/interrup.htm
        // CPU interrupt handling
        //
        if (int_type & Intr_NMI)
        {
            debugss(ssZ80, VERBOSE, "NMI Raised\n");
            int_type &= ~Intr_NMI;
            IFF0      = IFF1 = false;
            PUSH(PC);
            PC        = 0x66;
            mode      = cm_running;
        }
        else if ((int_type & Intr_INT) && (IFF1))
        {
            // ISR required to enable interrupts (EI).
            IFF2 = IFF1 = IFF0 = false;
            mode = cm_running;

            switch (IM)
            {
                case 0:
                    // mode zero currently just supports the RST instructions based on the level
                    // passed in - this is all that is needed on an Heathkit H89.
                    /// \todo But it should be enhance to support any instruction placed on the
                    /// data bus. -
                    debugss(ssZ80, VERBOSE, "Processing interrupt mode 0\n");
                    processingIntr = true;
                    break;

                case 1:
                    debugss(ssZ80, VERBOSE, "Processing interrupt mode 1\n");
                    int_type    &= ~Intr_INT;
                    lastInstByte = 0xff;
                    op_rst();
                    ticks       -= 4;
                    break;

                case 2:
                    // mode 2 not currently supported.
                    debugss(ssZ80, FATAL, "%s: Interrupt mode 2 not supported\n", __FUNCTION__);

                    /// \todo assert
                    break;

                default:
                    debugss(ssZ80, FATAL, "%s: Invalid Interrupt Mode: %d\n", __FUNCTION__, IM);

                    /// \todo assert
            }
        }

        IFF1 = IFF0;

#if NOTNOW
        debug("%04o.%04o", (PC >> 8), PC & 0xff);

        if (PC < 8192)
        {
            traceInstructions(); // debug("\n");
        }
        else
        {
            traceInstructions();
        }

#endif

        // check to see if the clock has any ticks left
        if (ticks <= 0)
        {
            // No virtual time left in this timer tick, wait for the next one.
            static struct timespec sp;
            static struct timespec act;

            // Can over-shoot the time to sleep, the timer interrupt will wake
            // CPU at the correct time.
            sp.tv_sec  = 1;
            sp.tv_nsec = 0;
            h89.systemMutexRelease();
            nanosleep(&sp, &act);
            h89.systemMutexAcquire();
            continue;
        }

        // If in halt, we just do a NOP, without any PC changes.
        if (mode == cm_halt)
        {
            ticks        -= 4;
            lastInstTicks = ticks; // don't double-bill next instruction
            WallClock::instance()->addTicks(4);
            continue;
        }

        // traceInstructions();
        lastInstByte  = curInst[0] = readInst();
        (this->*op_code[curInst[0]])();
        unsigned int val = lastInstTicks - ticks;
        lastInstTicks = ticks;
        WallClock::instance()->addTicks(val);

#if ACCURATE_R
        // incrementing R doesn't affect bit 7,
        R = (R | 0x80) | ((R + 1) & 0x7F);
        // could also just use R++ and save another R', then when needing R,
        // R = (R' & 0x80) | (R & 0x7f).
#else
        R++; /* increment refresh register */
#endif

#ifdef WANT_GUI
        check_gui_break();
#endif

        if ((cpu_state == RUN_C) && (limited) && (--numInst == 0))
        {
            cpu_state = SINGLE_STEP_C;
        }

        processingIntr = false;
    }
    while (cpu_state == RUN_C);

    h89.systemMutexRelease();

    return (0);
}

///
/// \brief No Operation
///
/// \details
/// <pre>
/// Operation: -
///
/// Opcode: NOP
///          +-+-+-+-+-+-+-+-+
///          |0|0|0|0|0|0|0|0|   0x00
///          +-+-+-+-+-+-+-+-+
///
/// Description: The CPU performs no operation during this machine cycle.
///
/// M Cycles  T States    4 MHz E.T.
/// ---------+---------+-------------
///    1          4          1.00
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_nop(void)
{

}


///
/// \brief Halt
///
/// \details
/// <pre>
/// Operation: -
///
/// Opcode: HALT
///          +-+-+-+-+-+-+-+-+
///          |0|1|1|1|0|1|1|0|   0x76
///          +-+-+-+-+-+-+-+-+
///
/// Description: The HALT instruction suspends CPU operation until a subsequent interrupt
///              or reset is received. While in the HALT state, the processor executes NOPs
///              to maintain memory refresh logic.
///
/// M Cycles  T States    4 MHz E.T.
/// ---------+---------+-------------
///    1          4          1.00
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_halt(void)
{
    debugss(ssZ80, VERBOSE, "%s - iff1 %d iff2 %d\n", __FUNCTION__, IFF1, IFF2);

    mode = cm_halt;
}

///
/// \brief Set Carry Flag
///
/// \details
/// <pre>
/// Operation: CY <- 1
///
/// Opcode: SCF
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|1|0|1|1|1|   0x37
///          +-+-+-+-+-+-+-+-+
///
/// Description: The Carry flag in the F register is set.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+----------+------------
///    1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is reset
///     P/V is not affected
///     N is reset
///     C is set
/// </pre>
///
/// \retval none
///
void
Z80::op_scf(void)
{
    SET_FLAGS(C_FLAG);
    CLEAR_FLAGS(N_FLAG | H_FLAG);
}

///
/// \brief Complement Carry Flag
///
/// <pre>
/// Operation: CY <- !CY
///
/// Opcode: CCF
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|1|0|1|1|1|   0x37
///          +-+-+-+-+-+-+-+-+
///
/// Description: The Carry flag in the F register is inverted.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+----------+------------
///    1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H previous carry is copied
///     P/V is not affected
///     N is reset
///     C is set if CY was 0 before operation; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_ccf(void)
{
    if (CHECK_FLAGS(C_FLAG))
    {
        SET_FLAGS(H_FLAG);
        CLEAR_FLAGS(C_FLAG);
    }
    else
    {
        CLEAR_FLAGS(H_FLAG);
        SET_FLAGS(C_FLAG);
    }

    CLEAR_FLAGS(N_FLAG);
}

///
/// \brief Complement Accumulator
///
/// <pre>
/// Operation: A <- !A
///
/// Opcode: CPL
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|0|1|1|1|1|   0x2F
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator (register A) are inverted (one's
///              complement).
///
/// M Cycles  T States    4 MHz E.T.
/// --------+----------+------------
///    1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is set
///     P/V is not affected
///     N is set
///     C is not affected
/// </pre>
///
/// \retval none
///
void
Z80::op_cpl(void)
{
    A = ~A;

    SET_FLAGS(H_FLAG | N_FLAG);
}

///
/// \brief Decimal Adjust AL (DAA)
///
/// <pre>
/// Operation:
///
/// Opcode: DAA
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|0|0|1|1|1|   0x27
///          +-+-+-+-+-+-+-+-+
///
/// Description: This instruction conditionally adjusts the Accumulator for BCD addition and
///              subtraction operations. For addition (ADD, ADC, INC) or subtraction (SUB,
///              SBC, DEC, NEG), the following table indicates the operation performed:
///
///  - \todo - add table from manual here.
///
/// M Cycles  T States    4 MHz E.T.
/// ---------+----------+------------
///    1          4          1.00
///
/// Condition Bits Affected:
///     S is set if most-significant bit of Accumulator is 1 after operation; reset
///       otherwise
///     Z is set if Accumulator is zero after operation; reset otherwise
///     H, see instruction
///     P/V is set if Accumulator is even parity after operation; reset otherwise
///     N is not affected
///     C, see instruction
/// </pre>
///
/// \note This took a LONG time to get all the results/flags accurate with a real Z80,
///       so any modifications should be done with extreme care and revalidated
///       with the instruction test program.
///
/// \retval none
///
void
Z80::op_daa(void)
{
    WORD tmp_a      = A;
    BYTE low_nibble = A & 0x0f;
    bool carry      = CHECK_FLAGS(C_FLAG);

    // determine what type of operation was last done.
    if (CHECK_FLAGS(N_FLAG))
    {
        // Subtraction
        int adjustment = (carry || (tmp_a > 0x99)) ? 0x160 : 0x00;

        if (CHECK_FLAGS(H_FLAG) || (low_nibble > 9))
        {
            if (low_nibble > 5)
            {
                CLEAR_FLAGS(H_FLAG);
            }

            tmp_a = (tmp_a - 6) & 0xff;
        }

        tmp_a -= adjustment;
    }
    else
    {
        // Addition
        if (CHECK_FLAGS(H_FLAG) || (low_nibble > 9))
        {
            COND_FLAGS((low_nibble > 9), H_FLAG);
            tmp_a += 6;
        }

        if (carry || ((tmp_a & 0x1f0) > 0x90))
        {
            tmp_a += 0x60;
        }
    }

    A = (tmp_a & 0xff);

    COND_FLAGS((carry || (tmp_a & 0x100)), C_FLAG);
    SET_ZSP_FLAGS(A);
}

///
/// \brief Enable Interrupts (EI)
///
/// <pre>
/// Operation: IFF <- 1
///
/// Opcode: EI
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|1|1|0|1|1|   0xFB
///          +-+-+-+-+-+-+-+-+
///
/// Description: The enable interrupt instruction sets both interrupt enable flip flops
///              (IFF1 and IFF2) to a logic 1, allowing recognition of any maskable
///              interrupt. Note that during the execution of this instruction and the
///              following instruction, maskable interrupts are disabled.
///
/// M Cycles  T States    4 MHz E.T.
/// --------  ---------  ------------
///    1          4          1.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ei(void)
{
    /*IFF1 = */ IFF2 = true;
    IFF0             = true;

}

///
/// \brief Disable Interrupts (DI)
///
/// <pre>
/// Operation: IFF <- 0
///
/// Opcode: DI
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|1|0|0|1|1|   0xF3
///          +-+-+-+-+-+-+-+-+
///
/// Description: DI disables the maskable interrupt by resetting the interrupt enable
///              flip-flops (IFF1 and IFF2). Note that this instruction disables the
///              maskable interrupt during its execution.
///
/// M Cycles  T States    4 MHz E.T.
/// --------  ---------  ------------
///    1          4          1.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_di(void)
{
    IFF0 = IFF1 = IFF2 = false;
}

///
/// \brief Read from Port (IN)
///
/// <pre>
/// Operation: A <- (n)
///
/// Opcode: IN
/// Operands: A, (n)
///          +-+-+-+-+-+-+-+-+
///          |1|1|0|1|1|0|1|1|   0xDB
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The operand n is placed on the bottom half (A0 through A7) of the address
///              bus to select the I/O device at one of 256 possible ports. The contents of
///              the Accumulator also appear on the top half (A8 through A15) of the address
///              bus at this time. Then one byte from the selected port is placed on the data
///              bus and written to the Accumulator (register A) in the CPU.
///
/// M Cycles  T States    4 MHz E.T.
/// --------  ---------  ------------
///    3      11(4,3,4)      2.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_in(void)
{
    A      = (h89.getIO()).in(READn());

    ticks -= 4;
}

///
/// \brief Write to Port (OUT)
///
/// <pre>
/// Operation: (n) <- A
///
/// Opcode: OUT
/// Operands: (n), A
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|0|1|0|0|1|1|   0xD3
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The operand n is placed on the bottom half (A0 through A7) of the address
///              bus to select the I/O device at one of 256 possible ports. The contents of
///              the Accumulator (register A) also appear on the top half (A8 through A15) of
///              the address bus at this time. Then the byte contained in the Accumulator is
///              placed on the data bus and written to the selected peripheral device.
///
/// M Cycles  T States    4 MHz E.T.
/// --------  ---------  ------------
///    3      11(4,3,4)      2.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_out(void)
{
    (h89.getIO()).out(READn(), A);

    ticks -= 4;
}

///
/// \brief Load Register (LD)
///
/// <pre>
/// Operation: r <- (n)
///
/// Opcode: LD
/// Operands: r, n
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|  r  |1|1|0|
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The 8-bit integer n is loaded to any register r, where r identifies
///              register A, B, C, D, E, H, or L, assembled as follows in the object code:
///
///          +--------+------+
///          |Register|  r, C|
///          +--------+------+
///          |    A   |  111 |
///          |    B   |  000 |
///          |    C   |  001 |
///          |    D   |  010 |
///          |    E   |  011 |
///          |    H   |  100 |
///          |    L   |  101 |
///          +--------+------+
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_x_n(void)
{
    getReg8(lastInstByte >> 3) = READn();
}

///
/// \brief LD A, (BC)
///
/// <pre>
/// Operation: A <- (BC)
///
/// Opcode: LD
/// Operands: A, (BC)
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|0|0|1|0|1|0|  0x0A
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the memory location specified by the contents of the BC
///              register pair are loaded to the Accumulator.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_a_ibc(void)
{
    A = readMEM(BC);
}

///
/// \brief Load Accumulator
///
/// <pre>
/// Operation: A <- (DE)
///
/// Opcode: LD
/// Operands: A, (DE)
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|0|1|1|0|1|0|  0x1A
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the memory location specified by the register pair DE are
///              loaded to the Accumulator.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_a_ide(void)
{
    A = readMEM(DE);
}

///
/// \brief Load immediate Accumulator
///
/// <pre>
/// Operation: A <- (nn)
///
/// Opcode: LD
/// Operands: A, (nn)
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|1|1|0|1|0|  0x3A
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the memory location specified by the operands nn are
///              loaded to the Accumulator. The first n operand after the Opcode is the
///              low order byte of a 2-byte memory address.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    4     13(4,3,3,3)      3.25
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_a_inn(void)
{
    A = readMEM(READnn());
}

///
/// \brief Store Accumulator
///
/// <pre>
/// Operation: (BC) <- A
///
/// Opcode: LD
/// Operands: (BC), A
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|0|0|0|0|1|0|  0x02
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator are loaded to the memory location
///              specified by the contents of the register pair BC.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_ibc_a(void)
{
    writeMEM(BC, A);
}

///
/// \brief Store Accumulator
///
/// <pre>
/// Operation: (DE) <- A
///
/// Opcode: LD
/// Operands: (DE), A
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|0|1|0|0|1|0|  0x12
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator are loaded to the memory location
///              specified by the contents of the DE register pair.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_ide_a(void)
{
    writeMEM(DE, A);
}

///
/// \brief Store immediate Accumulator
///
/// <pre>
/// Operation: (nn) <- A
///
/// Opcode: LD
/// Operands: (nn), A
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|1|0|0|1|0|  0x32
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator are loaded to the memory address
///              specified by the operand nn. The first n operand after the Opcode is the
///              low order byte of nn.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    4     13(4,3,3,3)      3.25
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_inn_a(void)
{
    writeMEM(READnn(), A);
}

///
/// \brief LD (HL), r
///
/// <pre>
/// Operation: (HL) <- r
///
/// Opcode: LD
/// Operands: (HL), r
///
///          +-+-+-+-+-+-+-+-+
///          |0|1|1|1|0|  r  |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of register r are loaded to the memory location specified by
///              the contents of the HL register pair. The symbol r identifies register A,
///              B, C, D, E, H, or L, assembled as follows in the object code:
///
///          +--------+------+
///          |Register|  r, C|
///          +--------+------+
///          |    A   |  111 |
///          |    B   |  000 |
///          |    C   |  001 |
///          |    D   |  010 |
///          |    E   |  011 |
///          |    H   |  100 |
///          |    L   |  101 |
///          +--------+------+
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)      1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_ihl_x(void)
{
    writeMEM(getIndirectAddr(), getCoreReg8Val(lastInstByte));
}

///
/// \brief Store immediate
///
/// <pre>
/// Operation: (HL) <- n
///
/// Opcode: LD
/// Operands: (HL), n
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|1|1|0|1|1|0|  0x36
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: Integer n is loaded to the memory address specified by the contents of the
///              HL register pair.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    3      10(4,3,3)      3.25
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_ihl_n(void)
{
    writeMEM(getIndirectAddr(), READn());
}

///
/// \brief LD r, r'
///
/// <pre>
/// Operation: r <- r'
///
/// Opcode: LD
/// Operands: r, r'
///
///          +-+-+-+-+-+-+-+-+
///          |0|1|  r  |  r' |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of any register r' are loaded to any other register r. r, r'
///              identifies any of the registers A, B, C, D, E, H, or L, assembled as follows
///              in the object code:
///
///          +--------+------+
///          |Register|  r,r'|
///          +--------+------+
///          |    A   |  111 |
///          |    B   |  000 |
///          |    C   |  001 |
///          |    D   |  010 |
///          |    E   |  011 |
///          |    H   |  100 |
///          |    L   |  101 |
///          +--------+------+
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    1          4         1.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_x_x(void)
{
    getReg8(lastInstByte >> 3) = getReg8Val(lastInstByte);
}

///
/// \brief LD r,(HL)
///
/// <pre>
/// Operation: r <- (HL)
///
/// Opcode: LD
/// Operands: r, (HL)
///
///          +-+-+-+-+-+-+-+-+
///          |0|1|  r  |1|1|0|
///          +-+-+-+-+-+-+-+-+
///
/// Description: The 8-bit contents of memory location (HL) are loaded to register r,
///              where r identifies register A, B, C, D, E, H, or L, assembled as follows in
///              the object code:
///
///          +--------+------+
///          |Register|   r  |
///          +--------+------+
///          |    A   |  111 |
///          |    B   |  000 |
///          |    C   |  001 |
///          |    D   |  010 |
///          |    E   |  011 |
///          |    H   |  100 |
///          |    L   |  101 |
///          +--------+------+
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        7(4,3)       1.75
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_x_ihl(void)
{
    getCoreReg8(lastInstByte >> 3) = readMEM(getIndirectAddr());
}

///
/// \brief LD  dd, nn
///
/// <pre>
/// Operation: dd <- nn
///
/// Opcode: LD
/// Operands: dd, nn
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|d|d|0|0|0|1|
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///          |       n       |
///          +-+-+-+-+-+-+-+-+
///
/// Description: The 2-byte integer nn is loaded to the dd register pair, where dd
///              defines the BC, DE, HL, or SP register pairs, assembled as follows
///              in the object code:
///
///     +----+--+
///     |Pair|dd|
///     +----+--+
///     | BC |00|
///     | DE |01|
///     | HL |10|
///     | SP |11|
///     +----+--+
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2      10(4,3,3)       2.50
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_xx_nn(void)
{
    getReg16(lastInstByte >> 4) = READnn();
}

///
/// \brief
///
/// <pre>
/// Operation: SP <- HL
///
/// Opcode: LD
/// Operands: SP, HL
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|1|1|0|0|1| 0xF9
///          +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the register pair HL are loaded to the Stack Pointer (SP).
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    1          6         1.50
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_sp_hl(void)
{
    SP     = HL;

    ticks -= 2;
}

///
/// \brief
///
/// <pre>
///
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_hl_inn(void)
{
    HL = readWord(READnn());
}

///
/// \brief LD (nn), HL
///
/// <pre>
///
/// </pre>
///
/// \retval none
///
void
Z80::op_ld_inn_hl(void)
{
    writeWord(READnn(), HL);
}

///
/// \brief INC ss
///
/// <pre>
/// Operation: ss <- ss + 1
///
/// Opcode: INC
/// Operands: ss
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|s|s|0|0|1|1|
///          +-+-+-+-+-+-+-+-+
///
///             +----+--+
///             |Pair|ss|
///             +----+--+
///             | BC |00|
///             | DE |01|
///             | HL |10|
///             | SP |11|
///             +----+--+
///
/// Description: The contents of register pair ss (any of register pairs BC, DE, HL, or SP)
///              are incremented. Operand ss is specified as follows in the assembled
///              object code.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    1          6         1.50
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_inc_xx(void)
{
    getReg16(lastInstByte >> 4)++;

    ticks -= 2;
}

///
/// \brief Decrement ss
///
/// <pre>
/// Operation: ss <- ss - 1
///
/// Opcode: DEC
/// Operands: ss
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|s|s|1|0|1|1|
///          +-+-+-+-+-+-+-+-+
///
///             +----+--+
///             |Pair|ss|
///             +----+--+
///             | BC |00|
///             | DE |01|
///             | HL |10|
///             | SP |11|
///             +----+--+
///
/// Description: The contents of register pair ss (any of register pairs BC, DE, HL, or SP)
///              are incremented. Operand ss is specified as follows in the assembled
///              object code.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    1          6         1.50
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_dec_xx(void)
{
    --getReg16(lastInstByte >> 4);

    ticks -= 2;
}

///
/// \brief ADD HL, ss
///
/// <pre>
/// Operation: HL <- HL + ss
///
/// Opcode:  ADD
/// Operands: HL,ss
///
///          +-+-+-+-+-+-+-+-+
///          |0|0|s|s|1|0|0|1|
///          +-+-+-+-+-+-+-+-+
///
///             +----+--+
///             |Pair|ss|
///             +----+--+
///             | BC |00|
///             | DE |01|
///             | HL |10|
///             | SP |11|
///             +----+--+
///
/// Description: The contents of register pair ss (any of register pairs BC, DE, HL, or SP)
///              are added to the contents of register pair HL and the result is stored in HL.
///              Operand ss is specified as follows in the assembled object code.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    3      11(4,4,3)      2.75
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is set if carry out of bit 11; reset otherwise
///     P/V is not affected
///     N is reset
///     C is set if carry from bit 15; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_add_reg16(WORD& res, WORD val)
{
    unsigned int sum = res + val;

    COND_FLAGS((res & 0x0fff) + (val & 0x0fff) > 0x0fff, H_FLAG);
    COND_FLAGS(sum > 0xffff, C_FLAG);
    CLEAR_FLAGS(N_FLAG);

    res    = (sum & 0xffff);

    ticks -= 7;
}


void
Z80::op_add_hl_xx(void)
{
    op_add_reg16(getHLReg16(), getReg16Val(lastInstByte >> 4));
}

///
/// \brief Logical AND
///
/// <pre>
/// Operation: A <- A ^ s
///
/// Opcode:  AND
/// Operands: s
///
///           The s operand is any of r, n, (HL), (IX+d), or (lY+d), as defined for the
///           analogous ADD instructions. These possible Opcode/operand
///           combinations are assembled as follows in the object code:
///                    +-+-+-+-+-+-+-+-+
///           AND r    |1|0|1|0|0|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           AND n    |1|1|1|0|0|1|1|0| 0xE6
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           AND (HL) |1|0|1|0|0|1|1|0| 0xA6
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         AND (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|0|0|1|1|0| 0xA6
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         AND (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|0|0|1|1|0| 0xA6
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: A logical AND operation is performed between the byte specified by the s
///              operand and the byte contained in the Accumulator; the result is stored in
///              the Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///   AND r       1          4          1.00
///   AND n       2        7(4,3)       1.75
///   AND (HL)    2        7(4,3)       1.75
///   AND (IX+d)  5     19(4,4,3,5,3)   4.75
///   AND (IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set
///     P/V is reset if overflow; reset otherwise
///     N is reset
///     C is reset
/// </pre>
///
/// \retval none
///
void
Z80::op_and(BYTE val)
{
    A &= val;

    SET_FLAGS(H_FLAG);
    CLEAR_FLAGS((N_FLAG | C_FLAG));
    SET_ZSP_FLAGS(A);
}

/// \retval none
void
Z80::op_and_x(void)
{
    op_and(getReg8Val(lastInstByte));
}

/// \retval none
void
Z80::op_and_ihl(void)
{
    op_and(readMEM(getIndirectAddr()));
}

/// \retval none
void
Z80::op_and_n(void)
{
    op_and(READn());
}

///
/// \brief OR
///
/// <pre>
/// Operation: A <- A v s
///
/// Opcode:  OR
/// Operands: s
///
///           The s operand is any of r, n, (HL), (IX+d), or (lY+d), as defined for the
///           analogous ADD instructions. These possible Opcode/operand
///           combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///           OR r     |1|0|1|1|0|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           OR n     |1|1|1|1|0|1|1|0| 0xF6
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           OR (HL)  |1|0|1|1|0|1|1|0| 0xB6
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          OR (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|1|0|1|1|0| 0xB6
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          OR (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|1|0|1|1|0| 0xB6
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: A logical OR operation is performed between the byte specified by the s
///              operand and the byte contained in the Accumulator; the result is stored in
///              the Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///    OR r       1          4          1.00
///    OR n       2        7(4,3)       1.75
///    OR (HL)    2        7(4,3)       1.75
///    OR (IX+d)  5     19(4,4,3,5,3)   4.75
///    OR (IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is reset
///     P/V is set if overflow; reset otherwise
///     N is reset
///     C is reset
/// </pre>
///
/// \retval none
///
void
Z80::op_or(BYTE val)
{
    A |= val;

    SET_ZSP_FLAGS(A);
    CLEAR_FLAGS(H_FLAG | N_FLAG | C_FLAG);
}

/// \retval none
///
void
Z80::op_or_x(void)
{
    op_or(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_or_ihl(void)
{
    op_or(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_or_n(void)
{
    op_or(READn());
}

///
/// \brief XOR (Exclusive OR)
///
/// <pre>
/// Operation: A <- A (+) s
///
///  Opcode:  XOR
/// Operands: s
///
///           The s operand is any of r, n, (HL), (IX+d), or (lY+d), as defined for the
///           analogous ADD instructions. These possible Opcode/operand
///           combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///           XOR r    |1|0|1|0|1|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           XOR n    |1|1|1|0|1|1|1|0| 0xEE
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          XOR (HL)  |1|0|1|0|1|1|1|0| 0xAE
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         XOR (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|0|1|1|1|0| 0xAE
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         XOR (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|0|1|1|1|0| 0xAE
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The logical exclusive-OR operation is performed between the byte
///              specified by the s operand and the byte contained in the Accumulator; the
///              result is stored in the Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///   XOR r       1          4          1.00
///   XOR n       2        7(4,3)       1.75
///   XOR (HL)    2        7(4,3)       1.75
///   XOR (IX+d)  5     19(4,4,3,5,3)   4.75
///   XOR (IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is reset
///     P/V is set if parity even; reset otherwise
///     N is reset
///     C is reset
/// </pre>
///
/// \retval none
///
void
Z80::op_xor(BYTE val)
{
    A ^= val;

    SET_ZSP_FLAGS(A);
    CLEAR_FLAGS(H_FLAG | N_FLAG | C_FLAG);
}

/// \retval none
///
void
Z80::op_xor_x(void)
{
    op_xor(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_xor_ihl(void)
{
    op_xor(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_xor_n(void)
{
    op_xor(READn());
}

///
/// \brief ADD
///
/// <pre>
/// Operation: A <- A + r
///
/// Opcode: ADD
/// Operands: A, r
///
///          +-+-+-+-+-+-+-+-+
///          |1|0|0|0|0|  r  |
///          +-+-+-+-+-+-+-+-+
///
///          +--------+------+
///          |Register|   r  |
///          +--------+------+
///          |    A   |  111 |
///          |    B   |  000 |
///          |    C   |  001 |
///          |    D   |  010 |
///          |    E   |  011 |
///          |    H   |  100 |
///          |    L   |  101 |
///          +--------+------+
///
/// Description: The contents of register r are added to the contents of the Accumulator, and
///              the result is stored in the Accumulator. The symbol r identifies the
///              registers A, B, C, D, E, H, or L, assembled as follows in the object code:
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    1          4          1.00
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if carry from bit 3; reset otherwise
///     P/V is set if overflow; reset otherwise
///     N is reset
///     C is set if carry from bit 7; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_add(BYTE val)
{
    SWORD i;

    COND_FLAGS(((A & 0xf) + (val & 0xf) > 0xf), H_FLAG);
    COND_FLAGS((A + val > 255), C_FLAG);

    A = i = sA + (signed char) val;

    COND_FLAGS((i < -128 || i > 127), P_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    CLEAR_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_add_x(void)
{
    op_add(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_add_ihl(void)
{
    op_add(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_add_n(void)
{
    op_add(READn());
}

///
/// \brief ADC (Add w/ Carry)
///
/// <pre>
/// Operation: A <- A + s + CY
///
/// Opcode: ADC
/// Operands: A, s
///
///        This s operand is any of r, n, (HL), (IX+d), or (lY+d) as defined for the
///        analogous ADD instruction. These possible Opcode/operand
///        combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///         ADC A,r    |1|0|0|0|1|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         ADC A,n    |1|1|0|0|1|1|1|0| 0xCE
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         ADC A,(HL) |1|0|0|0|1|1|1|0| 0x8E
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       ADC A,(IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|0|1|1|1|0| 0x8E
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       ADC A,(IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|0|1|1|1|0| 0x8E
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The s operand, along with the Carry Flag (C in the F register) is added
///              to the contents of the Accumulator, and the result is stored in the
///              Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
/// ADC A,r       1          4          1.00
/// ADC A,n       2        7(4,3)       1.75
/// ADC A,(HL)    2        7(4,3)       1.75
/// ADC A,(IX+d)  5     19(4,4,3,5,3)   4.75
/// ADC A,(IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if carry from bit 3; reset otherwise
///     P/V is set if overflow; reset otherwise
///     N is reset
///     C is set if carry from bit 7: reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_adc(BYTE val)
{
    SWORD i;
    BYTE  carry = CHECK_FLAGS(C_FLAG) ? 1 : 0;

    COND_FLAGS(((A & 0xf) + (val & 0xf) + carry > 0xf), H_FLAG);
    COND_FLAGS((A + val + carry > 255), C_FLAG);

    A = i = sA + (signed char) val + carry;

    COND_FLAGS((i < -128 || i > 127), P_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    CLEAR_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_adc_x(void)
{
    op_adc(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_adc_ihl(void)
{
    op_adc(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_adc_n(void)
{
    op_adc(READn());
}

///
/// \brief SUB (Subtract)
///
/// <pre>
/// Operation: A <- A - s
///
/// Opcode: SUB
/// Operands: A, s
///
///        This s operand is any of r, n, (HL), (IX+d), or (lY+d) as defined for the
///        analogous ADD instruction. These possible Opcode/operand
///        combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///         SUB A,r    |1|0|0|1|0|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         SUB A,n    |1|1|0|1|0|1|1|0| 0xD6
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         SUB A,(HL) |1|0|0|1|0|1|1|0| 0x96
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       SUB A,(IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|1|0|1|1|0| 0x96
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       SUB A,(IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|1|0|1|1|0| 0x96
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The s operand is subtracted from the contents of the Accumulator, and the
///              result is stored in the Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
/// SUB A,r       1          4          1.00
/// SUB A,n       2        7(4,3)       1.75
/// SUB A,(HL)    2        7(4,3)       1.75
/// SUB A,(IX+d)  5     19(4,4,3,5,3)   4.75
/// SUB A,(IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if borrow from bit 4; reset otherwise
///     P/V is set if overflow; reset otherwise
///     N is set
///     C is set if borrow; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_sub(BYTE val)
{
    SWORD i;

    COND_FLAGS(((val & 0xf) > (A & 0xf)), H_FLAG);
    COND_FLAGS((val > A), C_FLAG);

    A = i = sA - (signed char) val;

    COND_FLAGS(((i < -128) || (i > 127)), P_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    SET_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_sub_x(void)
{
    op_sub(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_sub_ihl(void)
{
    op_sub(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_sub_n(void)
{
    op_sub(READn());
}

///
/// \brief SBC (Subtract w/Carry)
///
/// <pre>
/// Operation: A <- A - s - CY
///
/// Opcode: SBC
/// Operands: A, s
///
///        This s operand is any of r, n, (HL), (IX+d), or (lY+d) as defined for the
///        analogous ADD instruction. These possible Opcode/operand
///        combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///         SBC A,r    |1|0|0|1|1|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         SBC A,n    |1|1|0|1|1|1|1|0| 0xDE
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         SBC A,(HL) |1|0|0|1|1|1|1|0| 0x9E
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       SBC A,(IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|1|1|1|1|0| 0x9E
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///       SBC A,(IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|0|1|1|1|1|0| 0x9E
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The s operand, along with the Carry flag (C in the F register) is subtracted
///              from the contents of the Accumulator, and the result is stored in the
///              Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
/// SBC A,r       1          4          1.00
/// SBC A,n       2        7(4,3)       1.75
/// SBC A,(HL)    2        7(4,3)       1.75
/// SBC A,(IX+d)  5     19(4,4,3,5,3)   4.75
/// SBC A,(IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if borrow from bit 4; reset otherwise
///     P/V is reset if overflow; reset otherwise
///     N is set
///     C is set if borrow; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_sbc(BYTE val)
{
    SWORD i;
    BYTE  carry = CHECK_FLAGS(C_FLAG) ? 1 : 0;

    COND_FLAGS(((val & 0xf) + carry > (A & 0xf)), H_FLAG);
    COND_FLAGS((val + carry > A), C_FLAG);

    A = i = sA - (signed char) val - carry;

    COND_FLAGS(((i < -128) || (i > 127)), P_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    SET_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_sbc_x(void)
{
    op_sbc(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_sbc_ihl(void)
{
    op_sbc(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_sbc_n(void)
{
    op_sbc(READn());
}

///
/// \brief CP (compare)
///
/// <pre>
/// Operation: A - s
///
/// Opcode: CP
/// Operands: s
///
///        This s operand is any of r, n, (HL), (IX+d), or (lY+d) as defined for the
///        analogous ADD instruction. These possible Opcode/operand
///        combinations are assembled as follows in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///            CP r    |1|0|1|1|1|  r  |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///            CP n    |1|1|1|1|1|1|1|0| 0xFE
///                    +-+-+-+-+-+-+-+-+
///                    |       n       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///            CP (HL) |1|0|1|1|1|1|1|0| 0xBE
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          CP (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|1|1|1|1|0| 0xBE
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          CP (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |1|0|1|1|1|1|1|0| 0xBE
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The s operand is subtracted from the contents of the Accumulator, and the
///              result is stored in the Accumulator.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///    CP r       1          4          1.00
///    CP n       2        7(4,3)       1.75
///    CP (HL)    2        7(4,3)       1.75
///    CP (IX+d)  5     19(4,4,3,5,3)   4.75
///    CP (IY+d)  5     19(4,4,3,5,3)   4.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if borrow from bit 4; reset otherwise
///     P/V is set if overflow; reset otherwise
///     N is set
///     C is set if borrow; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_cp(BYTE val)
{
    SWORD i;

    COND_FLAGS(((val & 0xf) > (A & 0xf)), H_FLAG);
    COND_FLAGS((val > A), C_FLAG);

    i = sA - (signed char) val;

    COND_FLAGS(((i < -128) || (i > 127)), P_FLAG);
    COND_FLAGS((i & 0x80), S_FLAG);
    COND_FLAGS((!i), Z_FLAG);
    SET_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_cp_x(void)
{
    op_cp(getReg8Val(lastInstByte));
}

/// \retval none
///
void
Z80::op_cp_ihl(void)
{
    op_cp(readMEM(getIndirectAddr()));
}

/// \retval none
///
void
Z80::op_cp_n(void)
{
    op_cp(READn());
}

///
/// \brief INC (increment)
///
/// <pre>
/// Operation: m <- m + 1
///
/// Opcode: INC
/// Operands: m
///
///        This m operand is any of r, (HL), (IX+d), or (lY+d). These
///        possible Opcode/operand combinations are assembled as follows
///        in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///           INC r    |0|0|  r  |1|0|0|
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///          INC (HL)  |0|0|1|1|0|1|0|0| 0x34
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         INC (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |0|0|1|1|0|1|0|0| 0x34
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         INC (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |0|0|1|1|0|1|0|0| 0x34
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The m operand is incremented by 1.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///   INC r       1          4          1.00
///   INC (HL)    3       11(4,4,3)     2.75
///   INC (IX+d)  6    23(4,4,3,5,4,3)  5.75
///   INC (IY+d)  6    23(4,4,3,5,4,3)  5.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if carry from bit 3; reset otherwise
///     P/V is set if operand was 0x7f before operation; reset otherwise
///     N is reset
///     C is not affected
/// </pre>
///
/// \retval none
///
void
Z80::op_inc(BYTE& val)
{
    COND_FLAGS(((val & 0xf) == 0xf), H_FLAG);

    val++;

    COND_FLAGS((val == 0x80), P_FLAG);
    COND_FLAGS((val & 0x80), S_FLAG);
    COND_FLAGS((!val), Z_FLAG);
    CLEAR_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_inc_x(void)
{
    op_inc(getReg8(lastInstByte >> 3));
}

/// \retval none
///
void
Z80::op_inc_ihl(void)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_inc(val);
    writeMEM(addr, val);

    ticks -= 1;
}

///
/// \brief DEC (decrement)
///
/// <pre>
/// Operation: m <- m - 1
///
/// Opcode: DEC
/// Operands: m
///
///        This m operand is any of r, (HL), (IX+d), or (lY+d). These
///        possible Opcode/operand combinations are assembled as follows
///        in the object code:
///
///                    +-+-+-+-+-+-+-+-+
///           DEC r    |0|0|  r  |1|0|1|
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///           DEC (HL) |0|0|1|1|0|1|0|1| 0x35
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         DEC (IX+d) |1|0|1|0|0|1|1|0| 0xDD
///                    +-+-+-+-+-+-+-+-+
///                    |0|0|1|1|0|1|0|1| 0x35
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///                    +-+-+-+-+-+-+-+-+
///         DEC (IY+d) |1|0|1|0|0|1|1|0| 0xFD
///                    +-+-+-+-+-+-+-+-+
///                    |0|0|1|1|0|1|0|1| 0x35
///                    +-+-+-+-+-+-+-+-+
///                    |       d       |
///                    +-+-+-+-+-+-+-+-+
///
///                    +--------+------+
///                    |Register|   r  |
///                    +--------+------+
///                    |    A   |  111 |
///                    |    B   |  000 |
///                    |    C   |  001 |
///                    |    D   |  010 |
///                    |    E   |  011 |
///                    |    H   |  100 |
///                    |    L   |  101 |
///                    +--------+------+
///
/// Description: The m operand decremented by 1.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///   DEC r       1          4          1.00
///   DEC (HL)    3       11(4,4,3)     2.75
///   DEC (IX+d)  6    23(4,4,3,5,4,3)  5.75
///   DEC (IY+d)  6    23(4,4,3,5,4,3)  5.75
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is zero; reset otherwise
///     H is set if borrow from bit 4; reset otherwise
///     P/V is set if m was 0x80 before operation; reset otherwise
///     N is set
///     C is not affected
/// </pre>
///
/// \retval none
///
void
Z80::op_dec(BYTE& val)
{
    COND_FLAGS(((val & 0xf) == 0), H_FLAG);
    COND_FLAGS((val == 0x80), P_FLAG);

    val--;

    COND_FLAGS((val & 0x80), S_FLAG);
    COND_FLAGS((!val), Z_FLAG);
    SET_FLAGS(N_FLAG);
}

/// \retval none
///
void
Z80::op_dec_x(void)
{
    op_dec(getReg8(lastInstByte >> 3));
}

/// \retval none
///
void
Z80::op_dec_ihl(void)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_dec(val);
    writeMEM(addr, val);

    ticks -= 1;
}

///
/// \brief RLCA
///
/// <pre>
/// Operation:          +-------------+
///              [CY] <-+--[7 << 0]<--+
///                        [- A - ]
///
/// Opcode: RLCA
/// Operands: -
///
///           +-+-+-+-+-+-+-+-+
///           |0|0|0|0|0|1|1|1|  0x07
///           +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator (register A) are rotated left
///              1-bit position. The sign bit (bit 7) is copied to the Carry flag
///              and also to bit 0. Bit 0 is the least-significant bit.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is reset
///     P/V is not affected
///     N is reset
///     C is data from bit 7 of Accumulator
/// </pre>
///
/// \retval none
///
void
Z80::op_rlc_a(void)
{
    BYTE i = (A & 0x80) ? 1 : 0;

    COND_FLAGS((i), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    A <<= 1;
    A  |= i;
}

///
/// \brief RRCA
///
/// <pre>
/// Operation:         +<-------------+
///                    +-->[7 >> 0]-->+--> [CY]
///                        [- A - ]
///
/// Opcode: RRCA
/// Operands: -
///
///           +-+-+-+-+-+-+-+-+
///           |0|0|0|0|1|1|1|1| 0x0F
///           +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator (register A) are rotated right
///              1-bit position. Bit 0 is copied to the Carry flag and also
///              to bit 7. Bit 0 is the least-significant bit.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is reset
///     P/V is not affected
///     N is reset
///     C is data from bit 0 of Accumulator
/// </pre>
///
/// \retval none
///
void
Z80::op_rrc_a(void)
{
    BYTE i = (A & 1) ? 0x80 : 0x00;

    COND_FLAGS((i), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    A >>= 1;
    A  |= i;
}

///
/// \brief RLA
///
/// <pre>
/// Operation:    +-----------------------+
///               +---[7 << 0]<---[CY]<---+
///                    - A -
///
/// Opcode: RLA
/// Operands: -
///
///           +-+-+-+-+-+-+-+-+
///           |0|0|0|1|0|1|1|1|  0x17
///           +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator (register A) are rotated left
///              1-bit position through the Carry flag. The previous content of
///              the Carry flag is copied to bit 0. Bit 0 is the least-significant
///              bit.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is reset
///     P/V is not affected
///     N is reset
///     C is data from bit 7 of Accumulator
/// </pre>
///
/// \retval none
///
void
Z80::op_rl_a(void)
{
    BYTE i = (CHECK_FLAGS(C_FLAG) ? 0x01 : 0x00);

    COND_FLAGS((A & 128), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    A <<= 1;
    A  |= i;
}

///
/// \brief RRA
///
/// <pre>
/// Operation:         +<----------------------+
///                    +-->[7 >> 0]---->[CY]-->+
///                          - A -
///
/// Opcode: RRA
/// Operands: -
///
///           +-+-+-+-+-+-+-+-+
///           |0|0|0|1|1|1|1|1| 0x1F
///           +-+-+-+-+-+-+-+-+
///
/// Description: The contents of the Accumulator (register A) are rotated right
///              1-bit position through the Carry flag. The previous content of the
///              Carry flag is copied to bit 7. Bit 0 is the least-significant bit.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected:
///     S is not affected
///     Z is not affected
///     H is reset
///     P/V is not affected
///     N is reset
///     C is data from bit 0 of Accumulator
/// </pre>
///
/// \retval none
///
void
Z80::op_rr_a(void)
{
    BYTE i = (CHECK_FLAGS(C_FLAG) ? 0x80 : 0x00);

    COND_FLAGS((A & 1), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    A >>= 1;
    A  |= i;
}

///
/// \brief EX DE, HL
///
/// <pre>
/// Operation:  DE <--> HL
///
/// Opcode: EX
/// Operands: DE, HL
///
///           +-+-+-+-+-+-+-+-+
///           |1|1|1|0|1|0|1|1| 0xEB
///           +-+-+-+-+-+-+-+-+
///
/// Description: The 2-byte contents of register pairs DE and HL are exchanged.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_ex_de_hl(void)
{
    WORD tmp;

    tmp = HL;
    HL  = DE;
    DE  = tmp;
}

///
/// \brief EX AF, AF'
///
/// <pre>
/// Operation:  AF <--> AF'
///
/// Opcode: EX
/// Operands: AF, AF'
///
///           +-+-+-+-+-+-+-+-+
///           |0|0|0|0|1|0|0|0| 0x08
///           +-+-+-+-+-+-+-+-+
///
/// Description: The 2-byte contents of register pairs AF and AF' are exchanged.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_ex_af_af(void)
{
    WORD tmp;

    tmp = AF;
    AF  = _af;
    _af = tmp;
}

///
/// \brief EXX
///
/// <pre>
/// Operation:  BC <--> BC', DE <--> DE', HL <--> HL'
///
/// Opcode: EXX
/// Operands: --
///
///           +-+-+-+-+-+-+-+-+
///           |1|1|0|1|1|0|0|1| 0xD9
///           +-+-+-+-+-+-+-+-+
///
/// Description: Each 2-byte value in register pairs BC, DE, and HL is exchanged with the
///              2-byte value in BC', DE', and HL', respectively.
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               1          4          1.00
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_exx(void)
{
    WORD tmp;

    tmp = BC;
    BC  = _bc;
    _bc = tmp;

    tmp = DE;
    DE  = _de;
    _de = tmp;

    tmp = HL;
    HL  = _hl;
    _hl = tmp;
}

///
/// \brief EX (SP), HL
///
/// <pre>
/// Operation:  H <--> (SP+1), L <--> (SP)
///
/// Opcode: EX
/// Operands: (SP), HL
///
///           +-+-+-+-+-+-+-+-+
///           |1|1|1|0|0|0|1|1| 0xE3
///           +-+-+-+-+-+-+-+-+
///
/// Description: The low order byte contained in register pair HL is exchanged with the
///              contents of the memory address specified by the contents of register pair
///              SP (Stack Pointer), and the high order byte of HL is exchanged with the
///              next highest memory address (SP+1).
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               5    19(4,3,4,3,5)    4.75
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_ex_isp_hl(void)
{
    WORD i = readWord(SP);

    writeWord(SP, HL);
    HL     = i;

    ticks -= 3;
}

///
/// \brief PUSH qq
///
/// <pre>
/// Operation:  (SP-2) <-- qqL, (SP-1) <-- qqH
///
/// Opcode: PUSH
/// Operands: qq
///
///           +-+-+-+-+-+-+-+-+
///           |1|1|q|q|0|1|0|1|
///           +-+-+-+-+-+-+-+-+
///
///             +----+--+
///             |Pair|qq|
///             +----+--+
///             | BC |00|
///             | DE |01|
///             | HL |10|
///             | AF |11|
///             +----+--+
///
/// Description: The contents of the register pair qq are pushed to the external memory
///              LIFO (last-in, first-out) Stack. The Stack Pointer (SP) register pair
///              holds the 16-bit address of the current top of the Stack. This instruction
///              first decrements SP and loads the high order byte of the register pair qq
///              to the memory address specified by the SP. The SP is decremented again and
///              loads the low order byte of qq to the memory location corresponding to this
///              new address in the SP. The operand qq identifies register pair BC, DE, HL,
///              or AF, assembled as follows in the object code:
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               3       11(5,3,3)    2.75
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_push_xx(void)
{
    PUSH(getReg16qq(lastInstByte >> 4));

    ticks -= 1;
}

///
/// \brief POP qq
///
/// <pre>
/// Operation:  qqH <-- (SP+1), qqL <-- (SP)
///
/// Opcode: POP
/// Operands: qq
///
///           +-+-+-+-+-+-+-+-+
///           |1|1|q|q|0|1|0|1|
///           +-+-+-+-+-+-+-+-+
///
///             +----+--+
///             |Pair|qq|
///             +----+--+
///             | BC |00|
///             | DE |01|
///             | HL |10|
///             | AF |11|
///             +----+--+
///
/// Description: The top two bytes of the external memory LIFO (last-in, first-out) Stack
///              are popped to the register pair qq. The Stack Pointer (SP) register pair
///              holds the 16-bit address of the current top of the Stack. This instruction
///              first loads to the low order portion of qq, the byte at memory location
///              corresponding to the contents of SP; then SP is incremented and the
///              contents of the corresponding adjacent memory location are loaded to the
///              high order portion of qq and the SP is incremented again. The operand qq
///              identifies register pair BC, DE, HL, or AF, assembled as follows in the
///              object code:
///
///            M Cycles  T States    4 MHz E.T.
///            --------+-----------+------------
///               3       10(4,3,3)    2.50
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_pop_xx(void)
{
    POP(getReg16qq(lastInstByte >> 4));
}

/// \brief Jump
///
/// \todo - document JP.
///
/// \retval none
///
void
Z80::op_jp(void) // JP
{
    PC = READnn();
}

/// \retval none
///
void
Z80::op_jp_hl(void) // JP HL - manual states JP (HL), but this is wrong.
{
    PC = getHLReg16Val();;
}

/// \retval none
///
void
Z80::op_jr(void) // JR
{
    SBYTE rjmp = sREADn();

    // Note: do NOT try to optimize this into a single PC += sREADn(); does not work.
    PC    += rjmp;

    ticks -= 5;
}

/// \retval none
///
void
Z80::op_djnz(void) // DJNZ
{
    if (--B)
    {
        ticks -= 3;
        op_jr();
    }
    else
    {
        PC++;

        ticks -= 4;
    }
}

/// \brief CALL
///
/// \retval none
///
void
Z80::op_call(void)
{
    WORD i = READnn();

    PUSH(PC);
    PC     = i;

    ticks -= 5;
}

/// \brief RET
///
/// \retval none
///
void
Z80::op_ret(void)
{
    POP(PC);
}

/// \retval none
///
void
Z80::op_jp_cc(void)
{
    if (checkCondition(curInst[0]))
    {
        PC = READnn();
    }
    else
    {
        PC += 2;
    }
}

/// \todo - document CALL
///
/// \retval none
///
void
Z80::op_call_cc(void)
{
    if (checkCondition(curInst[0]))
    {
        WORD i = READnn();

        PUSH(PC);
        PC     = i;

        ticks -= 5;
    }
    else
    {
        PC    += 2;

        ticks -= 6;
    }
}

/// \todo - document RET
///
/// \retval none
///
void
Z80::op_ret_cc(void)
{
    ticks -= 1;

    if (checkCondition(curInst[0]))
    {
        POP(PC);
    }
}

/// \todo - document JR
///
/// \retval none
///
void
Z80::op_jr_z(void) // JR Z,n
{
    if (CHECK_FLAGS(Z_FLAG))
    {
        op_jr();
    }
    else
    {
        PC++;

        ticks -= 3;
    }
}

/// \todo - FIXME ???
///
/// \retval none
///
void
Z80::op_jr_nz(void) // JR NZ,n
{
    if (!CHECK_FLAGS(Z_FLAG))
    {
        op_jr();
    }
    else
    {
        PC++;

        ticks -= 3;
    }
}

/// \retval none
///
void
Z80::op_jr_c(void) // JR C,n
{
    if (CHECK_FLAGS(C_FLAG))
    {
        op_jr();
    }
    else
    {
        PC++;

        ticks -= 3;
    }
}

/// \retval none
///
void
Z80::op_jr_nc(void) // JR NC,n
{
    if (!CHECK_FLAGS(C_FLAG))
    {
        op_jr();
    }
    else
    {
        PC++;

        ticks -= 3;
    }
}

///
/// \brief RST
///
/// <pre>
/// Operation: (SP-1) <- PCH, (SP-2) <- PCL, PCH <- 0, PCL <- P
///
/// Opcode:  RST
/// Operands: p
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|  t  |1|1|1|
///          +-+-+-+-+-+-+-+-+
///
///           +------+-----+
///           |   p  |  t  |
///           +------+-----+
///           | 0x00 | 000 |
///           | 0x08 | 001 |
///           | 0x10 | 010 |
///           | 0x18 | 011 |
///           | 0x20 | 100 |
///           | 0x28 | 101 |
///           | 0x30 | 110 |
///           | 0x38 | 111 |
///           +------+-----+
///
/// Description: The current Program Counter (PC) contents are pushed onto the external
///              memory stack, and the page zero memory location given by operand p is
///              loaded to the PC. Program execution then begins with the Opcode in the
///              address now pointed to by PC. The push is performed by first decrementing
///              the contents of the Stack Pointer (SP), loading the high-order byte of PC to
///              the memory address now pointed to by SP, decrementing SP again, and
///              loading the low order byte of PC to the address now pointed to by SP. The
///              Restart instruction allows for a jump to one of eight addresses indicated in
///              the table below. The operand p is assembled to the object code using the
///              corresponding T state.
///
///              Because all addresses are in page zero of memory, the high order byte of
///              PC is loaded with 00H. The number selected from the p column of the table
///              is loaded to the low order byte of PC.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    3      11(5,3,3)      2.75
///
/// Condition Bits Affected: None
/// </pre>
///
/// \retval none
///
void
Z80::op_rst(void)
{
    PUSH(PC);

    PC     = (lastInstByte & 0x38);

    ticks -= 1;
}


///
/// CB Prefix instructions
///
/// \retval none
///
void
Z80::op_cb_handle(void)
{

    switch (prefix)
    {
        case ip_dd:
        case ip_fd:
            lastInstByte          = curInst[2] = readInst();
            sW                    = (signed char) curInst[2];

            xxcb_effectiveAddress = getIxReg16Val() + sW;

            lastInstByte          = curInst[3] = readInst();

            (this->*op_xxcb[curInst[3] >> 3])();
            break;

        case ip_none:
        default:
            lastInstByte = curInst[1] = readInst();

            (this->*op_cb[curInst[1]])();
            break;
    }
}

/// \todo - document SRL - Shift Right Logical
///
/// \retval none
///
void
Z80::op_srl(BYTE& m)
{
    COND_FLAGS((m & 1), C_FLAG);

    m >>= 1;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_srl_x(void)
{
    op_srl(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_srl_ihl(void) // SRL (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_srl(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document SLA - Shift Left Arithmetic
///
/// \retval none
///
void
Z80::op_sla(BYTE& m)
{
    COND_FLAGS((m & 0x80), C_FLAG);

    m <<= 1;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_sla_x(void)
{
    op_sla(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_sla_ihl(void) // SLA (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_sla(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - undocumented SLL - Shift Left Logical
/// same as SLA but bit 0 is set.
/// UNVERIFIED - coded based on "The Undocumented Z80 Documented", vers. 0.91
///
/// \retval none
///
void
Z80::op_undoc_sll(BYTE& m) // SLL m
{
    COND_FLAGS((m & 0x80), C_FLAG);

    m <<= 1;
    m  |= 1;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

///
/// \retval none
///
void
Z80::op_sll_x(void)
{
    op_undoc_sll(getReg8(lastInstByte));
}

/// SLL (HL)
/// \retval none
///
void
Z80::op_sll_ihl(void)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_undoc_sll(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document RL - Rotate Left
///
/// \retval none
///
void
Z80::op_rl(BYTE& m) // RL m
{
    BYTE carry = CHECK_FLAGS(C_FLAG) ? 0x01 : 0x00;

    COND_FLAGS((m & 128), C_FLAG);

    m <<= 1;
    m  |= carry;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_rl_x(void)
{
    op_rl(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_rl_ihl(void) // RL (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_rl(val);
    writeMEM(addr, val);

    ticks -= 1;
}

///
/// \todo - document RR - Rotate Right
///
/// \retval none
///
void
Z80::op_rr(BYTE& m) // RR m
{
    BYTE carry = CHECK_FLAGS(C_FLAG) ? 0x80 : 0x00;

    COND_FLAGS((m & 1), C_FLAG);

    m >>= 1;
    m  |= carry;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_rr_x(void)
{
    op_rr(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_rr_ihl(void) /* RR (HL) */
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_rr(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo document RRC - Rotate Right Circular
///
/// \retval none
///
void
Z80::op_rrc(BYTE& m) /* RRC m */
{
    BYTE carry = (m & 0x01) ? 0x80 : 0x00;

    COND_FLAGS((carry), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    m >>= 1;
    m  |= carry;

    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_rrc_x(void)
{
    op_rrc(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_rrc_ihl(void) // RRC (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_rrc(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document RLC - Rotate Left Circular
///
/// \retval none
///
void
Z80::op_rlc(BYTE& m) // RLC m
{
    BYTE carry = (m & 0x80) ? 0x01 : 0x00;

    COND_FLAGS((carry), C_FLAG);
    CLEAR_FLAGS(H_FLAG | N_FLAG);

    m <<= 1;
    m  |= carry;

    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_rlc_x(void)
{
    op_rlc(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_rlc_ihl(void) // RLC (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_rlc(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document SRA - Shift Right Arithmetic
/// \retval none
///
void
Z80::op_sra(BYTE& m) // SRA m
{
    BYTE i = m & 0x80;

    COND_FLAGS((m & 1), C_FLAG);

    m >>= 1;
    m  |= i;

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(m);
}

/// \retval none
///
void
Z80::op_sra_x(void)
{
    op_sra(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_sra_ihl(void) // SRA (HL)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_sra(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document SET
/// \retval none
///
void
Z80::op_sb_n(BYTE& val)
{
    val |= getBit(lastInstByte);
}

/// \retval none
///
void
Z80::op_sb_n_x(void)
{
    op_sb_n(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_sb_n_ihl(void)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_sb_n(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \todo - document RES
///
/// \retval none
///
void
Z80::op_rb_n(BYTE& val)
{
    val &= ~getBit(lastInstByte);
}

/// \retval none
///
void
Z80::op_rb_n_x(void)
{
    op_rb_n(getReg8(lastInstByte));
}

/// \retval none
///
void
Z80::op_rb_n_ihl(void)
{
    WORD addr = getIndirectAddr();
    BYTE val;

    val = readMEM(addr);
    op_rb_n(val);
    writeMEM(addr, val);

    ticks -= 1;
}

/// \retval none
///
void
Z80::op_tb_n(BYTE m,
             BYTE bit)
{
    CLEAR_FLAGS(N_FLAG);
    SET_FLAGS(H_FLAG);

    m &= bit;

    COND_FLAGS((!(m)), (Z_FLAG | P_FLAG));
    COND_FLAGS((m & 0x80), S_FLAG);
}

/// \retval none
///
void
Z80::op_tb_n_x(void)
{
    op_tb_n(getReg8(lastInstByte), getBit(lastInstByte));
}

/// \retval none
///
void
Z80::op_tb_n_ihl(void)
{
    op_tb_n(readMEM(getIndirectAddr()), getBit(lastInstByte));

    ticks -= 1; // read handles 3 of the ticks, just have to have 1 here.
}


///
/// DD Prefix instructions
///
///
/// \retval none
///
void
Z80::op_dd_handle(void)
{
    prefix       = ip_dd;

    lastInstByte = curInst[1] = readInst();

    (this->*op_code[curInst[1]])();
}

///
/// ED Prefix instructions.
///
/// \retval none
///
void
Z80::op_ed_handle(void)
{
    lastInstByte = curInst[1] = readInst();

    (this->*op_ed[curInst[1]])();
}

///
/// \brief ED prefixed NOP
///
/// Many 2 byte instructions with an ED prefix do not
/// perform any operation, but just act as 2 NOP instructions
///
/// \retval none
///
void
Z80::op_ed_nop(void)
{
    // NOP - do nothing.

    // It's like 2 NOP instructions..
}

///
/// \brief Interrupt Mode 0
///
/// <pre>
/// Operation: -
///
/// Opcode: IM
/// Operands: 0
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|0|0|1|1|0|  0x46
///          +-+-+-+-+-+-+-+-+
///
/// Description: The IM 0 instruction sets interrupt mode 0. In this mode, the interrupting
///              device can insert any instruction on the data bus for execution by the
///              CPU. The first byte of a multi-byte instruction is read during the interrupt
///              acknowledge cycle. Subsequent bytes are read in by a normal memory
///              read sequence.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        8(4,4)       2.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_im0(void)
{
    IM = 0;
}

///
/// \brief Interrupt Mode 1
///
/// <pre>
/// Operation: -
///
/// Opcode: IM
/// Operands: 1
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|1|0|1|1|0|  0x56
///          +-+-+-+-+-+-+-+-+
///
/// Description: The IM 1 instruction sets interrupt mode 1. In this mode, the processor
///              responds to an interrupt by executing a restart to location 0038H.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        8(4,4)       2.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_im1(void)
{
    IM = 1;
}

///
/// \brief Interrupt Mode 2
///
/// <pre>
/// Operation: -
///
/// Opcode: IM
/// Operands: 2
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|1|1|1|1|0|  0x5E
///          +-+-+-+-+-+-+-+-+
///
/// Description: The IM 2 instruction sets the vectored interrupt mode 2. This mode allows
///              an indirect call to any memory location by an 8-bit vector supplied from the
///              peripheral device. This vector then becomes the least-significant eight bits
///              of the indirect pointer, while the I register in the CPU provides the most-
///              significant eight bits. This address points to an address in a vector table
///              that is the starting address for the interrupt service routine.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        8(4,4)       2.00
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_im2(void)
{
    IM = 2;
}

///
/// \brief Return from Interrupt
///
/// <pre>
/// Operation: Return from Interrupt
///
/// Opcode: RETI
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|0|1|1|0|1|  0x4D
///          +-+-+-+-+-+-+-+-+
///
/// Description: This instruction is used at the end of a maskable interrupt service routine
///              to:
///              - Restore the contents of the Program Counter (PC) (analogous to the
///                RET instruction)
///              - Signal an I/O device that the interrupt routine is completed. The RETI
///                instruction also facilitates the nesting of interrupts, allowing higher
///                priority devices to temporarily suspend service of lower priority
///                service routines. However, this instruction does not enable interrupts
///                that were disabled when the interrupt routine was entered. Before
///                doing the RETI instruction, the enable interrupt instruction (EI)
///                should be executed to allow recognition of interrupts after completion
///                of the current service routine.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    4     14(4,4,3,3)     3.50
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_reti(void)
{
    POP(PC);
}

///
/// \brief Return from non-maskable interrupt
///
/// <pre>
/// Operation: Return from non maskable interrupt
///
/// Opcode: RETN
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|0|0|1|0|1|  0x45
///          +-+-+-+-+-+-+-+-+
///
/// Description: This instruction is used at the end of a non-maskable interrupts service
///              routine to restore the contents of the Program Counter (PC) (analogous to
///              the RET instruction). The state of IFF2 is copied back to IFF1 so that
///              maskable interrupts are enabled immediately following the RETN if they
///              were enabled before the non-maskable interrupt.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    4     14(4,4,3,3)     3.50
///
/// Condition Bits Affected: none
/// </pre>
///
/// \retval none
///
void
Z80::op_retn(void)
{
    POP(PC);
    IFF0 = IFF1 = IFF2;
}


///
/// \brief Negate
///
/// <pre>
/// Operation: A <- 0 - A
///
/// Opcode: NEG
///
///          +-+-+-+-+-+-+-+-+
///          |1|1|1|0|1|1|0|1|  0xED
///          +-+-+-+-+-+-+-+-+
///          |0|1|0|0|0|1|0|0|  0x44
///          +-+-+-+-+-+-+-+-+
///
///
/// Description: The contents of the Accumulator are negated (two's complement). This is
///              the same as subtracting the contents of the Accumulator from zero. Note
///              that 80H is left unchanged.
///
/// M Cycles  T States    4 MHz E.T.
/// --------+-----------+------------
///    2        8(4,4)       2.00
///
/// Condition Bits Affected:
///     S is set if result is negative; reset otherwise
///     Z is set if result is 0; reset otherwise
///     H is set if borrow from bit 4; reset otherwise
///     P/V is set if Accumulator was 80H before operation; reset otherwise
///     N is set
///     C is set if Accumulator was not 00H before operation; reset otherwise
/// </pre>
///
/// \retval none
///
void
Z80::op_neg(void)
{
    COND_FLAGS((A), C_FLAG);
    COND_FLAGS((A == 0x80), P_FLAG);
    COND_FLAGS((A & 0xf), H_FLAG);

    A = 0 - A;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);
}

///
/// \brief IN val, (C)
///
/// \retval none
///
void
Z80::op_in_ic(BYTE& reg)
{
    reg = (h89.getIO()).in(C);

    CLEAR_FLAGS(N_FLAG | H_FLAG);
    SET_ZSP_FLAGS(reg);

    ticks -= 4;
}

///
/// \brief IN x,(C)
///
/// \retval none
///
void
Z80::op_in_x_c(void)
{
    op_in_ic(getReg8(lastInstByte >> 3));
}

///
/// \brief IN F, (C)
///
/// This instruction only sets the flags, it doesn't store the value
///
/// \retval none
///
void
Z80::op_in_f_ic(void)
{
    BYTE tmp;

    op_in_ic(tmp);
}

///
/// \brief OUT (C),x
///
/// \retval none
///
void
Z80::op_out_c_x(void)
{
    (h89.getIO()).out(C, getReg8Val(lastInstByte >> 3));

    ticks -= 4;
}

///
/// \brief OUT (C),0
///
/// \retval none
///
void
Z80::op_out_c_0(void)
{
    (h89.getIO()).out(C, 0);

    ticks -= 4;
}

///
/// \brief INI
///
/// \retval none
///
void
Z80::op_ini(void) // INI
{
    writeMEM(HL, (h89.getIO()).in(C));

    HL++;
    B--;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((!B), Z_FLAG);

    ticks -= 5;
}

///
/// \brief INIR
///
/// \retval none
///
void
Z80::op_inir(void)
{
    op_ini();

    if (B)
    {
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief IND
///
/// \retval none
///
void
Z80::op_ind(void)
{
    writeMEM(HL, (h89.getIO()).in(C));

    HL--;
    B--;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((!B), Z_FLAG);

    ticks -= 5;
}

///
/// \brief INDR
///
/// \retval none
///
void
Z80::op_indr(void)
{
    op_ind();

    if (B)
    {
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief OUTI
///
/// \retval none
///
void
Z80::op_outi(void)
{
    (h89.getIO()).out(C, readMEM(HL));

    HL++;
    B--;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((!B), Z_FLAG);

    ticks -= 5;
}

///
/// \brief OTIR
///
/// \retval none
///
void
Z80::op_otir(void)
{
    op_outi();

    if (B)
    {
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief OUTD
///
/// \retval none
///
void
Z80::op_outd(void)
{
    (h89.getIO()).out(C, readMEM(HL));

    HL--;
    B--;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((!B), Z_FLAG);

    ticks -= 5;
}

///
/// \brief OTDR
///
/// \retval none
///
void
Z80::op_otdr(void)
{
    op_outd();

    if (B)
    {
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief LD A,I
///
/// \retval none
///
void
Z80::op_ld_a_i(void)
{
    A = I;

    CLEAR_FLAGS(N_FLAG | H_FLAG);
    COND_FLAGS((IFF2), P_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);

    ticks -= 1;
}

///
/// \brief LD A,R
///
/// \retval none
///
void
Z80::op_ld_a_r(void)
{
    A = (BYTE) R;

    CLEAR_FLAGS(N_FLAG | H_FLAG);
    COND_FLAGS((IFF2), P_FLAG);
    COND_FLAGS((!A), Z_FLAG);
    COND_FLAGS((A & 0x80), S_FLAG);

    ticks -= 1;
}

///
/// \brief LD I,A
///
/// \retval none
///
void
Z80::op_ld_i_a(void)
{
    I      = A;

    ticks -= 1;
}

///
/// \brief LD R,A
///
/// \retval none
///
void
Z80::op_ld_r_a(void)
{
    R      = A;

    ticks -= 1;
}

///
/// \brief LD xx,(nn)
///
/// \retval none
///
void
Z80::op_ld_xx_inn(void)
{
    getReg16(lastInstByte >> 4) = readWord(READnn());
}

///
/// \brief LD (nn),xx
///
/// \retval none
///
void
Z80::op_ld_inn_xx(void)
{
    writeWord(READnn(), getReg16Val(lastInstByte >> 4));
}

/// \brief ADC HL,xx
///
///
///
/// \retval none
///
void
Z80::op_adc16(WORD op)
{
    WORD  carry = (CHECK_FLAGS(C_FLAG)) ? 1 : 0;
    SWORD sOp   = op;
    int   i;

    COND_FLAGS(((HL & 0x0fff) + (op & 0x0fff) + carry) > 0x0fff, H_FLAG);

    i = sHL + sOp + carry;

    COND_FLAGS((i > 32767) || (i < -32768), P_FLAG);
    COND_FLAGS((HL + op + carry) & 0x10000, C_FLAG);

    i &= 0xffff;

    COND_FLAGS(!i, Z_FLAG);
    COND_FLAGS(i & 0x8000, S_FLAG);
    CLEAR_FLAGS(N_FLAG);

    HL     = i;

    ticks -= 7;
}


///
/// \brief ADC HL,xx
///
/// \retval none
///
void
Z80::op_adc_hl_xx(void)
{
    op_adc16(getCoreReg16Val(lastInstByte >> 4));
}


///
/// \brief
///
/// \retval none
///
void
Z80::op_sbc16(WORD op)
{
    WORD  carry = (CHECK_FLAGS(C_FLAG)) ? 1 : 0;
    SWORD sOp   = op;
    int   i;

    COND_FLAGS(((op & 0x0fff) + carry) > (HL & 0x0fff), H_FLAG);

    i = sHL - sOp - carry;

    COND_FLAGS((i > 32767) || (i < -32768), P_FLAG);
    COND_FLAGS((op + carry) > HL, C_FLAG);

    i &= 0xffff;

    COND_FLAGS(!i, Z_FLAG);
    COND_FLAGS(i & 0x8000, S_FLAG);
    SET_FLAGS(N_FLAG);

    HL     = i;

    ticks -= 7;
}

///
/// \brief SBC HL,xx
///
/// \retval none
///
void
Z80::op_sbc_hl_xx(void)
{
    op_sbc16(getCoreReg16Val(lastInstByte >> 4));
}

///
/// \brief LDI
///
/// \retval none
///
void
Z80::op_ldi(void)
{
    writeMEM(DE++, readMEM(HL++));

    COND_FLAGS((--BC), P_FLAG);
    CLEAR_FLAGS(N_FLAG | H_FLAG);

    ticks -= 2;
}

///
/// \brief LDIR
///
/// \retval none
///
void
Z80::op_ldir(void)
{
    op_ldi();

    if (BC)
    {
        // BC not zero, repeat instruction.
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief LDD
///
/// \retval none
///
void
Z80::op_ldd(void)
{
    writeMEM(DE--, readMEM(HL--));

    COND_FLAGS((--BC), P_FLAG);
    CLEAR_FLAGS(N_FLAG | H_FLAG);

    ticks -= 2;
}

///
/// \brief LDDR
///
/// \retval none
///
void
Z80::op_lddr(void)
{
    op_ldd();

    if (BC)
    {
        // BC not zero, repeat instruction.
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief CPI
///
/// \retval none
///
void
Z80::op_cpi(void)
{
    BYTE i;

    i = readMEM(HL++);

    COND_FLAGS((i & 0xf) > (A & 0xf), H_FLAG);

    i = A - i;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((--BC), P_FLAG);
    COND_FLAGS((!i), Z_FLAG);
    COND_FLAGS((i & 0x80), S_FLAG);

    ticks -= 5;
}

///
/// \brief CPIR
///
/// \retval none
///
void
Z80::op_cpir(void)
{
    op_cpi();

    if ((BC) && (!CHECK_FLAGS(Z_FLAG)))
    {
        // not a match and BC not zero, retry instruction.
        PC    -= 2;

        ticks -= 5;
    }
}


///
/// \brief CPD
///
/// From The Undocumented Z80 Documented
/// SF, ZF, HF flags - Set by the hypothetical CP (HL).
/// PF flag Set if BC is not 0.
/// NF flag Always set.
/// CF flag Unchanged.
///
/// \retval none
///
void
Z80::op_cpd(void)
{
    BYTE i;

    i = readMEM(HL--);

    COND_FLAGS((i & 0xf) > (A & 0xf), H_FLAG);

    i = A - i;

    SET_FLAGS(N_FLAG);
    COND_FLAGS((--BC), P_FLAG);
    COND_FLAGS((!i), Z_FLAG);
    COND_FLAGS((i & 0x80), S_FLAG);

    ticks -= 5;
}

///
/// \brief CPDR
///
/// \retval none
///
void
Z80::op_cpdr(void)
{
    op_cpd();

    if ((BC) && (!CHECK_FLAGS(Z_FLAG)))
    {
        // not a match and BC not zero, retry instruction.
        PC    -= 2;

        ticks -= 5;
    }
}

///
/// \brief RLD (HL)
///
/// \todo - document RLD
/// \retval none
///
void
Z80::op_rld_ihl(void)
{
    BYTE i, j;
    WORD addr = getIndirectAddr();

    i = readMEM(addr);
    j = A & 0x0f;
    A = (A & 0xf0) | (i >> 4);
    i = (i << 4) | j;
    writeMEM(addr, i);

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(A);

    ticks -= 4;
}

///
/// \brief RRD (HL)
///
/// \todo - document RRD
/// \retval none
///
void
Z80::op_rrd_ihl(void)
{
    BYTE i, j;
    WORD addr = getIndirectAddr();

    i = readMEM(addr);
    j = A & 0x0f;
    A = (A & 0xf0) | (i & 0x0f);
    i = (i >> 4) | (j << 4);
    writeMEM(addr, i);

    CLEAR_FLAGS(H_FLAG | N_FLAG);
    SET_ZSP_FLAGS(A);

    ticks -= 4;
}

//
// FD Prefix Instructions
//

///
/// \brief
///
///
/// \retval none
///
void
Z80::op_fd_handle(void)
{

    prefix       = ip_fd;
    lastInstByte = curInst[1] = readInst();

    (this->*op_code[curInst[1]])();
}

//
// Prefix DD CB or FD CB
//

///
/// \brief BIT n,(Ix+d)
///
/// \retval none
///
void
Z80::op_tb_n_xx_d(void)
{
    op_tb_n(readMEM(xxcb_effectiveAddress), getBit(curInst[3]));

    ticks -= 1;
}


///
/// \brief RES n,(Ix+d)
///
/// \retval none
///
void
Z80::op_rb_n_xx_d(void)
{
    BYTE val;

    val  = readMEM(xxcb_effectiveAddress);
    val &= ~getBit(curInst[3]);
    writeMEM(xxcb_effectiveAddress, val);

    // The undocumented function of also storing it to one of the register.
    if ((curInst[3] & 0x7) != 0x06)
    {
        getCoreReg8(curInst[3]) = val;
    }

    ticks -= 1;
}

///
/// \brief SET n,(Ix+d)
///
/// \retval none
///
void
Z80::op_sb_n_xx_d(void)
{
    BYTE val;

    val  = readMEM(xxcb_effectiveAddress);
    val |= getBit(curInst[3]);
    writeMEM(xxcb_effectiveAddress, val);

    // The undocumented function of also storing it to one of the register.
    if ((curInst[3] & 0x7) != 0x06)
    {
        getCoreReg8(curInst[3]) = val;
    }

    ticks -= 1;
}

/// Generic helper routine for both the DDCB and FDCB instructions.
///
void
Z80::op_xxx_xx_d(xd_cbMethod operation)
{
    BYTE val;

    val = readMEM(xxcb_effectiveAddress);
    (this->*(operation))(val);
    writeMEM(xxcb_effectiveAddress, val);

    // The undocumented function of also storing it to one of the register.
    if ((curInst[3] & 0x7) != 0x06)
    {
        getCoreReg8(curInst[3]) = val;
    }

    ticks -= 1;
}


///
/// \brief RLC (Ix+d)
///
/// \retval none
///
void
Z80::op_rlc_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_rlc);
}

///
/// \brief RRC (Ix+d)
///
/// \retval none
///
void
Z80::op_rrc_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_rrc);
}

///
/// \brief RL (Ix+d)
///
/// \retval none
///
void
Z80::op_rl_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_rl);
}

///
/// \brief RR (Ix+d)
///
/// \retval none
///
void
Z80::op_rr_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_rr);
}

///
/// \brief SLA (Ix+d)
///
/// \retval none
///
void
Z80::op_sla_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_sla);
}

///
/// \brief SRA (Ix+d)
///
/// \retval none
///
void
Z80::op_sra_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_sra);
}

///
/// \brief SRL (Ix+d)
///
/// \retval none
///
void
Z80::op_srl_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_srl);
}

///
/// \brief SLL (Ix+d)
///
/// \retval none
///
void
Z80::op_sll_xx_d(void)
{
    op_xxx_xx_d(&Z80::op_undoc_sll);
}
