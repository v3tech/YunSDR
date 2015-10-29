#Kernel Build Guide#

##download Kernel ##

    git clone https://github.com/Xilinx/linux-xlnx
    git checkout -b yunsdr xilinx-v2014.4-trd

##patch Kernel##

    git am -s < 0001-add-yunsdr_defconfig-for-V3best-YunSDR.patch
    git am -s < 0002-change-zynq-ps-gpio-base-number-in-gpio-driver.patch
	
##configure Kernel##

	make yunsdr_defconfig

##build Kernel##

    make -j4 uImage LOADADDR=0x8000