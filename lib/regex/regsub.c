/*
 * regsub
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regexp.h>
#include "regmagic.h"

/*
 - regsub - perform substitutions after a regexp match
 */
void regsub(const regexp *rp, const char *source, char *dest)
{
	register const regexp * prog = (const regexp *)rp;
	register char *src = (char *)(unsigned long)source;
	register char *dst = dest;
	register char c;
	register int no;
	register size_t len;

	if (prog == NULL || source == NULL || dest == NULL) {
		regerror("NULL parameter to regsub");
		return;
	}
	if ((unsigned char)*(prog->program) != MAGIC) {
		regerror("damaged regexp");
		return;
	}

	while ((c = *src++) != '\0') {
		if (c == '&')
			no = 0;
		else if (c == '\\' && isdigit(*src))
			no = *src++ - '0';
		else
			no = -1;

		if (no < 0) {	/* Ordinary character. */
			if (c == '\\' && (*src == '\\' || *src == '&'))
				c = *src++;
			*dst++ = c;
		} else if (prog->startp[no] != NULL && prog->endp[no] != NULL &&
					prog->endp[no] > prog->startp[no]) {
			len = prog->endp[no] - prog->startp[no];
			(void) strlcpy(dst, prog->startp[no], len);
			dst += len;
			if (*(dst-1) == '\0') {	/* strlcpy hit NUL. */
				regerror("damaged match string");
				return;
			}
		}
	}
	*dst++ = '\0';
}
