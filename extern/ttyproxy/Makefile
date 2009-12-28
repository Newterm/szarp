ifneq ($(KERNELRELEASE),)
obj-m:= ttyproxy.o
else
KDIR:= /usr/src/linux
PWD:= $(shell pwd)

all: ttyproxyd ttyproxy.ko

ttyproxyd: ttyproxyd.c ttyproxy.h
	gcc -O0 -g -levent -o ttyproxyd ttyproxyd.c

ttyproxy.ko: ttyproxy.h ttyproxy.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm *~ ttyproxyd Module.markers  modules.order  Module.symvers ttyproxy.ko  ttyproxy.mod.c  ttyproxy.mod.o  ttyproxy.o

endif
