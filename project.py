#!/usr/bin/python3
import sys
import os
import shutil

sys.path.insert(0, './utils/python')
sys.path.insert(0, '../utils/python')

import tool

tools = tool.ProjectTools()

welcome_str = 'QDU-RM-MCU 2023\n感谢使用本项目，使用./project.py help获取使用方法'
project_path_str = '工程目录:'

cfg_dir = tools.project_path + '/config'
bsp_dir = tools.project_path + '/hw/bsp'
src_dir = tools.project_path + '/src'
dev_dir = tools.project_path + '/src/device'
mod_dir = tools.project_path + '/src/module'
sys_dir = tools.project_path + '/src/system'
rbt_dir = tools.project_path + '/src/robot'
fm_dir = tools.project_path + '/firmware'
build_dir = tools.project_path + '/build'


def list_target():
    for dirname in tools.list_dir(bsp_dir):
        print('Board[' + dirname + ']')
        config_dir = bsp_dir + '/' + dirname + '/config'
        for filename in tools.list_file(config_dir):
            if filename.endswith('.config'):
                print('\tRobot[' + filename[0:-7] + ']')


def select_config(board, robot):
    board_dir = bsp_dir + '/' + board
    config_path = bsp_dir + '/' + board + '/config/' + robot + '.config'

    if not os.path.exists(board_dir):
        print('Error:Board not found.')
        exit(-1)

    if not os.path.exists(config_path):
        print('Error:Robot not found.')
        exit(-1)

    shutil.copyfile(config_path, cfg_dir + '/.config')
    os.system("cd " + tools.project_path + ' && ./project.py refresh')

    print('Load config success for [' + board + ']', robot)


def build(board, robot):
    target = []

    os.system("rm -rf " + fm_dir)
    for dirname in tools.list_dir(bsp_dir):
        config_dir = bsp_dir + '/' + dirname + '/config'

        for filename in tools.list_file(config_dir):
            if (board == 'all'
                    or dirname == board) and filename.endswith('.config'):
                if filename.endswith(".config") and (robot == 'all' or
                                                     filename[:-7] == robot):
                    select_config(dirname, filename[:-7])
                    os.system("cd " + build_dir + ' && ninja')
                    os.makedirs(fm_dir, exist_ok=True)
                    shutil.copyfile(
                        tools.project_path + '/build/src/qdu_rm_mcu.elf',
                        fm_dir + '/' + dirname + '&' + filename[:-7] + '.elf')

                    print('\n')

                    target.append([dirname, filename[:-7]])
    if len(target) == 0:
        print('ERROR:No target select.')
    else:
        for item in target:
            print('Robot[' + item[1] + ']' + ' for board ' + item[0] +
                  ' build done.')


def generate_kconfig():
    print("Start generate Kconfig.")
    kconfig_file = open(tools.project_path + '/config/auto.Kconfig', 'w')
    kconfig_file.write('# QDU-RM-MCU\n')
    kconfig_file.write('# Auto generated file. Do not edit.\n')
    kconfig_file.write('# -----------------------------------------------')

    tools.kconfig_add_choice('开发板/board', kconfig_file,
                             tools.list_dir(bsp_dir), bsp_dir)
    tools.kconfig_add_choice('系统/system', kconfig_file,
                             tools.list_dir(sys_dir), sys_dir)
    tools.kconfig_add_choice('机器人/robot', kconfig_file,
                             tools.list_dir(rbt_dir), rbt_dir)
    tools.kconfig_add_menu('设备/device', kconfig_file, tools.list_dir(dev_dir),
                           dev_dir)
    tools.kconfig_add_menu('模块/module', kconfig_file, tools.list_dir(mod_dir),
                           mod_dir)

    kconfig_file.close()
    print("Generate Kconfig done.")


def generate_cmake(path):
    print('Start generate config.cmake.')
    if os.path.exists(path + '/.config'):
        print('Found config file.')
    else:
        tools.menuconfig(path)
    config_file = open(path + '/.config', 'r')
    cmake_file = open(path + '/config.cmake', 'w')

    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')

    cmake_file.write('set(CONFIG_PREFIX  ' + tools.config_prefix + ')\n')

    for lines in config_file.readlines():
        line = lines.rstrip('\n')

        if not line:
            continue

        if line.startswith('#') and line.endswith(' is not set'):
            print('[CONFIG] ' + line.strip('# '))
            tools.cmake_add_detail(cmake_file, line[2:-11], '0')
            continue

        if line.endswith('=y'):
            print('[CONFIG] ' + line)
            tools.cmake_add_detail(cmake_file, line[:-2], '1')
            continue

        if line.startswith('#'):
            continue

        temp = line.split('=')
        tools.cmake_add_detail(cmake_file, temp[0], temp[1])

    config_file.close()
    cmake_file.close()
    print('Generate done.')


cmd = sys.argv
cmd_len = len(cmd)

if cmd_len < 2:
    print('参数错误，请输入./project.py help')
    exit()

if cmd[1] == 'config':
    tools.clean_cache()
    generate_kconfig()
    tools.menuconfig(cfg_dir)
    generate_cmake(cfg_dir)
    tools.config_cmake()

elif cmd[1] == 'help':
    print(welcome_str)
    print(project_path_str + tools.project_path)
    print('帮助页面/help')
    print('#命令                  -   #功能')
    print('help                   -   显示本页面')
    print('config                 -   生成新配置')
    print('refresh                -   重新生成cmake缓存')
    print('clean                  -   清除编译产物')
    print('build [BOARD] [ROBOT]  -   构建目标')
    print('list                   -   列出可构建目标')
    print('init                   -   安装必备软件包')
    print('select [BOARD] [ROBOT] -   选择构建目标')

elif cmd[1] == 'generate':
    generate_cmake(cmd[2] + '/config')

elif cmd[1] == 'refresh':
    tools.clean_cache()
    tools.config_cmake()

elif cmd[1] == 'clean':
    tools.clean_cache()
    print('clean done.')

elif cmd[1] == 'build':
    if (cmd_len < 4):
        print('参数错误')
        exit(-1)
    build(cmd[2], cmd[3])
elif cmd[1] == 'list':
    list_target()
elif cmd[1] == 'init':
    os.system(
        'sudo apt install cmake gcc-arm-none-eabi ninja-build python3-tk')
elif cmd[1] == 'select':
    if (cmd_len < 4):
        print('参数错误')
        exit(-1)
    select_config(cmd[2], cmd[3])
else:
    print('参数错误，请输入./project.py help')

exit(0)
