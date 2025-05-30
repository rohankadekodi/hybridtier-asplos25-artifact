#!/bin/bash

export BIGMEMBENCH_COMMON_PATH="/ssd1/asplos25_ae/hybridtier-asplos25-artifact"

echo "start" > ./exp_tracker

# update boot up script
cp ./rc.local.hybridtier /etc/rc.local

# start with 32GB hybridtier kernel
cp /etc/default/grubs_kevin/grub_hybridtier_32GB /etc/default/grub
update-grub2
reboot

