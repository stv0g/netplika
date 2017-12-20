/** Userspace TCP functions.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#ifndef _TCP_H_
#define _TCP_H_

/** Simple checksum function, may use others such as Cyclic Redundancy Check, CRC */
unsigned short tcp_csum(unsigned short *buf, int len);

#endif