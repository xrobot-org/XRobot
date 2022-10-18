import sys
import os


class ProjectTools:
    project_path = ''
    kconfig_path = ''
    config_prefix = 'auto_generated_config_prefix_'

    def __init__(self):
        self.project_path = os.path.split(os.path.realpath(__file__))[0][:-13]
        self.kconfig_path = self.project_path + '/lib/Kconfiglib'

    def menuconfig(self, path):
        print('Start menu config.')
        os.system('cd ' + path + ' && ' + self.kconfig_path + '/guiconfig.py')
        print('Menu config done.')

    def clean_cache(self):
        os.system('rm -rf ' + self.project_path + '/build/*')

    def config_cmake(self):
        os.system(
            'cd ' + self.project_path +
            ' && cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=toolchain/toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -Bbuild -G Ninja'
        )

    def list_dir(self, path):
        ans = []
        for dirname in os.listdir(path):
            if os.path.isdir(path + '/' + dirname):
                ans.append(dirname)
        return ans

    def list_file(self, path):
        ans = []
        for filename in os.listdir(path):
            if os.path.isfile(path + '/' + filename):
                ans.append(filename)
        return ans

    def kconfig_add_choice(self, name, file, list):
        file.write('\n# ' + name)
        file.write('\nchoice\n\tprompt \"' + name + '"\n')

        for item in list:
            file.write('\n\tconfig ' + self.config_prefix + item +
                       '\n\t\tbool \"' + item + '\"\n')
        file.write('endchoice\n')

    def kconfig_conditional_include(self, path, file, name):
        file.write('\nif ' + self.config_prefix + name + '\n\tsource \"' +
                   path + '/' + name + '/Kconfig"\nendif\n')

    def kconfig_add_menu(self, name, file, list, path):
        file.write('\n# ' + name)
        file.write('\nmenu \"' + name + '"\n')

        for item in list:
            file.write('\n\tconfig ' + self.config_prefix + item +
                       '\n\t\ttristate \"' + item + '\"\n')
            self.kconfig_conditional_include(path, file, item)
        file.write('endmenu\n')

    def cmake_add_detail(self, file, name, value):
        name = name[7:]

        if name.startswith(self.config_prefix):
            file.write('set(' + name + ' ' + value + ')\n')
        else:
            file.write('set(' + name + ' ' + value + ')\n')
            file.write('add_compile_definitions(' + name + '=${' + name +
                       '})\n')
