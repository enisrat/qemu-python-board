#!/bin/sh

find . -iname "*.c" -and -not -name "sysbus_device_skeleton.c" -and -not -name "machine_skeleton.c"
