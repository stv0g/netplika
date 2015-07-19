/** Global configuration.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION		"0.1"

#define SRC_PORT	23532
#define DEST_PORT	22

struct config {
	int mark;
	int mask;
	int interval;
	int rate;
	int limit;
	
	char *dev;
};

/* Declared in main.c */
extern struct config cfg;

#endif