mkdir extract
cd extract
..\bin\pfsextract ../phantom.img
cmp snapcopy.img extract/last_snap.data 