#/bin/sh
# __unused=$1
#gawk  -v $1 -f ph_depend.awk $1
gawk  -v src=ru.dz.phantom.system.shell.ph -f ph_depend.awk ru.dz.phantom.system.shell.ph 
#  >setup.py
