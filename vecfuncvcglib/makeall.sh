#!/usr/bin/env bash

set -e

if [[ $# -ge 1 ]]
then
    make_dims="$@"
else
    make_dims=$(seq 1 6)
fi

for i in ${make_dims}; do
    echo "Making all types for DIM=$i"
	make dim=${i} value=int32
	make dim=${i} value=int64
	make dim=${i} value=uint32
	make dim=${i} value=uint64
	make dim=${i} value=float32
	make dim=${i} value=float64
done
