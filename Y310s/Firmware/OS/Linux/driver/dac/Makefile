KERN_SRC=/home/jacky/Work/14.6/linux-xlnx
obj-m := v3best-dma-dac.o
 
all:
	make -C $(KERN_SRC) ARCH=arm M=`pwd` modules
clean:
	#make -C $(KERN_SRC) ARCH=arm M=`pwd=` clean
	rm modules.order Module.symvers *.o *.ko *.mod.c .*.cmd .tmp_versions -rf
