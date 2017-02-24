#U-boot Build Guide#

##download u-boot##

    git clone https://github.com/Xilinx/u-boot-xlnx
    git checkout -b yunsdr xilinx-v2015.2

##patch the u-boot##

    git am -s < 0001-add-zingsk-support-step-1.patch
    git am -s < 0002-don-t-init-uart-in-uboot-boot-stage.patch
    git am -s < 0003-modify-fdt_high-and-initrd_high.patch

##build u-boot##

    make zynq_zingsk_config
    make -j4
    mv u-boot u-boot.elf
