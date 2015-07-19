/** Hardware / Kernelspace Timestamping of network packets.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#ifndef _TS_H_
#define _TS_H_

ssize_t ts_sendmsg(int sd, const struct msghdr *msgh, int flags, struct timespec *ts);

ssize_t ts_recvmsg(int sd,       struct msghdr *msgh, int flags, struct timespec *ts);

int ts_enable_if(const char *dev);

int ts_enable_sd(int sd);

#endif