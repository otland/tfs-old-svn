#!/bin/bash
# CCcache + multi core Compile script by Stian
# For "The Forgotten Server"
 
# Example:
# # make clean
# # time ./dev-build.sh

# Gives:
# real	3m27.070s
# user	6m4.066s
# sys	0m16.659s

# CCACHE recompile (from stratch):
# # make clean
# # time ./dev-build.sh

# Gives:
# real	0m27.620s
# user	0m43.744s
# sys	0m4.766s

# 1/7 of the original compile speed!
# When more you do it, more ccache will cache, default is limited to use 1GB storage

echo "TheForgottenServer test build script, seems to speed things up ALOT when testing new features. Requires ccache installed!"

# Enable CCACHE
export PATH=/usr/lib/ccache/bin:$PATH
# Enable make processes - 1 + number of cores
 # Get cores
 cores=`grep cores /proc/cpuinfo | wc -l` 
 makeopt_j=$(($cores+1))
echo "Building on $cores cores, using $makeopt_j processes"
make -j $makeopt_j
