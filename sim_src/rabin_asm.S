/*
 * This program implements rabin_slide8 function for i386 and x86_64 architectures.
 * The function rabin_slide8 is the core function in the rabin fingerprint checksum computation.
 * At this point, the code works only for normalized minimal polynomial (i.e. MSB=1).
 * If this polynomial will ever be made un-normalized, the source code needs to be modified
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
 * 3. the rest of (r<<8 + m) & (~MSB64) (clear the MSB)
 *
 * The fingerprint r' can be updated according to 
 *
 * 	r' = old_ringing[om] + new_ringing[r>>55] + (r<<8 + m)&0x7fffffffffffffff; 
 *
 * Our implementation follows this algorithm.
 *
 */ 

#if defined __i386__

	.text
	.align 4,0x90

// u_int64_t rabin_slide8(struct rabin_state *s, u_char m)
// {
//    u_char om;
//    u_int64_t residual,fingerprint;
//    om = *s->bufptr;
//    *s->bufptr = m;
//    s->bufptr--;
//    if (s->bufptr < s->buf) s->bufptr += s->size;

//    fingerprint = s->fingerprint;
//    residual = s->old_ringing[om];
//    residual ^= s->new_ringing[fingerprint >> 55];
//    fingerprint = ((fingerprint << 8)|m) ^ residual;
//    fingerprint &= ~MSB64;
//    s->fingerprint = fingerprint;
//    return fingerprint;
// }

    // rabin_state structure (defined in rabin_apl.h)
    //     0  u_int64_t   old_ringing[256];
    //  2048  u_int64_t   new_ringing[256];
    //  4096  u_int64_t   fingerprint;            // rabin checksum fingerprint
    //  4104  int         size;                   // window size
    //  4108  u_char      *bufptr;                // circular buffer pointer
    //  4112  u_char      *buf;                   // circular buffer
    //  4120  u_int64_t   poly;                   // minimal polynom

    // the following numbers need to be modified, should the rabin_state structure changes
    #define old_ringing 0
    #define new_ringing 2048
    #define fingerprint 4096
    #define size        4104
    #define bufptr      4108
    #define buf         4112
    #define poly        4120

.global _rabin_slide8
_rabin_slide8:

	// allocate stack space and store ebx/esi/edi/ebp

	subl	$20, %esp
	movl	%ebx, 4(%esp)
	movl	%esi, 8(%esp)
	movl	%edi, 12(%esp)
	movl	%ebp, 16(%esp)

	#define	s	%ebp

	movl	24(%esp), s				// ebp -> rabin_state *s;
	movl	28(%esp), %ebx				// ebx = m

	movl	bufptr(s), %eax				// eax : s->bufptr; 
	movzbl	(%eax), %ecx				// ecx : om = *s->bufptr; 
	movb	%bl, (%eax)				// *s->bufptr = m;

	decl	%eax					// s->bufptr--;
	movl	%eax, bufptr(s)				// save s->bufptr
	cmpl	buf(s), %eax				// s->buf vs s->bufptr
	jae		L2
	addl	size(s), %eax				// s->bufptr += s->size;
	movl	%eax, bufptr(s)				// save s->bufptr
L2:
	movzbl	%cl, %eax
	movl	4(s,%eax,8), %edx
	movl	(s,%eax,8), %eax			// edx.eax : residual = s->old_ringing[om];

	movl	(fingerprint+4)(s), %edi		// fingerprint high32
	movl	%edi, %esi
	shrl	$23, %esi				// fingerprint>>55

	movl	fingerprint(s), %ecx
	xorl	new_ringing(s,%esi,8), %eax
	xorl	(new_ringing+4)(s,%esi,8), %edx		// edx.eax : residual ^= s->new_ringing[fingerprint>>55];
	movzbl	%bl, %esi
	movl	(fingerprint+4)(s), %ebx		// ebx.ecx : fingerprint
	shldl	$8, %ecx, %ebx
	sall	$8, %ecx				// fingerprint<<8
	orl		%ecx, %esi			// (fingerprint<<8)|m

	xorl	%ebx, %edx
	xorl	%esi, %eax				// edx.eax : fingerprint = ((fingerprint<<8)|m)^residual;

	andl	$2147483647, %edx			// fingerprint &= ~MSB64;

	movl	%eax, fingerprint(s)
	movl	%edx, (fingerprint+4)(s)		// s->fingerprint = fingerprint;

	// restore ebx/esi/edi/ebp and return

	movl	4(%esp), %ebx
	movl	8(%esp), %esi
	movl	12(%esp), %edi
	movl	16(%esp), %ebp
	addl	$20, %esp
	ret

#elif defined __x86_64__

	.text
	.align 4,0x90

    // rabin_state structure (defined in rabin_apl.h)
    //     0  u_int64_t   old_ringing[256];
    //  2048  u_int64_t   new_ringing[256];
    //  4096  u_int64_t   fingerprint;            // rabin checksum fingerprint
    //  4104  int         size;                   // window size
    //  4112  u_char      *bufptr;                // circular buffer pointer
    //  4120  u_char      *buf;                   // circular buffer
    //  4128  u_int64_t   poly;                   // minimal polynom

	// the following numbers need to be modified, should the rabin_state structure changes
	#define	old_ringing	0
	#define	new_ringing	2048
	#define	fingerprint	4096
	#define	size		4104
	#define	bufptr		4112
	#define	buf		4120
	#define	poly		4128

.global _rabin_slide8
_rabin_slide8:
    movq    bufptr(%rdi), %rdx				// rdx = s->bufptr
    movzbl  (%rdx), %eax				// eax = om = *s->bufptr
    movb    %sil, (%rdx)         		   	// *s->bufptr = m
    decq    %rdx                    			// s->bufptr--
    cmpq    buf(%rdi), %rdx        			// s->buf vs s->bufptr
    jae L2                          			// if bufptr >= buf, branch to L2
    movslq  size(%rdi),%rcx				// s->size
    addq    %rcx, %rdx					// bufptr += size;
L2:
    movq    fingerprint(%rdi), %rcx			// rcx : fingerprint = s->fingerprint;
    movq    %rdx, bufptr(%rdi)				// update s->bufptr
    movq    old_ringing(%rdi,%rax,8), %rdx		// rdx : residual = s->old_ringing[om];
    movq    %rcx, %rax					// rax : fingerprint
    shrq    $55, %rcx					// rcx : fingerprint >> 55; 
    xorq    new_ringing(%rdi,%rcx,8), %rdx		// rdx : residual ^= s->new_ringing[fingerprint>>55];  
    salq    $8, %rax					// rax : fingerprint<<8

    orq     %rsi, %rax					// rax : ((fingerprint<<8)+m)
    xorq    %rdx, %rax					// rax : ((fingerprint<<8)+m) ^ residual 

    movabsq $9223372036854775807, %rdx
    andq    %rdx, %rax

    movq    %rax, fingerprint(%rdi)			// s->fingerprint = fingerprint;
    ret

#endif	// __i386__ or __x86_64__
