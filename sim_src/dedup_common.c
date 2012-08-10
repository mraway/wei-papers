#include "dedup_common.h"

void pr_msg(const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZ], *p2;
	const char *p1;
	char *errstr = strerror(errno);

	buf[0] = '\0';
	p2 = buf + strlen(buf);

	/*
	 * Look for "%m" in the string.
	 * If found, then substitute the errno string.
	 */
	for (p1 = fmt; *p1; p1++) {
		if (*p1 == '%' && *(p1+1) == 'm') {
			(void) strcpy(p2, errstr);
			p2 += strlen(errstr);
			p1++;
		} else {
			*p2++ = *p1;
		}
	}
	if (p2 > buf && *(p2-1) != '\n')
		*p2++ = '\n';
	*p2 = '\0';

	va_start(ap, fmt);
	(void) vfprintf(stderr, buf, ap);
	fflush(stderr);
	va_end(ap);
}

