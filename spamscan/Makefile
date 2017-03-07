include ../build.mk

LDFLAGS+=-fPIC

.PHONY: all
all: spamscan.so

spamscan.so: spamscan.o spamscan_array.c spamscan_engine.c dmp.c dmp_pool.c
