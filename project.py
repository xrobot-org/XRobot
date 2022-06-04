#!/usr/bin/python3
import sys
import os


def menuconfig():
    print('Start menu config.')
    os.system('cd config && kconfig-qconf Kconfig')
    print('Menu config done.')


def clean_cache():
    os.system('rm -rf ./build/*')


def config_cmake():
    os.system('cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=toolchain/toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -Bbuild -G Ninja')


def add_detail(file, name: str, value: str):
    file.write('set('+name+' '+value+')\n')
    file.write('add_compile_definitions(' +
               name.replace('CONFIG_', '')+'=${'+name+'})\n')


def generate_cmake(path):
    print('Start generate config.cmake.')
    if os.path.exists(path+'/.config'):
        print('Found config file.')
    else:
        print('No config file found. Start menuconfig.')
        os.system('cd '+path+' && kconfig-qconf Kconfig')
    config_file = open(path+'/.config', 'r')
    cmake_file = open(path+'/config.cmake', 'w')

    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')

    for lines in config_file.readlines():
        line = lines.rstrip('\n')

        if not line:
            continue

        if line.startswith('#') and line.endswith(' is not set'):
            print('[CONFIG] '+line.strip('# '))
            add_detail(cmake_file, line.rstrip(' is not set').strip('# '), '0')
            continue

        if line.endswith('=y'):
            print('[CONFIG] '+line)
            add_detail(cmake_file, line.rstrip('=y'), '1')
            continue

        if line.startswith('#'):
            continue

        temp = line.split('=')
        add_detail(cmake_file, temp[0], temp[1])

    config_file.close()
    cmake_file.close()
    print('Generate done.')


cmd = sys.argv
cmd_len = len(cmd)

if cmd_len < 2:
    print('参数错误，请查看help')

if cmd[1] == 'config':
    clean_cache()
    menuconfig()
    generate_cmake('./config')
    config_cmake()

elif cmd[1] == 'help':
    print('青岛大学电控代码\n帮助页面')
    print('#命令         -    #功能')
    print('config        -   生成新配置')
    print('refresh       -   重新生成cmake缓存')

elif cmd[1] == 'generate':
    generate_cmake(cmd[2]+'/config')

elif cmd[1] == 'refresh':
    clean_cache()
    config_cmake()

else:
    print('错误的参数')
