#!/usr/bin/python3
import sys
import os

def menuconfig():
    print('Start menu config.')
    os.system('cd config && kconfig-gconf Kconfig')
    print('Menu config done.')

def generate_cmake():
    print('Open config file.')
    config_file = open('./config/.config','r')
    cmake_file = open('./config/config.cmake','w')

    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')

    while True:
        line = config_file.readline().rstrip('\n')

        if not line:
            break;

        if line.startswith('#') and line.endswith(' is not set'):
            print(line)
            cmake_file.write('set('+line.rstrip(' is not set').strip('# ')+' OFF)\n')

        if line.endswith('=y'):
            print(line)
            cmake_file.write('set('+line.rstrip('=y')+' ON)\n')
    config_file.close()
    cmake_file.close()
    print('File closed.')

cmd = sys.argv
cmd_len = len(cmd)

if cmd_len != 2:
    print('参数错误，请查看help')

if cmd[1] == 'config':
    menuconfig()
    generate_cmake()

elif cmd[1] == 'help':
    print('青岛大学电控代码 - 帮助页面')
    print('#命令    #功能')
    print('config - 生成新配置')
