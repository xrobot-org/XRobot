#!/usr/bin/python3
import sys
import os
import shutil

project_path = os.path.split(os.path.realpath(__file__))[0]


def build_all():
    os.system("rm -rf " + project_path + '/firmware')
    for dirname in os.listdir(project_path + '/hw/bsp'):
        if os.path.isdir(project_path + '/hw/bsp/' + dirname):
            for filename in os.listdir(project_path + '/hw/bsp/' + dirname +
                                       '/config'):
                if os.path.isfile(project_path + '/hw/bsp/' + dirname +
                                  '/config/' + filename):
                    if filename.endswith(".config"):
                        shutil.copyfile(
                            project_path + '/hw/bsp/' + dirname + '/config/' +
                            filename, project_path + '/config/.config')
                        os.system(
                            "cd " + project_path +
                            ' && ./project.py refresh && cd build && ninja')
                        os.makedirs(project_path + '/firmware', exist_ok=True)
                        shutil.copyfile(
                            project_path + '/build/src/qdu_rm_mcu.elf',
                            project_path + '/firmware/' + dirname + '&' +
                            filename[:-7] + '.elf')


def menuconfig(path):
    print('Start menu config.')
    os.system('cd ' + path + ' && ' + project_path +
              '/lib/Kconfiglib/guiconfig.py')
    print('Menu config done.')


def clean_cache():
    os.system('rm -rf ./build/*')


def config_cmake():
    os.system(
        'cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=toolchain/toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -Bbuild -G Ninja'
    )


def add_detail(file, name: str, value: str):
    name = name[7:]
    file.write('set(' + name + ' ' + value + ')\n')
    file.write('add_compile_definitions(' + name + '=${' + name + '})\n')


def foreach_config_single(head, file, path):
    file.write('\n# ' + head)
    file.write('\nchoice\n\tprompt \"' + head + '"\n')

    for dirname in os.listdir(path):
        if os.path.isdir(path + '/' + dirname):
            file.write('\n\tconfig _SUB_CFG_' + dirname + '\n\t\tbool \"' +
                       dirname + '\"\n')
    file.write('endchoice\n')

    for dirname in os.listdir(path):
        if os.path.isdir(path + '/' + dirname):
            file.write('\nif ' + '_SUB_CFG_' + dirname + '\n\tsource \"' +
                       path + '/' + dirname + '/Kconfig"\nendif\n')


def foreach_config(head, file, path):
    file.write('\n# ' + head)
    file.write('\nmenu \"' + head + '"\n')

    for dirname in os.listdir(path):
        if os.path.isdir(path + '/' + dirname):
            file.write('\nmenu \"' + dirname + '"\n')
            file.write('\n\tconfig _SUB_CFG_' + dirname + '\n\t\ttristate \"' +
                       dirname + '\"\n')

            file.write('\nif ' + '_SUB_CFG_' + dirname + '\n\tsource \"' +
                       path + '/' + dirname + '/Kconfig"\nendif\n')
            file.write('endmenu\n')
    file.write('endmenu\n')


def generate_kconfig():
    print("Start generate Kconfig.")
    cmake_file = open(project_path + '/config/auto.Kconfig', 'w')
    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')
    cmake_file.write('# -----------------------------------------------')

    foreach_config_single('开发板/board', cmake_file, project_path + '/hw/bsp')
    foreach_config_single('机器人/robot', cmake_file, project_path + '/src/robot')
    foreach_config('设备/device', cmake_file, project_path + '/src/device')
    foreach_config('模块/module', cmake_file, project_path + '/src/module')

    cmake_file.close()
    print("Generate Kconfig done.")


def generate_cmake(path):
    print('Start generate config.cmake.')
    if os.path.exists(path + '/.config'):
        print('Found config file.')
    else:
        menuconfig(path)
        print(path)
    config_file = open(path + '/.config', 'r')
    cmake_file = open(path + '/config.cmake', 'w')

    cmake_file.write('# QDU-RM-MCU\n')
    cmake_file.write('# Auto generated file. Do not edit.\n')

    for lines in config_file.readlines():
        line = lines.rstrip('\n')

        if not line:
            continue

        if line.startswith('#') and line.endswith(' is not set'):
            print('[CONFIG] ' + line.strip('# '))
            add_detail(cmake_file, line[2:-11], '0')
            continue

        if line.endswith('=y'):
            print('[CONFIG] ' + line)
            add_detail(cmake_file, line[:-2], '1')
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
    generate_kconfig()
    menuconfig(project_path + '/config')
    generate_cmake(project_path + '/config')
    config_cmake()

elif cmd[1] == 'help':
    print('青岛大学电控代码\n帮助页面')
    print('#命令         -    #功能')
    print('config        -   生成新配置')
    print('refresh       -   重新生成cmake缓存')

elif cmd[1] == 'generate':
    generate_cmake(cmd[2] + '/config')

elif cmd[1] == 'refresh':
    clean_cache()
    config_cmake()

elif cmd[1] == 'build':
    if cmd[2] == 'all':
        print("Build all target.")
        build_all()

else:
    print('错误的参数')
