#!/bin/bash
echo "TheForgottenServer test build script, seems to speed things up ALOT when testing new features. Requires ccache installed!"

# Enable CCACHE
export PATH=/usr/lib/ccache:$PATH
# Enable 5 make processes - 1 + number of cores
make -j 5
