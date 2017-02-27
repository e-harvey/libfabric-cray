#!/bin/bash
gcc -Wall `pkg-config cray-alpsutil --clflags --libs` `pkg-config cray-ugni --cflags --libs` `pkg-config cray-gni-headers --cflags --libs` `pkg-config cray-udreg --cflags --libs` -I $PWD/../libfabric-cray/include/ -I $PWD/../libfabric-cray/prov/gni/include/ -L$PWD/../libfabric-cray-build-new-gnix_vec/lib/ -I $PWD/../libfabric-cray -lfabric -lpthread -lalps -L/opt/cray/pmi/default -L/opt/cray/xpmem/default/lib64 -lxpmem -L/opt/cray/alps/default/lib64 -lalpsutil -lalpslli -lrt -I$PWD/common gnix_vector_tests.c $PWD/../libfabric-cray-build-new-gnix_vec/lib/libfabric.a

