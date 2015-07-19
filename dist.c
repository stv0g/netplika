/** Generate distribution tables for netem qdisc.
 *
 * Partially based on:
 *  - http://git.kernel.org/cgit/linux/kernel/git/shemminger/iproute2.git/tree/netem/maketable.c
 *  - https://github.com/thom311/libnl/blob/master/lib/route/qdisc/netem.c
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

/**
 * Set the delay distribution. Latency/jitter must be set before applying.
 * @arg qdisc Netem qdisc.
 * @return 0 on success, error code on failure.
 */
int rtnl_netem_set_delay_distribution_data(struct rtnl_qdisc *qdisc, double *data, size_t len) {
	struct rtnl_netem *netem;

	if (!(netem = rtnl_tc_data(TC_CAST(qdisc))))
		BUG();
	
	if (len > MAXDIST)
		return -NLE_INVAL;

	netem->qnm_dist.dist_data = (int16_t *) calloc(len, sizeof(int16_t));
	
	size_t i;		
	for (i = 0; i < len; i++)
		netem->qnm_dist.dist_data[n++] = data[i];
	
	netem->qnm_dist.dist_size = len;
	netem->qnm_mask |= SCH_NETEM_ATTR_DIST;

	return 0;	
}

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

static int * makedist(double *x, int limit, double mu, double sigma)
{
	int *table;
	int i, index, first=DISTTABLESIZE, last=0;
	double input;

	table = calloc(DISTTABLESIZE, sizeof(int));
	if (!table) {
		perror("table alloc");
		exit(3);
	}

	for (i=0; i < limit; ++i) {
		/* Normalize value */
		input = (x[i]-mu)/sigma;

		index = (int)rint((input+DISTTABLEDOMAIN)*DISTTABLEGRANULARITY);
		if (index < 0) index = 0;
		if (index >= DISTTABLESIZE) index = DISTTABLESIZE-1;
		++table[index];
		if (index > last)
			last = index +1;
		if (index < first)
			first = index;
	}
	return table;
}

/* replace an array by its cumulative distribution */
static void cumulativedist(int *table, int limit, int *total)
{
	int accum=0;

	while (--limit >= 0) {
		accum += *table;
		*table++ = accum;
	}
	*total = accum;
}

static short * inverttable(int *table, int inversesize, int tablesize, int cumulative)
{
	int i, inverseindex, inversevalue;
	short *inverse;
	double findex, fvalue;

	inverse = (short *)malloc(inversesize*sizeof(short));
	for (i=0; i < inversesize; ++i) {
		inverse[i] = MINSHORT;
	}
	for (i=0; i < tablesize; ++i) {
		findex = ((double)i/(double)DISTTABLEGRANULARITY) - DISTTABLEDOMAIN;
		fvalue = (double)table[i]/(double)cumulative;
		inverseindex = (int)rint(fvalue*inversesize);
		inversevalue = (int)rint(findex*TABLEFACTOR);
		if (inversevalue <= MINSHORT) inversevalue = MINSHORT+1;
		if (inversevalue > MAXSHORT) inversevalue = MAXSHORT;
		inverse[inverseindex] = inversevalue;
	}
	return inverse;

}

/* Run simple linear interpolation over the table to fill in missing entries */
static void interpolatetable(short *table, int limit)
{
	int i, j, last, lasti = -1;

	last = MINSHORT;
	for (i=0; i < limit; ++i) {
		if (table[i] == MINSHORT) {
			for (j=i; j < limit; ++j)
				if (table[j] != MINSHORT)
					break;
			if (j < limit) {
				table[i] = last + (i-lasti)*(table[j]-last)/(j-lasti);
			} else {
				table[i] = last + (i-lasti)*(MAXSHORT-last)/(limit-lasti);
			}
		} else {
			last = table[i];
			lasti = i;
		}
	}
}