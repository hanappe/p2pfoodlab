#!/bin/bash

# Install required packages. Also remove package ifplugd because it is
# doing a bad job on the Raspberry Pi in combination with the DHCP
# server isc-dhcp-server.
apt-get update
apt-get install apache2 php5 libapache2-mod-php5 gcc libjpeg8-dev i2c-tools libi2c-dev isc-dhcp-server
apt-get purge ifplugd
apt-get autoremove

# Add user 'pi' to the required groups.
adduser pi i2c
adduser pi video
adduser pi www-data

# Load the modules for the I2C bus.
modprobe i2c_dev
modprobe i2c_bcm2708

# Delete any trailing files from a previous download.
rm -rf /tmp/sensorbox.tar.gz /tmp/dist

# Download the latest version of the sensorbox software and
# configuration files.
curl --output /tmp/sensorbox.tar.gz https://p2pfoodlab.net/sensorbox-versions/sensorbox-latest.tar.gz
cd /tmp
tar xzf sensorbox.tar.gz
chown -R pi.pi dist

# Compile the source code.
su -l -c "make -C /tmp/dist/src clean all" pi

# Install the files.
cd /tmp/dist
bash ./install.sh /var/p2pfoodlab pi pi

# Install the P2P Food Lab configuration file.
install --directory --owner=pi --group=www-data --mode=0775 /var/p2pfoodlab/etc
install --owner=pi --group=www-data --mode=0664 config.json /var/p2pfoodlab/etc

# Install the system configuration files.
source ./files.txt
for file in ${etc[@]}; do
    install --owner=root --group=root $file /$file
done

# Install the start-up scripts.
update-rc.d p2pfoodlab start 99 2 3 4 5 . stop 99 0 6 .
update-rc.d arduino-hwclock start 10 S . stop 10 0 1 6 .

service p2pfoodlab start

# Restart the web server.
service apache2 restart

# Enable the DHCP server on eth0
update-rc.d isc-dhcp-server enable

