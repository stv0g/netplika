#!/bin/bash

sudo LD_PRELOAD=${PWD}/mark.so MARK=0xcd $@
