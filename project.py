#!/usr/bin/python3
import sys
import os
import shutil

sys.path.insert(0, './utils/python')
sys.path.insert(0, '../utils/python')

import tool

tools = tool.ProjectTools()

welcome_str = 'XRobot 2023\n感谢使用本项目，使用./project.py help获取使用方法'
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
user_dir = tools.project_path + '/user'
user_bsp_dir = user_dir + '/bsp'
user_cmp_dir = user_dir + '/component'
user_dev_dir = user_dir + '/device'
user_mod_dir = user_dir + '/module'
user_sys_dir = user_dir + '/system'
user_rbt_dir = user_dir + '/robot'


def list_target():
    print("Offical:")
    for dirname in tools.list_dir(bsp_dir):
        print('  Board[' + dirname + ']')
        config_dir = bsp_dir + '/' + dirname + '/config'
        for filename in tools.list_file(config_dir):
            if filename.endswith('.config'):
                print('    Robot[' + filename[0:-7] + ']')
    print("User:")
    for dirname in tools.list_dir(user_bsp_dir):
        print('  Board[' + dirname + ']')
        config_dir = user_bsp_dir + '/' + dirname + '/config'
        for filename in tools.list_file(config_dir):
            if filename.endswith('.config'):
                print('    Robot[' + filename[0:-7] + ']')


def select_config(config_path, type):

    if not os.path.exists(config_path):
        print('Error:Robot not found.')
        exit(-1)

    shutil.copyfile(config_path, cfg_dir + '/.config')
    tools.clean_cache()
    tools.config_cmake(type)

    print('Load config success from', config_path)


def build(board, robot, type='Debug'):
    target = []

    os.system("rm -rf " + fm_dir)
    os.makedirs(fm_dir, exist_ok=True)

    if board == 'all':
        for dirname in tools.list_dir(bsp_dir):
            target.append([bsp_dir + '/' + dirname, []])
        for dirname in tools.list_dir(user_bsp_dir):
            target.append([user_bsp_dir + '/' + dirname, []])
    else:
        for dirname in tools.list_dir(bsp_dir):
            if dirname == board:
                target.append([bsp_dir + '/' + dirname, []])
        for dirname in tools.list_dir(user_bsp_dir):
            if dirname == board:
                target.append([user_bsp_dir + '/' + dirname, []])

    if len(target) == 0:
        print('ERROR:No target select.')
        return

    for i in range(len(target)):
        config_dir = target[i][0] + '/config'
        for filename in tools.list_file(config_dir):
            if filename.endswith(".config") and (robot == 'all'
                                                 or robot == filename[:-7]):
                target[i][1].append(filename[:-7])
                print('Select config ' + target[i][0] + ' ' + filename)
        if (len(target[i][1]) == 0):
            print('ERROR:No target select.')
            return

    for item in target:
        bsp = item[0]
        for rbt in item[1]:
            config_file = bsp + '/config/' + rbt + '.config'
            fm_board_dir = fm_dir + '/' + bsp.split('/')[-1]
            fm_robot_dir = fm_board_dir + '/' + rbt
            select_config(config_file, type)
            os.system("cd " + build_dir + ' && ninja')
            if not os.path.exists(build_dir + '/' + 'xrobot.elf'):
                print('\033[0;31;40mBuild ' + bsp + ' [' + rbt +
                      '] failed.\033[0m')
                exit(-1)
            os.makedirs(fm_board_dir, exist_ok=True)
            os.makedirs(fm_robot_dir, exist_ok=True)
            dirlist = os.walk(build_dir, )
            for root, dirs, files in dirlist:
                for file in files:
                    tmp_path = root[len(build_dir):]
                    if tmp_path.count('/') > 1:
                        continue
                    if len(tmp_path) == 0:
                        tmp_path = '/'
                    else:
                        tmp_path = tmp_path + '/'
                    if file.endswith('.hex') or file.endswith(
                            '.bin') or file.endswith('.elf'):
                        print('Found object', file)
                        os.makedirs(fm_robot_dir + '/' + tmp_path,
                                    exist_ok=True)
                        shutil.copyfile(
                            root + '/' + file,
                            fm_robot_dir + tmp_path + file.split('/')[-1])
                        os.system('md5sum ' + fm_robot_dir + tmp_path +
                                  file.split('/')[-1] + ' > ' + fm_robot_dir +
                                  tmp_path + file.split('/')[-1] + '.md5')
    for item in target:
        bsp = item[0]
        for rbt in item[1]:
            print('\033[0;32;40mRobot[' + rbt + ']' + ' for board ' + bsp +
                  ' build done.\033[0m')


