#!/bin/sh

if [ $# = 1 ]; then
  if [ $1 = 'clean' ]; then
    make -C ../ clean
    rm -rf ./build
    exit 0
  fi
fi

make -C ../
./west_build_prj.sh

/usr/bin/echo -e "\n**** Write build/zephyr/zephyr.hex to Flash ROM. ****\n"
