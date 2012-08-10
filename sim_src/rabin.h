/*
 * This program implements the rabin fingerprint checksum computation.
 * At this point, the code works only for normalized minimal polynomial
 * (i.e. MSB=1). If this polynomial will ever be made un-normalized,
 * the source code needs to be modified
 */

#ifndef _RABIN_H_
#define _RABIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>

struct rabin_state {
	u_int64_t	old_ringing[256];	// fingerprint due to ringing from window slided out byte
	u_int64_t	new_ringing[256];	// fingerprint due to ringing from most significant byte in fingerprint
	u_int64_t	fingerprint;		// rabin checksum fingerprint
	int		size;			// window size
	u_char		*bufptr;		// circular buffer pointer
	u_char		*buf;			// circular buffer
	u_int64_t	poly;			// minimal polynomial
};

#define POLY	0xb15e234bd3792f63ull	// Actual polynomial
#define RABIN_WINDOW_SIZE 48		// slide window size

struct rabin_state *rabin_init(u_int64_t, int);
#if defined __i386__ || defined __x86_64__
/* i386/x86_64 had optimized code in rabin_sam.S */
extern u_int64_t rabin_slide8(struct rabin_state *, u_char) asm("_rabin_slide8"); 
#else
u_int64_t rabin_slide8(struct rabin_state *, u_char);
#endif
void rabin_reset(struct rabin_state *); 
void rabin_free(struct rabin_state *);

#endif /* _RABIN_H_ */
