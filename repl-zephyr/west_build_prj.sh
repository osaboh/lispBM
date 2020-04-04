#!/bin/bash

source zephyr-env-user.sh

if [ $# = 1 ]; then
    if [ $1 = "menuconfig" ]; then
        west build -t menuconfig
        exit 0
    fi
fi

west build -b nrf52840_pca10059  -- -DBOARD_ROOT=./ -DCONF_FILE="prj.conf"


