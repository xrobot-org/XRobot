#!/bin/bash
cd && mkdir esp
cd esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd ../esp-idf
git checkout v5.1-dev
bash ./install.sh esp32-c3
