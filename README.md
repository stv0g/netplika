# Netem Tool

This tool uses the 3-way TCP handshake to measure the real RTT of a TCP connection.

### Usage

Run TCP SYN/ACK probes to measure round-trip-time (RTT):

    ./netem probe 8.8.8.8 > probing.dat

The output of this command and be stored in a file or directly passed to the `emulate` subcommand:

    ./netem probe 8.8.8.8 | ./netem emulate
    ./netem emulate < probing.dat

### Building

##### Install libnl

git submodule init
git submodule update

cd libnl/
autreconf -i
./configure
make
make install

##### Install netem

Just run `make`.

### Dependencies

- [libnl3](http://www.infradead.org/~tgr/libnl/) (3.2.26)
- recent Linux Kernel version

For building libnl (make libnl)

- libtool
- bison
- flex
- autotools

### See also

- http://www.infradead.org/~tgr/libnl/
- https://www.kernel.org/doc/Documentation/networking/timestamping.txt
- [tc-netem(8)](http://man7.org/linux/man-pages/man8/tc-netem.8.html)

### Author

Steffen Vogel <post@steffenvogel.de>

This tool was developed as a project for the Network Programming Laboratory of the Institute for Networked Systems, RWTH Aachen University, in summer 2015.

### License

Copyright (C) 2015 Steffen Vogel

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.