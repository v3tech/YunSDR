#!/bin/sh

echo "Starting YunSDR Application..."

#########################################################
cp /mnt/resolv.conf /etc/
route add default gw 192.168.1.1
#########################################################
ifconfig eth0 down
ifconfig eth0 hw ether 00:0a:35:00:01:21
ifconfig eth0 192.168.1.10 up

cd /mnt
insmod v3best-dma-dac.ko 
insmod v3best-dma-adc.ko 

./yunsdr_timestamp_ctl.elf &
./yunsdr_timestamp_trx.elf &




