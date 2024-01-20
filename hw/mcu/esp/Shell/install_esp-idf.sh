#!/bin/bash
rm -rf ~/esp
cd && mkdir esp
cd esp
git clone --depth 1 --branch v4.4.4 https://github.com/espressif/esp-idf.git
cd ./esp-idf
git submodule update --init --recursive --recommend-shallow --depth 1
bash ./install.sh esp32-c3
cd components && git clone --depth 1 --branch idf-release/v4.4 https://github.com/espressif/arduino-esp32.git --depth 1
