#!/bin/bash

curdir=`pwd`
#uid=`id -u`
#gid=`id -g`
uid=pi
gid=pi
home=/home/pi
dest=/var/p2pfoodlab

# Install required packages. Also remove package ifplugd because it is
# doing a bad job on the Raspberry Pi in combination with the DHCP
# server isc-dhcp-server.
apt-get update
apt-get install apache2 php5 libapache2-mod-php5 gcc libjpeg8-dev i2c-tools libi2c-dev isc-dhcp-server git
apt-get purge ifplugd
apt-get autoremove

# Add user 'pi' to the required groups.
adduser pi i2c
adduser pi video
adduser pi www-data

install --directory --owner=$uid --group=$gid --mode=0700 $home/.ssh
install --directory --owner=$uid --group=www-data --mode=0775 /etc/opensensordata

# Load the modules for the I2C bus.
modprobe i2c_dev
modprobe i2c_bcm2708

# Download the latest version of the sensorbox software and
# configuration files.
cd $home
if [ -d p2pfoodlab ]; then
    cd p2pfoodlab
    git pull 
else
    git clone https://github.com/hanappe/p2pfoodlab.git
    cd p2pfoodlab
fi

# Compile the source code.
su -l -c "make -C rpi/src clean all" pi

# Install the files.
cd dist
bash ./install.sh $dest $uid $gid
cd ..

# Install the P2P Food Lab configuration file.
install --directory --owner=$uid --group=www-data --mode=0775 $dest
install --directory --owner=$uid --group=www-data --mode=0775 $dest/backup
install --directory --owner=$uid --group=www-data --mode=0775 $dest/photostream
install --directory --owner=$uid --group=www-data --mode=0775 $dest/etc
install --owner=$uid --group=www-data --mode=0664 config.json $dest/etc

touch $dest/log.txt
chown pi.www-data $dest/log.txt
chmod 0664 $dest/log.txt

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

cd $curdir

