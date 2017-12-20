/** Utilities.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel
 * @license GPLv3
 * @file
 *********************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdarg.h>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

void hexdump(void *mem, unsigned int len);

void * alloc(size_t bytes);

/** Safely append a format string to an existing string.
 *
 * This function is similar to strlcat() from BSD.
 */
int strap(char *dest, size_t size, const char *fmt,  ...);

/** Variadic version of strap() */
int vstrap(char *dest, size_t size, const char *fmt, va_list va);

#endif
