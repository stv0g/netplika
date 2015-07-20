# Netem Tool

This tool uses a 3-way TCP handshake to measure the real RTT of a TCP connection.
The gathered information can be used to configure the Linux network emulation queing discipline (see tc-netem(8)).

It is possible to directly pass the results of the RTT probes to the Kernel.
Therefore it allows to simulate an existing network connection in realtime.
Alternatively, the measurements can be stored and replayed later.

One use case might be to test your application, equipement, protocols against sudden changes of the link quality.
It also allows to generate custom delay distributions which can be used with tc-netem(8).

### Usage

###### Use case 1: collect RTT measurements

Run TCP SYN/ACK probes to measure round-trip-time (RTT):

    ./netem probe 8.8.8.8 53 > measurements.dat

###### Use case 2a: convert measurements into delay distribution table

Collect measurements to build a tc-netem(8) delay distribution table

    ./netem dist generate < measurements.dat > google_dns.dist

###### Use case 2b: generate distribution from measurements and load it to the Kernel

    ./netem dist load < probing.dat

###### Use case 3: on-the-fly link simulation

The output of this command and be stored in a file or directly passed to the `emulate` subcommand:

    ./netem probe 8.8.8.8 53 | ./netem emulate

or

    ./netem emulate < measurements.dat

###### Use case 4: Limit the effect of the network emulation to a specific application

To apply the network emulation only to a limit stream of packets, you can use the `mark` tool:

    ./netem -m 0xCD dist load < measurements.dat
    sudo LD_PRELOAD=${PWD}/mark.so MARK=0xCD ping google.de

Please make sure the specify the environmental variables after the sudo command!
This is necessary, as `ping` is a SUID program.
The dynamic linker strips certain enviromental variables (as `LD_PRELOAD`) for security reasons!

### ToDo

Add more metrics:

  - loss
  - duplication
  - corruption
  - reordering

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
- https://github.com/shemminger/iproute2/tree/master/netem
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