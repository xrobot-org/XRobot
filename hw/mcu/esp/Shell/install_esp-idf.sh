#!/bin/bash
rm -rf ~/esp
cd && mkdir esp
cd esp
git clone --depth 1 --branch v4.4.4 https://github.com/espressif/esp-idf.git
cd ./esp-idf
git submodule update --init --recursive --recommend-shallow --depth 1
bash ./install.sh && rm -rf .git
cd components && git clone https://github.com/espressif/arduino-esp32.git && cd arduino-esp32 && git checkout 5bf60b74261e0eb9cd2001d46508580dd97868d4 && rm -rf .git
