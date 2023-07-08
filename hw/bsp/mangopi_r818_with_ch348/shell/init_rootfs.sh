#!/usr/bin/bash
parted /dev/mmcblk0 p
parted /dev/mmcblk0 rm 10
parted /dev/mmcblk0 resizepart 9 100%
resize2fs /dev/mmcblk0p9
parted /dev/mmcblk0 p
