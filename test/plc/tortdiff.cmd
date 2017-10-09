call remove_ip_tail.cmd %1 a
call remove_ip_tail.cmd %2 b
tortoisemerge a b
del a b
