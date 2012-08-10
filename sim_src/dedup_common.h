#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Print an error.
 * (fmt string and variable args) except that it will prepend EXECNAME
 * and substitute an error message for a "%m" string (like syslog).
 */
void pr_msg(const char *fmt, ...);
