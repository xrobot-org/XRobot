import sys
import os
import shutil


class ProjectTools:
    project_path = ""
    kconfig_path = ""
    config_prefix = "auto_generated_config_prefix_"

    def __init__(self):
        self.project_path = os.path.split(os.path.realpath(__file__))[0][:-13]
        self.project_path = self.project_path.replace("\\", "/")
        self.kconfig_path = self.project_path + "/lib/Kconfiglib"
        if " " in self.project_path:
            print("工程路径请不要带有空格")
            exit()

        for ch in self.project_path:
            if "\u4e00" <= ch <= "\u9fff":
                print("工程路径请不要带有中文")
                exit()

    def guiconfig(self, path):
        print("Start menu config.")
        os.system("cd " + path + " && " + self.kconfig_path + "/guiconfig.py")
        print("Menu config done.")

    def menuconfig(self, path):
        print("Start menu config.")
        os.system("cd " + path + " && " + self.kconfig_path + "/menuconfig.py")
        print("Menu config done.")

    def clean_cache(self):
        filepath = self.project_path + "/build"

        if not os.path.exists(filepath):
            os.mkdir(filepath)
            return

        del_list = os.listdir(filepath)

        for f in del_list:
            file_path = os.path.join(filepath, f)
            if os.path.isfile(file_path):
                os.remove(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)

    def config_cmake(self, type="Debug"):
        os.system(
            "cd "
            + self.project_path
            + " && cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=utils/CMake/toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING="
            + type
            + " -Bbuild -G Ninja"
        )

    def config_cmake_idf(self, type="Debug"):
        os.system(
            'bash -c "source ~/.idf_profile && cd '
            + self.project_path
            + " && cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=utils/CMake/toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING="
            + type
            + ' -Bbuild -G Ninja"'
        )

    def list_dir(self, path):
        ans = []
        for dirname in os.listdir(path):
            if os.path.isdir(path + "/" + dirname):
                ans.append(dirname)
        return ans

    def list_file(self, path):
        ans = []
        for filename in os.listdir(path):
            if os.path.isfile(path + "/" + filename):
                ans.append(filename)
        return ans

    def kconfig_add_choice(self, prefix, name, file, list, path):
        file.write("\n# " + name)
        file.write('\nchoice\n\tprompt "' + name + '"\n')
        for i in range(len(path)):
            for item in list[i]:
                file.write(
                    "\n\tconfig "
                    + self.config_prefix
                    + prefix
                    + item
                    + '\n\t\tbool "'
                    + prefix
                    + item
                    + '"\n'
                )
        file.write("endchoice\n")

        for i in range(len(path)):
            for item in list[i]:
                self.kconfig_conditional_include(prefix, path[i], file, item)

    def kconfig_conditional_include(self, prefix, path, file, name):
        kconfig_file_path = path + "/" + name + "/Kconfig"
        if not os.path.exists(kconfig_file_path):
            print(
                "\033[0;31;40mError:Kconfig file ["
                + kconfig_file_path
                + "] not found.\033[0m"
            )
            exit(-1)
        file.write(
            "\nif "
            + self.config_prefix
            + prefix
            + name
            + '\n\tsource "'
            + path
            + "/"
            + name
            + '/Kconfig"\nendif\n'
        )

    def kconfig_add_menu(self, prefix, name, file, list, path):
        file.write("\n# " + name)
        file.write('\nmenu "' + name + '"\n')
        for i in range(len(path)):
            for item in list[i]:
                file.write(
                    "\n\tconfig "
                    + self.config_prefix
                    + prefix
                    + item
                    + '\n\t\ttristate "'
                    + prefix
                    + item
                    + '"\n'
                )
                self.kconfig_conditional_include(prefix, path[i], file, item)
        file.write("endmenu\n")

    def cmake_add_detail(self, file, name, value):
        name = name[7:]

        if name.startswith(self.config_prefix):
            file.write("set(" + name + " " + value + ")\n")
        else:
            file.write("set(" + name + " " + value + ")\n")
            file.write("add_compile_definitions(" + name + "=${" + name + "})\n")
