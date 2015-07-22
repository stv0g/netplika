#!/bin/bash

tc qdisc replace dev $1 root pfifo_fast
