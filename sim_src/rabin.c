/*
 * This program implements the rabin fingerprint checksum computation.
 * At this point, the code works only for normalized minimal polynomial
 * (i.e. MSB=1). If this polynomial will ever be made un-normalized,
 * the source code needs to be modified
 */

/*
 * Let f/P = g + r/P, where P is a 64-bit minimal polynomial,
 * '/' stands for the modulo P operation, and f is a windowed polynomial (of W bytes).
 * Given f = [ om ...... ] and the modulo P fingerprint r, the problem of interest
 * is how to derive the modulo P fingerprint for the next windowed polynomial f' = [ ...... m],
 * om stands for a byte to be shifted out of the window and m stands for a new byte into the window.
 * It can be shown that, f'/P = g<<8 + (r<<8 + m + om<<(8*W))/P.
 * Therefore, the modulo P fingerprint r' can be updated according to 
 * r' = (r<<8 + m + om<<(8*W)) / P. By exploiting the linearity of the xor operation,
 * the equation is solved by separating (r<<8 + m + om<<(8*W)) into 3 parts, and xor them to derive r'
 *
 * 1. (om<<(8*W))/P :  this is called old ringing from the shifted out byte om,
 *    and can be pre-computed (in rabin_init) and tabulated (256 entries).
 *    The saved om can be used as an index to the table to determine.
 *
 * 2. shifted out part of r<<8 : it should be noticed that the MSB in r is always 0,
 *    therefore, the fingerprint corresponding to the most significant 9 bits left shifted by 8,
 *    is precomputed and tabulated (256 entries). This fingerprint is called new ringing
 *    and can be computed by using r>>55 as an index to the 2nd table.
 *
 * 3. the rest of (r<<8 + m)&(~MSB64) (clear the MSB)
 *
 * The fingerprint r' can be updated according to 
 *
 * 	r' = old_ringing[om] + new_ringing[r>>55] + (r<<8 + m)&0x7fffffffffffffff; 
 *
 * Our implementation follows this algorithm.
 *
 */ 

#include "rabin.h"

#define MSB64 0x8000000000000000ULL

struct rabin_state *
rabin_init(u_int64_t poly, int size)
{
	int i, j;
	struct rabin_state *s;
	u_int64_t fingerprint;	

	if (!(poly & MSB64)) {
		//DEBUG("Error: non-normalized minimal polynomial is not supported.");
		return NULL;
	}

	s = (struct rabin_state *) malloc(sizeof(*s));
	if (s == NULL)
		return NULL;

	/* allocate the circular buffer for windowed bytes */
	s->size = size;
	s->buf = (u_char *) malloc(size);
	if (s->buf == NULL) {
		free(s);
		return NULL;
	}

	s->poly = poly;

	/*
	 * old_ringing[i] is the remainder in fingerprint
	 * due to an old byte i just slid out of the window.
	 */
	for (i = 0; i < 256; i++) {
		fingerprint = i;
		for (j = 0; j < size * 8; j++) {
			fingerprint <<= 1;
			if (fingerprint & MSB64)
				fingerprint ^= poly;
		}
		s->old_ringing[i] = fingerprint;
	}

	/*
	 * new_ringing[i] is the remainder in fingerprint
	 * due to the 9 most significant bits in fingerprint before update.
	 */
	for (i = 0; i < 256; i++) {
		fingerprint = ((unsigned long long) i) << 55;
		for (j = 0; j < 8; j++) {
			fingerprint <<= 1;
			if (fingerprint & MSB64)
				fingerprint ^= poly;
 		}
		s->new_ringing[i] = fingerprint;
	}

	rabin_reset(s);	// reset the rabin state

	return s;
}

#if defined __i386__ || defined __x86_64__
/* i386/x86_64 hand optimized code in rabin_apl_sub.s */
//extern u_int64_t rabin_slide8(struct rabin_state *s, u_char m);
#else
u_int64_t
rabin_slide8(struct rabin_state *s, u_char m)
{
	u_char om;
	u_int64_t residual, fingerprint;

	/*
	 * Extract the old byte from the circular buffer
	 * and deposit the new byte into the circular buffer.
	 */
	om = *s->bufptr;
	*s->bufptr = m;

	/* update the circular buffer index */
	s->bufptr--;
	if (s->bufptr < s->buf)
		s->bufptr += s->size;

	/* initial fingerprint */
	fingerprint = s->fingerprint;

	residual = s->old_ringing[om];				// old byte ringing
	residual ^= s->new_ringing[fingerprint >> 55];		// + MS byte ringing
	fingerprint = ((fingerprint << 8) | m) ^ residual;	// + new fingerprint due to the new byte
	fingerprint &= ~MSB64;

	/* update fingerprint */
	s->fingerprint = fingerprint;

	return fingerprint;
}
#endif

void
rabin_reset(struct rabin_state *s)
{
	int i;

	s->fingerprint = 0; 
	s->bufptr = s->buf;
	for (i = 0; i < s->size; i++)
		s->buf[i] = 0;
}

void
rabin_free(struct rabin_state *s)
{
	if (s) {
		if (s->buf)
			free(s->buf);
		free(s);
	}
}
