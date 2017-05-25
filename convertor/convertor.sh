#!/bin/sh
killall convertor
rmmod usb_midi
sleep 1
/sbin/convertor &
sleep 1
insmod /lib/modules/3.4.39-h3/kernel/sound/usb/usb-midi.ko