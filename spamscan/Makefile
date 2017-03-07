include ../build.mk

LDFLAGS+=-fPIC

.PHONY: all
all: spamscan.so

spamscan.so: spamscan.o spamscan_array.o spamscan_engine.o dmp.o dmp_pool.o