def select(board, robot):
    config_file = ''
    for dirname in tools.list_dir(bsp_dir):
        if dirname == board:
            config_file = bsp_dir + '/' + dirname + '/config/' + robot + '.config'
    for dirname in tools.list_dir(user_bsp_dir):
        if dirname == board:
            config_file = user_bsp_dir + '/' + dirname + '/config/' + robot + '.config'
    if len(config_file) == 0:
        print('No config file found')
        exit(-1)
    select_config(config_file, 'Debug')


def generate_kconfig():
    print("Start generate Kconfig.")
    kconfig_file = open(tools.project_path + '/config/auto.Kconfig', 'w')
    kconfig_file.write('# XRobot\n')
    kconfig_file.write('# Auto generated file. Do not edit.\n')
    kconfig_file.write('# -----------------------------------------------')

    tools.kconfig_add_choice(
        'board-', '开发板', kconfig_file,
        [tools.list_dir(bsp_dir),
         tools.list_dir(user_bsp_dir)], [bsp_dir, user_bsp_dir])
    tools.kconfig_add_choice(
        'system-', '系统', kconfig_file,
        [tools.list_dir(sys_dir),
         tools.list_dir(user_sys_dir)], [sys_dir, user_sys_dir])
    tools.kconfig_add_choice(
        'robot-', '机器人', kconfig_file,
        [tools.list_dir(rbt_dir),
         tools.list_dir(user_rbt_dir)], [rbt_dir, user_rbt_dir])
    tools.kconfig_add_menu('component-', '组件', kconfig_file,
                           [tools.list_dir(user_cmp_dir)], [user_cmp_dir])
    tools.kconfig_add_menu(
        'device-', '设备', kconfig_file,
        [tools.list_dir(dev_dir),
         tools.list_dir(user_dev_dir)], [dev_dir, user_dev_dir])
    tools.kconfig_add_menu(
        'module-', '模块', kconfig_file,
        [tools.list_dir(mod_dir),
         tools.list_dir(user_mod_dir)], [mod_dir, user_mod_dir])

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

    cmake_file.write('# XRobot\n')
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


def new_component(name: str):
    cppfile = open(tools.project_path + '/src/component/comp_' + name + '.cpp',
                   mode='w+')
    cppfile.write('#include \"comp_' + name +
                  '.hpp\"\n\nusing namespace Component;\n')
    cppfile.close()
    hppfile = open(tools.project_path + '/src/component/comp_' + name + '.hpp',
                   mode='w+')
    hppfile.write('#include <component.hpp>\n\nnamespace Component {\n' +
                  'class YourComponentName {\n' + ' public:\n};\n' +
                  '}  // namespace Component')
    hppfile.close()
    print('Create new component [' + name + '] done.')


def new_device(name: str):
    os.mkdir(tools.project_path + '/src/device/' + name)
    cppfile = open(tools.project_path + '/src/device/' + name + '/dev_' +
                   name + '.cpp',
                   mode='w+')
    cppfile.write('#include \"dev_' + name +
                  '.hpp\"\n\nusing namespace Device;\n')
    cppfile.close()
    hppfile = open(tools.project_path + '/src/device/' + name + '/dev_' +
                   name + '.hpp',
                   mode='w+')
    hppfile.write('#include \"device.hpp\"\n\nnamespace Device {\n' +
                  'class YourDeviceName {\n' + ' public:\n};\n' +
                  '}  // namespace Device')
    hppfile.close()
    configfile = open(tools.project_path + '/src/device/' + name + '/Kconfig',
                      mode='w+')
    configfile.close()
    infofile = open(tools.project_path + '/src/device/' + name + '/info.cmake',
                    mode='w+')
    infofile.write(
        'CHECK_SUB_ENABLE(MODULE_ENABLE device)\nif(${MODULE_ENABLE})\n    '
        'file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")\n    SUB_ADD_SRC(CUR_SOURCES)\n    SUB_ADD_INC(SUB_DIR)\nendif()'
    )
    infofile.close()
    print('Create new device [' + name + '] done.')


