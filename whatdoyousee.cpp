#ifndef _WDYS_S_
#define _WDYS_S_

#define _WDYS_S_ID "$Id$"

#include <stdio.h>
#include <string.h>

#include "whatdoyousee.h"

//xTEA (Tiny encryption algorithm)
//Decode function, thanks to Anonymous54321 for it
void decodeQuad(unsigned int *first, unsigned int *second) {
unsigned int out1 = *first, out2 = *second, base = 0xc6ef3720;
unsigned int temp1, temp2, crossRef[4];
int index;

crossRef[0] = 0x6c0a5452;
crossRef[1] = 0x03827d0f;
crossRef[2] = 0x3a170b92;
crossRef[3] = 0x16db7fc2;

for (index = 0x20; index>0; index--) {
// First word
temp1 = temp2 = out1;
temp1 >>= 5;
temp2 <<= 4;
temp1 ^= temp2;
temp1 += out1;

temp2 = base;
temp2 >>= 0x0b;
temp2 &= 0x03;

temp2 = crossRef[temp2];
temp2 += base;
base += 0x61c88647;

temp1 ^= temp2;
out2 -= temp1;

// Second word
temp1 = temp2 = out2;
temp1 >>= 5;
temp2 <<= 4;
temp1 ^= temp2;
temp1 += out2;

temp2 = base;
temp2 &= 0x03;

temp2 = crossRef[temp2];
temp2 += base;

temp1 ^= temp2;
out1 -= temp1;
}

*first = out1;
*second = out2;

}

void encodeQuad(unsigned int *first, unsigned int *second)
{
unsigned int out1 = *first, out2 = *second, base = 0;
unsigned int temp1, temp2, crossRef[4];
int index;

crossRef[0] = 0x6c0a5452;
crossRef[1] = 0x03827d0f;
crossRef[2] = 0x3a170b92;
crossRef[3] = 0x16db7fc2;

for (index = 0x20; index>0; index--) {
// Second word
temp1 = temp2 = out2;
temp1 >>= 5;
temp2 <<= 4;
temp1 ^= temp2;
temp1 += out2;

temp2 = base;
temp2 &= 0x03;

temp2 = crossRef[temp2];
temp2 += base;
base -= 0x61c88647;

temp1 ^= temp2;
out1 += temp1;


// First word
temp1 = temp2 = out1;
temp1 >>= 5;
temp2 <<= 4;
temp1 ^= temp2;
temp1 += out1;

temp2 = base;
temp2 >>= 0x0b;
temp2 &= 0x03;

temp2 = crossRef[temp2];
temp2 += base;

temp1 ^= temp2;
out2 += temp1;

}

*first = out1;
*second = out2;

}

#endif

