KERN_SRC=/home/jacky/Work/14.6/linux-xlnx
#KERN_SRC=/media/eric/88c3e546-ad7f-4e7d-a539-7ff10cb8d3ef/home/jacky/Work/14.6/linux-zing
obj-m := v3best-dma-adc.o
 
all:
	make -C $(KERN_SRC) ARCH=arm M=`pwd` modules
clean:
	#make -C $(KERN_SRC) ARCH=arm M=`pwd=` clean
	rm modules.order Module.symvers *.o *.ko *.mod.c .*.cmd .tmp_versions -rf
