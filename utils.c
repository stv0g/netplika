/** Utilities.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 8
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <error.h>
#include <string.h>

#include "utils.h"

void hexdump(void *mem, unsigned int len)
{
	unsigned int i, j;
	
	for (i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
		if (i % HEXDUMP_COLS == 0)
			printf("0x%06x: ", i); /* print offset */

		if (i < len)
			printf("%02x ", 0xFF & ((char*)mem)[i]); /* print hex data */
		else
			printf("   "); /* end of block, just aligning for ASCII dump */
		
		/* print ASCII dump */
		if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
			for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
				if (j >= len) /* end of block, not really printing */
					putchar(' ');
				else if (isprint(((char*)mem)[j])) /* printable char */
					putchar(0xFF & ((char*)mem)[j]);	
				else /* other char */
					putchar('.');
			}
			
			putchar('\n');
		}
	}
}

void * alloc(size_t bytes)
{
	void *p = malloc(bytes);
	if (!p)
		error(-1, 0, "Failed to allocate memory");

	memset(p, 0, bytes);
	
	return p;
}

int strap(char *dest, size_t size, const char *fmt,  ...)
{
	int ret;
	
	va_list ap;
	va_start(ap, fmt);
	ret = vstrap(dest, size, fmt, ap);
	va_end(ap);
	
	return ret;
}

int vstrap(char *dest, size_t size, const char *fmt, va_list ap)
{
	int len = strlen(dest);

	return vsnprintf(dest + len, size - len, fmt, ap);
}