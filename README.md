Light alarm firmware
====================

Firmware for HW part of light alarm project. Based on Zephyr RTOS.

How-to build
------------

You need to have v2.2.0 Zephyr sources, all tools (like west) and Zephyr SDK
installed. Build and runtime were tested with
SDK
 - zephyr-sdk-0.11.1
west
 - v0.7.2

### Prepare environment (regular Zephyr step):

```
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=${YOUR_SDK_INSTALL_PATH}/zephyr-sdk-0.11.1
# in Zephyr root directory
. ./zephyr-env.sh
```

### Build firmware:

```
west build -p auto -b rff_light_alarm_v0 .
```

### Flash firmware (require ST-LINK/V2 or compatible programmer)

```
west flash
```
