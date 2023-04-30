#!/bin/bash
rm -rf ~/esp
cd && mkdir esp
cd esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd ./esp-idf
git checkout v4.4.4
git submodule update --init --recursive
bash ./install.sh esp32-c3
cd components && git clone https://github.com/espressif/arduino-esp32.git
