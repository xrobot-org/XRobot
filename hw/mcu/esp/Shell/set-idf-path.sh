#!/bin/bash

export PATH=/bin:/usr/bin
export IDF_PATH=''

if [ ! -f ~/.idf_profile ]; then
    touch ~/.idf_profile
else
    rm ~/.idf_profile
    touch ~/.idf_profile
fi

echo export OLD_PATH=$PATH >>~/.idf_profile

source ~/.idf_profile
paths=$(echo $PATH | sed 's/:/ /g')
a=$(ls -d $(echo $HOME)/.espressif/tools/*/*/*/* | grep bin)
path_list=()
for i in ${a[*]}; do
    for str in $paths; do
        if [[ "$i" = $str ]]; then
            continue 2
        fi
    done
    path_list+=($i)
done

echo $path_list

for i in ${a[*]}; do
    for str in $paths; do
        if [[ $str = $(echo $HOME)/esp/esp-idf/tools ]]; then
            break 2
        fi
    done
    path_list+=($(echo $HOME)/esp/esp-idf/tools)
    break
done

for i in ${path_list[*]}; do
    echo $i
    echo export PATH=$i:\$PATH >>~/.idf_profile
done

idf_path=$(echo $IDF_PATH)
if [[ $idf_path != $(echo $HOME)/esp/esp-idf ]]; then
    echo export IDF_PATH=$(echo $HOME)/esp/esp-idf >>~/.idf_profile
fi
source ~/.idf_profile

echo source $IDF_PATH/export.sh >>~/.idf_profile
