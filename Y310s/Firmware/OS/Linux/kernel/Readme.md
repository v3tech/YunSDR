#Kernel Build Guide#

##download Kernel ##

    git clone https://github.com/Xilinx/linux-xlnx
    git checkout -b yunsdr xilinx-v2014.4-trd

##configure Kernel##

    cp yunsdr_defconfig linux-xlnx/.config

##build Kernel##

    make -j4 uImage LOADADDR=0x8000