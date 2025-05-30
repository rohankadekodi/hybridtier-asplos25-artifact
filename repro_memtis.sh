#!/bin/bash

export BIGMEMBENCH_COMMON_PATH="/ssd1/songxin8/thesis/hybridtier/hybridtier-asplos25-artifact"

echo "memtis_start" > ./exp_tracker
# update boot up script
cp ./rc.local.memtis /etc/rc.local
echo "Updated rc.local"
cat /etc/rc.local


# memtis kernel
cp /etc/default/grubs_kevin/grub_memtis /etc/default/grub
update-grub2

echo "Updated kernel. Rebooting in 10 seconds..."
sleep 10

reboot

