#!/bin/sh

echo "Starting Zing axi-dma Application..."

#########################################################
cp /mnt/resolv.conf /etc/
route add default gw 192.168.1.1
#########################################################

cd /mnt/

insmod v3best-dma-dac.ko 
insmod v3best-dma-adc.ko 

./yunsdr_ctl.elf &
./yunsdr_rx.elf &
./yunsdr_tx.elf &



