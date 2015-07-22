/*
 * Experimental data  distribution table generator
 * Taken from the uncopyrighted NISTnet code (public domain).
 *
 * Read in a series of "random" data values, either
 * experimentally or generated from some probability distribution.
 * From this, create the inverse distribution table used to approximate
 * the distribution.
 *
 * @link https://github.com/shemminger/iproute2/blob/master/netem/maketable.c
 */

#ifndef _DIST_MAKETABLE_H_
#define _DIST_MAKETABLE_H_

#include <stdio.h>

/* Create a (normalized) distribution table from a set of observed
 * values.  The table is fixed to run from (as it happens) -4 to +4,
 * with granularity .00002.
 */

#define TABLESIZE	16384/4
#define TABLEFACTOR	8192
#ifndef MINSHORT
#define MINSHORT	-32768
#define MAXSHORT	32767
#endif

/* Since entries in the inverse are scaled by TABLEFACTOR, and can't be bigger
 * than MAXSHORT, we don't bother looking at a larger domain than this:
 */
#define DISTTABLEDOMAIN ((MAXSHORT/TABLEFACTOR)+1)
#define DISTTABLEGRANULARITY 50000
#define DISTTABLESIZE (DISTTABLEDOMAIN*DISTTABLEGRANULARITY*2)

double * readdoubles(FILE *fp, int *number);

void arraystats(double *x, int limit, double *mu, double *sigma, double *rho);

int * makedist(double *x, int limit, double mu, double sigma);

/* replace an array by its cumulative distribution */
void cumulativedist(int *table, int limit, int *total);

short * inverttable(int *table, int inversesize, int tablesize, int cumulative);

/* Run simple linear interpolation over the table to fill in missing entries */
void interpolatetable(short *table, int limit);

void printtable(const short *table, int limit);

#endif