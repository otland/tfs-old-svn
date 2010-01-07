#!/bin/bash
echo "TFS test build script, seems to speed things up ALOT when testing new features. Require ccache installed"

# Enable CCACHE
export PATH=/usr/lib/ccache:$PATH
# Enable 4 make processes
make -j 4
