#ifndef __U_WHATDOYOUSEE_H
#define __U_WHATDOYOUSEE_H

#define __U_WHATDOYOUSEE_H_ID "$Id$"

#ifdef __cplusplus
extern "C" {
#endif

//xTEA (Tiny encryption algorithm)
/** Decode function, thanks to Anonymous54321 for it */
void decodeQuad(unsigned int *first, unsigned int *second);
/** Encode function, thanks to Anonymous54321 for it */
void encodeQuad(unsigned int *first, unsigned int *second);

#ifdef __cplusplus
}
#endif

#endif

