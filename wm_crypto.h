#ifndef WM_CRYPTO_H
#define WM_CRYPTO_H

#include <stdint.h>

uint8_t ans_tbl[7][6];
uint8_t sboxes[10][256];

uint8_t ft[8];
uint8_t sb[8];

void generate_tables();

#endif
