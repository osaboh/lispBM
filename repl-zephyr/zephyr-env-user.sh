export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
# export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.10.0/   # for zephyr 1.14.x
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.10.3/     # for zephyr 2.1.x

# Zephyr DIR
source $HOME/Dev/zephyr/west_210_test/zephyrproject/zephyr/zephyr-env.sh

echo loaded
