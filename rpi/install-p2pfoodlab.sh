#!/bin/bash

curdir=`pwd`
#uid=`id -u`
#gid=`id -g`
uid=pi
gid=pi
home=/home/pi
dest=/var/p2pfoodlab

cd $home

first_time="yes"
if [ -d $dest ]; then
  first_time="no"
fi

echo --------------------------------------------------------
echo STEP 1: Updating and installing software packages

apt-get update

if [ "$first_time" == "yes" ]; then
    # Clean up unused packages to save space.
    apt-get purge --auto-remove scratch debian-reference-en dillo idle3 python3-tk idle python-pygame python-tk lightdm gnome-themes-standard gnome-icon-theme raspberrypi-artwork gvfs-backends gvfs-fuse desktop-base lxpolkit netsurf-gtk zenity xdg-utils mupdf gtk2-engines alsa-utils lxde lxtask menu-xdg gksu midori xserver-xorg xinit xserver-xorg-video-fbdev libraspberrypi-dev libraspberrypi-doc dbus-x11 libx11-6 libx11-data libx11-xcb1 x11-common x11-utils lxde-icon-theme gconf-service gconf2-common ifplugd

    # Install required packages. Also remove package ifplugd because
    # it is doing a bad job on the Raspberry Pi in combination with
    # the DHCP server isc-dhcp-server.
    apt-get install apache2 php5 libapache2-mod-php5 gcc libjpeg8-dev i2c-tools libi2c-dev isc-dhcp-server git libcurl4-openssl-dev bc wvdial
else
    apt-get --yes upgrade
fi

echo --------------------------------------------------------
echo STEP 2: Loading I2C kernel modules

# Load the modules for the I2C bus.
modprobe i2c_dev
modprobe i2c_bcm2708

echo --------------------------------------------------------
echo STEP 3: Manage user privileges

# Add user 'pi' to the required groups.
if [ "$first_time" == "yes" ]; then
    adduser pi i2c
    adduser pi video
    adduser pi www-data
    adduser pi dialout
fi

echo --------------------------------------------------------
echo STEP 4: Download, compile and install sensorbox software

# Download the latest version of the sensorbox software and
# configuration files.
if [ -d $home/p2pfoodlab ]; then
    su -l -c "cd $home/p2pfoodlab && git pull" $uid 
else
    su -l -c "cd $home && git clone https://github.com/hanappe/p2pfoodlab.git" $uid
fi

# Compile the source code.
su -l -c "make -C $home/p2pfoodlab/rpi/src clean all" $uid

# Install the files.
cd $home/p2pfoodlab/rpi
bash ./install.sh $dest $uid $gid

# Install the P2P Food Lab configuration file.
cd $home/p2pfoodlab/rpi
install --directory --owner=$uid --group=www-data --mode=0775 $dest
install --directory --owner=$uid --group=www-data --mode=0775 $dest/backup
install --directory --owner=$uid --group=www-data --mode=0775 $dest/photostream
install --directory --owner=$uid --group=www-data --mode=0775 $dest/etc
install --directory --owner=$uid --group=www-data --mode=0775 $dest/etc/opensensordata
install --directory --owner=$uid --group=$gid --mode=0700 $home/.ssh

install --directory --owner=$uid --group=www-data --mode=0755 $dest/web/data
ln -s $dest/datapoints.csv $dest/web/data/datapoints.csv
ln -s $dest/log.txt $dest/web/data/log.txt
ln -s $dest/backup $dest/web/data/backup
ln -s $dest/photostream $dest/web/data/photostream

touch $dest/log.txt
chown pi.www-data $dest/log.txt
chmod 0664 $dest/log.txt

# Install the system configuration files.
source ./files.txt
for file in ${etc[@]}; do
    install --owner=root --group=root $file /$file
done

if [ "$first_time" == "yes" ]; then
    install --owner=$uid --group=www-data --mode=0664 config.json $dest/etc
fi

if [ "$first_time" == "yes" ]; then
    install --owner=root --group=root etc/network/interfaces /etc/network/interfaces
fi

echo --------------------------------------------------------
echo "STEP 5: Activate sensorbox update application (crontab)"

# Add update requests to crontab
echo "* * * * * /var/p2pfoodlab/bin/sensorbox" | crontab -u $uid -

echo --------------------------------------------------------
echo STEP 6: Installing and starting system services

# Install the start-up scripts.

echo P2P Food Lab daemon:
update-rc.d p2pfoodlab start 99 2 3 4 5 . stop 99 0 6 .
#update-rc.d arduino-hwclock start 10 S . stop 10 0 1 6 .

service p2pfoodlab restart

# Restart the web server.
if [ "$first_time" == "yes" ]; then
    echo Web server:
    echo "ServerName localhost" > /etc/apache2/conf.d/fqdn
    service apache2 restart
fi

# Enable the DHCP server on eth0
if [ "$first_time" == "yes" ]; then
    echo DHCP server:
    update-rc.d isc-dhcp-server enable
    service isc-dhcp-server restart
fi

echo --------------------------------------------------------
echo Done!

cd $curdir

