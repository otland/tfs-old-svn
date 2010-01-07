#!/bin/bash
echo "TheForgottenServer test build script, seems to speed things up ALOT when testing new features. Requires ccache installed!"

# Enable CCACHE
export PATH=/usr/lib/ccache:$PATH
# Enable make processes - 1 + number of cores
 # Get cores
 cores=`grep cores /proc/cpuinfo | wc -l` 
 makeopt_j=$(($cores+1))
echo "Building on $cores, using $makeopt_j processes"
make -j $makeopt_j
