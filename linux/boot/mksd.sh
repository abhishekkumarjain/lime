#!/bin/bash
# eg. mksd.sh /dev/mmcblk0
run_cmd_exit_on_fail(){
	eval $1;
	error_code=$?;
	if [ $error_code != "0" ]; then 
		echo "failed:" $1;
		exit $error_code
	fi
}
if [ ! -b "${1}" ]; then
	echo "Usage: ${0} <path to SD device>"
	exit 1
fi
[ -z "${1##*mmcblk*}" ] && PT="p"
sudo umount ${1}*
sudo parted -s ${1} mklabel msdos
sudo parted -s ${1} -- mkpart primary fat32 1mib 1048575s
sudo parted -s ${1} -- mkpart primary ext4 512mib -1mib
sudo mkfs.vfat ${1}${PT}1
sudo mkfs.ext4 ${1}${PT}2
sudo mkdir -p /mnt/sd-boot
run_cmd_exit_on_fail "sudo mount ${1}${PT}1 /mnt/sd-boot"
sudo tar --no-same-owner -xJf bootfiles.tar.xz -C /mnt/sd-boot
sudo umount /mnt/sd-boot
sudo mkdir -p /mnt/sd-root
run_cmd_exit_on_fail "sudo mount ${1}${PT}2 /mnt/sd-root"
sudo tar -pxJf rootfs.tar.xz -C /mnt/sd-root
sudo umount /mnt/sd-root
