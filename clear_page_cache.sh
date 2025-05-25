#!/bin/bash

while true
do 
    echo "Clearing page cache"
    echo 3 > /proc/sys/vm/drop_caches
    echo "Done"
    sleep 10 
done
