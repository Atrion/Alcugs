
#include "alcdefs.h"
#include "whatdoyousee.h"

#include "alcutil/useful.h"

//xTEA (Tiny encryption algorithm)
//Decode function, thanks to Anonymous54321 for it
void decodeQuad(unsigned int *first, unsigned int *second) {
#if defined(NEED_STRICT_ALIGNMENT)
	unsigned int out1, out2, base = 0xc6ef3720;
	memcpy(&out1, first, 4);
	memcpy(&out2, second, 4);
#else
	unsigned int out1 = *first, out2 = *second, base = 0xc6ef3720;
#endif
	out1 = letoh32(out1);
	out2 = letoh32(out2);
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

	out1 = htole32(out1);
	out2 = htole32(out2);
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(first, &out1, 4);
	memcpy(second, &out2, 4);
#else
	*first = out1;
	*second = out2;
#endif
}

void encodeQuad(unsigned int *first, unsigned int *second) {
#if defined(NEED_STRICT_ALIGNMENT)
	unsigned int out1, out2, base = 0;
	memcpy(&out1, first, 4);
	memcpy(&out2, second, 4);
#else
	unsigned int out1 = *first, out2 = *second, base = 0;
#endif
	out1 = letoh32(out1);
	out2 = letoh32(out2);
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
	
	out1 = htole32(out1);
	out2 = htole32(out2);
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(first, &out1, 4);
	memcpy(second, &out2, 4);
#else
	*first = out1;
	*second = out2;
#endif
}


