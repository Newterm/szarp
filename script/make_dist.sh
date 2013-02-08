#!/bin/bash

if [ $# -ne 2 ] ; then
	echo "Usage:"
	echo "    $0 <dist> <ver>"
	echo
	echo "    dist - debian or ubuntu"
	echo "    ver  - distribution version name"
	exit 0
fi

# Default chroot directory. Change this line if different needed.
DIR=~/chroot

# User name and email for configuration. If you use different
# distribution than debian, or dont have proper variables set,
# uncomment and fill following lines.
#
# DEBFULLNAME= 
# DEBEMAIL=

NAME=${DEBFULLNAME?"Variable DEBFULLNAME must be set. Try running: \`DEBFULLNAME=\"Your Name\" $0\` or look into script."}
EMAIL=${DEBEMAIL?"Variable DEBEMAIL must be set. Try running: \`DEBEMAIL=\"your@email.com\" $0\` or look into script."}
NEWTERMDIR=~/newterm/

DIST=$1
VER=$2
CHROOT=$DIR/$VER

if [ $DIST == "debian" ] ; then
	URL=http://ftp.us.debian.org/debian/
elif [ $DIST == "ubuntu" ] ; then
	URL=http://archive.ubuntu.com/ubuntu/
else
	echo "Wrong distribution"
	exit 1
fi

cd $DIR
mkdir $VER || exit 1

#
# install system
#
sudo debootstrap --arch i386 $VER $CHROOT $URL || exit 1

#
# bind enviroment and set binding at startup
#
echo
echo "$NAME, please insert user password in $0"
echo -e "\a"
echo
notify-send "$NAME, please insert user password in $0" 2> /dev/null 

# /proc
sudo sh -c "echo \"proc $CHROOT/proc proc defaults 0 0\" >> /etc/fstab"
sudo mount proc $CHROOT/proc -t proc

# /sys
sudo sh -c "echo \"sysfs $CHROOT/sys sysfs defaults 0 0\" >> /etc/fstab"
sudo mount sysfs $CHROOT/sys -t sysfs

# /data for kontron-program
sudo mkdir -p $CHROOT/data
sudo sh -c "echo \"/data $CHROOT/data none defaults,bind 0 0\" >> /etc/fstab"
sudo mount $CHROOT/data

# /opt/szarp/ for szarp :)
sudo mkdir -p $CHROOT/opt/szarp
sudo sh -c "echo \"/opt/szarp/ $CHROOT/opt/szarp none defaults,bind 0 0\" >> /etc/fstab"
sudo mount $CHROOT/opt/szarp

# Directory where you store whole newterm stuff
sudo mkdir -p $CHROOT/$1/newterm
sudo sh -c "echo \"$NEWTERMDIR $CHROOT/newterm none defaults,bind 0 0\" >> /etc/fstab"
sudo mount $CHROOT/newterm

#
# make configs
#
sudo mkdir -p $CHROOT/root/.ssh/
sudo chmod 700 $CHROOT/root/.ssh/
sudo sh -c "echo 'Host newterm.pl' >> $CHROOT/root/.ssh/config"
sudo sh -c "echo 'Port 21' >> $CHROOT/root/.ssh/config"
sudo ssh-keygen -t dsa -f $CHROOT/root/.ssh/id_dsa -N ''
echo
echo "$NAME, please insert root@newterm.pl password in $0 \a"
echo -e "\a"
echo
notify-send "$NAME, please insert root@newterm.pl password in $0" 2> /dev/null 
sudo sh -c "cat $CHROOT/root/.ssh/id_dsa.pub | ssh -p21 root@newterm.pl 'cat >> /home/kontron-repo/.ssh/authorized_keys'"
sudo chroot $CHROOT chown root:root /root/.ssh -R
sudo sh -c "echo 'export DEBFULLNAME=\"$DEBFULLNAME\"' >> $CHROOT/root/.bashrc"
sudo sh -c "echo 'export DEBEMAIL=\"$DEBEMAIL\"' >> $CHROOT/root/.bashrc"

#
# set szarp repositories
#
sudo sh -c "echo \"deb http://www.szarp.org/debian $VER main contrib non-free\" >> $CHROOT/etc/apt/sources.list"
sudo sh -c "echo \"deb ssh://kontron-repo@newterm.pl:/home/kontron-repo/debian/ $VER main contrib non-free\" >> $CHROOT/etc/apt/sources.list"

#
# generate locale
#
sudo cp /etc/locale.gen $CHROOT/etc/locale.gen
sudo chroot $CHROOT locale-gen

#
# install os
#
sudo sh -c "echo \"$VER\" >> $CHROOT/etc/debian_chroot"
sudo chroot $CHROOT apt-get update
sudo chroot $CHROOT apt-get install aptitude wget vim git -y
sudo chroot $CHROOT wget -O - http://szarp.org/debian/key.asc | sudo chroot $CHROOT apt-key add -
sudo chroot $CHROOT git config --global user.name "$NAME"
sudo chroot $CHROOT git config --global user.email $EMAIL
sudo chroot $CHROOT /bin/bash

