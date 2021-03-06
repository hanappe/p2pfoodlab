#!/bin/bash

rm -f /var/p2pfoodlab/etc/config.json
rm -f /var/p2pfoodlab/etc/opensensordata/*
rm -rf ~/.ssh/
sudo rm -f /boot/p2pfoodlab*.json*
> /var/p2pfoodlab/log.txt
rm -f /home/pi/.bash_history 
sudo rm -f /root/.bash_history 

cd /home/pi/p2pfoodlab/rpi/src/
git pull
make

cp /home/pi/p2pfoodlab/rpi/bin/sensorbox /var/p2pfoodlab/bin/sensorbox 
cp /home/pi/p2pfoodlab/rpi/config.json /var/p2pfoodlab/etc/config.json

sudo /var/p2pfoodlab/bin/sensorbox -l - -D generate-system-files
sudo rm -rf /var/p2pfoodlab/backup/*

sudo cp /home/pi/p2pfoodlab/rpi/boot/p2pfoodlab.json /boot/p2pfoodlab.json

history -c
history -w