def new_module(name: str):
    os.mkdir(tools.project_path + '/src/module/' + name)
    cppfile = open(tools.project_path + '/src/module/' + name + '/mod_' +
                   name + '.cpp',
                   mode='w+')
    cppfile.write('#include \"mod_' + name +
                  '.hpp\"\n\nusing namespace Module;\n')
    cppfile.close()
    hppfile = open(tools.project_path + '/src/module/' + name + '/mod_' +
                   name + '.hpp',
                   mode='w+')
    hppfile.write('#include \"module.hpp\"\n\nnamespace Module {\n' +
                  'class YourModuleName {\n' + ' public:\n};\n' +
                  '}  // namespace Module')
    hppfile.close()
    configfile = open(tools.project_path + '/src/module/' + name + '/Kconfig',
                      mode='w+')
    configfile.close()
    infofile = open(tools.project_path + '/src/module/' + name + '/info.cmake',
                    mode='w+')
    infofile.write(
        'CHECK_SUB_ENABLE(MODULE_ENABLE module)\nif(${MODULE_ENABLE})\n    '
        'file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")\n'
        '    SUB_ADD_SRC(CUR_SOURCES)\n    '
        'SUB_ADD_INC(SUB_DIR)\nendif()')
    infofile.close()
    print('Create new module [' + name + '] done.')


def new_robot(name: str):
    os.mkdir(tools.project_path + '/src/robot/' + name)
    cppfile = open(tools.project_path + '/src/robot/' + name + '/robot.cpp',
                   mode='w+')
    cppfile.write(
        '#include \"robot.hpp\"\n\n#include <system.hpp>\n#include <thread.hpp>\n\nusing namespace Robot;\n\n'
    )
    cppfile.write(
        '/* clang-format off */\n//TODO: write your param\n/* clang-format on */\n\n'
    )
    cppfile.write(
        'void robot_init() {\n  System::Start<Robot::YourRobotName, Robot::YourRobotName::Param>(param);\n}\n'
    )

    cppfile.close()
    hppfile = open(tools.project_path + '/src/robot/' + name + '/robot.hpp',
                   mode='w+')
    hppfile.write(
        '/* #include "dev_xxx.hpp" */\n\nvoid robot_init();\n\nnamespace Robot {\n'
        + 'class YourRobotName {\n' + ' public:\n};\n' +
        '}  // namespace Robot')
    hppfile.close()
    configfile = open(tools.project_path + '/src/robot/' + name + '/Kconfig',
                      mode='w+')
    configfile.close()
    print('Create new Robot [' + name + '] done.')


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
    print('build  [BOARD] [ROBOT] -   构建目标')
    print('list                   -   列出可构建目标')
    print('init                   -   安装必备软件包')
    print('select [BOARD] [ROBOT] -   选择构建目标')
    print('new    [TYPE]  [NAME]  -   新建模块')

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

elif cmd[1] == 'build-debug':
    if (cmd_len < 4):
        print('参数错误')
        exit(-1)
    build(cmd[2], cmd[3], 'Debug')

elif cmd[1] == 'build-release':
    if (cmd_len < 4):
        print('参数错误')
        exit(-1)
    build(cmd[2], cmd[3], 'Release')
elif cmd[1] == 'list':
    list_target()
elif cmd[1] == 'init':
    os.system(
        'sudo apt install cmake gcc-arm-none-eabi ninja-build python3-tk clang clangd'
    )
elif cmd[1] == 'select':
    if (cmd_len < 4):
        print('参数错误')
        exit(-1)
    select(cmd[2], cmd[3])
elif cmd[1] == 'new':
    if (cmd_len < 4):
        print('参数错误')
    try:
        eval('new_' + cmd[2] + '(cmd[3])')
    except:
        print('命令错误')
else:
    print('参数错误，请输入./project.py help')

exit(0)
