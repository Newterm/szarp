#!/bin/bash

DIR=~/chroot/
CHROOT=$DIR/$1

[ $# -eq 1 ] || exit 1

sudo cp /etc/resolv.conf $CHROOT/etc/
sudo cp /etc/hosts $CHROOT/etc/
sudo cp /etc/redis.conf $CHROOT/etc/

sudo chroot $CHROOT /bin/bash

