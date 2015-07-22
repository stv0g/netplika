# Netem Tool

*Note:* This tool is in the alpha stage!

This tool uses a 3-way TCP handshake to measure the _real_ round-trip-time of a TCP connection.
The gathered information can be used to configure the Linux network emulation queuing discipline (see [tc-netem(8)](http://man7.org/linux/man-pages/man8/tc-netem.8.html)).

It is possible to directly pass the results of the RTT probes to the Kernel by using a netlink socket and [libnl](http://www.infradead.org/~tgr/libnl/).
Therefore it allows to simulate an existing network connection on-the-fly.
Alternatively, the measurements can be stored in a file and replayed later using Bash's IO redirection.

### Usage

###### Use case 1: collect RTT measurements

Run TCP SYN/ACK probes to measure round-trip-time (RTT):

    ./netem probe 8.8.8.8 53 > measurements.dat

The `probe` sub-command returns the following fields per line on STDOUT:

    current_rtt, mean, sigma

###### Use case 2a: convert measurements into delay distribution table

Collect measurements to build a [tc-netem(8)](http://man7.org/linux/man-pages/man8/tc-netem.8.html) delay distribution table

    ./netem dist generate < measurements.dat > google_dns.dist

*Please note:* you might have to change the scaling by adjusting the compile time constants in `dist-maketable.h`!

###### Use case 2b: generate distribution from measurements and load it to the Kernel

    ./netem dist load < probing.dat

*Please note:* you might have to change the scaling by adjusting the compile time constants in `dist-maketable.h`!

###### Use case 3: on-the-fly link simulation

The output of this command and be stored in a file or directly passed to the `emulate` subcommand:

    ./netem probe 8.8.8.8 53 | ./netem emulate

or

    ./netem emulate < measurements.dat

The emulate sub-command expects the following fields on STDIN seperated by whitespaces:

    current_rtt, mean, sigma, gap, loss_prob, loss_corr, reorder_prob, reorder_corr, corruption_prob, corruption_corr, duplication_prob, duplication_corr;

At least the first three fields have to be given. The remaining ones are optional.

###### Use case 4: Limit the effect of the network emulation to a specific application

To apply the network emulation only to a limit stream of packets, you can use the `mark` tool.

    ./netem -m 0xBABE dist load < measurements.dat
    sudo LD_PRELOAD=${PWD}/mark.so MARK=0xBABE netcat google.de 80

This tool uses the dynamic linker to hook into the `socket()` wrapper-function of libc (see `mark.c`).
Usually, the hook will simply call the original `socket(2)` syscall for non-AF_NET sockets.
But for AF_INET sockets, the hook will additionally call `setsockopt(sd, SOL_SOCKET, SO_MARK, ...)` after the socket has been created.

Later on, the `netem` tool will use combination of the classfull `prio` qdisc and the `fw` classifier to limit the network emulation only to the _marked_ application (see use case 5, below).

*Note:* There are two pittfalls when using this approach:

- Make sure to specify the environmental variables after the sudo command! This is necessary, as `ping` is a SUID program. The dynamic linker strips certain enviromental variables (as `LD_PRELOAD`) for security reasons when privileges are elevated.
- Setting the packet mark requires CAP_NET_ADMIN privs. Therefore you must start the application as root. Unfortunately, some applications also drop those privs quite early (ping is an example which luckily has an `-m` option).

Alternatively you can set the mark using netfilter:

    iptables -t mangle -I OUTPUT -d 8.8.8.8 --set-mark 0xBABE -j MARK

Or, the `tc-cgroup.sh` script which uses a special priority for a certain cgroup:

    ./tc-cgroup ping google.de

###### Use case 5: Show the current Traffic Controller setup

    ./tcdump.sh eth0

     ======= eth0: qdisc ========
     qdisc prio 1: root refcnt 2 bands 4 priomap  2 3 3 3 2 3 1 1 2 2 2 2 2 2 2 2
      Sent 17304 bytes 126 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0
     qdisc netem 2: parent 1:1 limit 1000 delay 3.3ms  9.1ms
      Sent 0 bytes 0 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0
     ======= eth0: filter ========
     filter parent 1: protocol all pref 49152 fw
     filter parent 1: protocol all pref 49152 fw handle 0xcd classid 1:1
     ======= eth0: class ========
     class prio 1:1 parent 1: leaf 2:
      Sent 0 bytes 0 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0
     class prio 1:2 parent 1:
      Sent 15126 bytes 115 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0
     class prio 1:3 parent 1:
      Sent 3270 bytes 17 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0
     class prio 1:4 parent 1:
      Sent 0 bytes 0 pkt (dropped 0, overlimits 0 requeues 0)
      backlog 0b 0p requeues 0

### ToDo

##### More metrics:

Add more metrics to the probing system:

  - loss
  - duplication
  - corruption
  - reordering


##### Hardware Timestamping Support:

There is experimental support for using Linux' HW / Kernelspace timestamping support (see `ts.c`).
This allows to measure the RTT by using the arrival / departure times of packets in the NIC or in the Kernel, instead of relying on the inaccuarate user space.

Unfortunately, this hardware timestamping support requires special driver support.
Therefore it's still disabled.

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
