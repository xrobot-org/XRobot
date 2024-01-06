#!/bin/bash
rm -rf ~/esp
cd && mkdir esp
cd esp
git clone --depth 1  --branch release/v5.1 https://github.com/espressif/esp-idf.git
cd ./esp-idf
git submodule update --init --recursive --recommend-shallow --depth 1
bash ./install.sh esp32-c3
cd components && git clone https://github.com/espressif/arduino-esp32.git --depth 1
cd .. && cd tools && ./idf_tools.py install esp-clang
sed -i '220i set_target_properties(\${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -w)' ~/esp/esp-idf/components/arduino-esp32/CMakeLists.txt
