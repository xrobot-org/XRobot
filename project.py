#!/usr/bin/python3
import sys
import os

def menuconfig():
    os.system('cd config && kconfig-gconf Kconfig')


cmd = sys.argv
cmd_len = len(cmd)

if(cmd[1] == 'config'):
    menuconfig()
