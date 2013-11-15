#!/bin/bash

dest=$1
if [ "x$dest" == "x" ]; then
    dest=/var/p2pfoodlab
fi

owner=$2
if [ "x$owner" == "x" ]; then
    owner=$(id -u -n)
fi

group=$3
if [ "x$group" == "x" ]; then
    group=$(id -g -n)
fi

install --directory --owner=$owner --group=$group --mode=0755 $dest
install --directory --owner=$owner --group=$group --mode=0755 $dest/src
install --directory --owner=$owner --group=$group --mode=0755 $dest/lib
install --directory --owner=$owner --group=$group --mode=0755 $dest/bin
install --directory --owner=$owner --group=$group --mode=0755 $dest/web

source ./files.txt

for file in ${src[@]}; do
    install --owner=$owner --group=$group --mode=0644 $file $dest/src/
done

for file in ${web[@]}; do
    install --owner=$owner --group=$group --mode=0644 $file $dest/web/
done

for file in ${script[@]}; do
    install --owner=$owner --group=$group --mode=0755 $file $dest/bin/
done

for file in ${bin[@]}; do
    if [ -e $file ]; then
        install --owner=$owner --group=$group --mode=0755 $file $dest/bin/
    fi
done

for file in ${lib[@]}; do
    install --owner=$owner --group=$group --mode=0644 $file $dest/lib/
done
