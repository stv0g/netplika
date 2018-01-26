/** Global configuration.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION		"0.1"

struct config {
	struct {
		enum {
			PROBE_ICMP,
			PROBE_TCP
		} mode;
		int payload;
		int limit;
		double rate;
		int warmup;
	} probe;

	struct {
		enum {
			FORMAT_TC,
			FORMAT_VILLAS
		} format;
		double scaling;
	} dist;

	struct {
		int mark;
		int mask;
		char *dev;
		int interval;
	} emulate;
};

/* Declared in main.c */
extern struct config cfg;

#endif
