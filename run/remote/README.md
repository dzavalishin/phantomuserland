# Remote execution on a real hardware

This directory contains scripts used to run Phantom
on a remote computer.

## Prequisites

*  Target PC wunning Ubuntu (or any Unix with grub as bootloader)
*  Free disk partition on a target PC which can be formatted as Phantom disk - IDE, not SATA drive!
*  Network connection between this computer and target one
*  Good to have: hardware watchdog for a target PC
*  Good to have: serial connection to target serial port for debug info

## Configuration

A `target_config.sh` script must contain settings for host name
and remote directory, login, etc.

An /mnt/boot/phantom/ directory on target PC must be on a disk which is 
accessible by grub during boot

## Format Phantom OS disk

Just dd =if=../img/phantom.superblock of=/dev/target_disk

## Scripts

The `target` subdir contains scripts to be run on remote system.

* `target_upload.sh` - send scripts on a target PC
* `target_reboot.sh` - reboot target host into the Phantom OS


## Debugging

Phantom krenel will print diagnostics info into the first
serial port @0x3F8 at 115200 baud.


## Grub config

There must be following code in `grub.cfg` on target machine:

```
### BEGIN /etc/grub.d/41_custom ###
if [ -f  ${config_directory}/custom.cfg ]; then
  source ${config_directory}/custom.cfg
elif [ -z "${config_directory}" -a -f  $prefix/custom.cfg ]; then
  source $prefix/custom.cfg;
fi
### END /etc/grub.d/41_custom ###
```
