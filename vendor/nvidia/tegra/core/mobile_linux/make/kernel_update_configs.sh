#! /bin/bash

p4 edit ${KERN_DIR}/arch/arm/configs/tegra*
for config_file in  ${KERN_DIR}/arch/arm/configs/tegra*
do
  make -C ${KERN_DIR} ARCH=arm `basename ${config_file}`
  make -C ${KERN_DIR} ARCH=arm menuconfig; 
  cp -f ${KERN_DIR}/.config ${config_file}
done

