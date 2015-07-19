/** Userspace TCP functions.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

unsigned short tcp_csum(unsigned short *buf, int len)
{
	unsigned long sum;

	for(sum=0; len>0; len--)
		sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);

	return (unsigned short)(~sum);
}