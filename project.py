#!/usr/bin/python3
import sys
import os


def menuconfig():
    print('Start menu config.')
    os.system('cd config && kconfig-gconf Kconfig')
    print('Menu config done.')


def generate_cmake(path):
    if os.path.exists(path+'/.config'):
        print('Found config file.')
    else:
        print('No config file found. Start menuconfig.')
        os.system('pwd')
        os.system('cd '+path+' && kconfig-gconf Kconfig')
    print('Open config file.')
    config_file = open(path+'/.config', 'r')
    cmake_file = open(path+'/config.cmake', 'w')

    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')

    while True:
        line = config_file.readline().rstrip('\n')

        if not line:
            break

        if line.startswith('#') and line.endswith(' is not set'):
            print('\t'+line)
            cmake_file.write(
                'set('+line.rstrip(' is not set').strip('# ')+' OFF)\n')

        if line.endswith('=y'):
            print('\t'+line)
            cmake_file.write('set('+line.rstrip('=y')+' ON)\n')
    config_file.close()
    cmake_file.close()
    print('File closed.')
    print('All done.')


cmd = sys.argv
cmd_len = len(cmd)

if cmd_len < 2:
    print('参数错误，请查看help')

if cmd[1] == 'config':
    menuconfig()
    generate_cmake('./config')

elif cmd[1] == 'help':
    print('青岛大学电控代码 - 帮助页面')
    print('#命令    #功能')
    print('config - 生成新配置')

elif cmd[1] == 'generate':
    generate_cmake(cmd[2]+'/config')

else:
    print('错误的参数')
