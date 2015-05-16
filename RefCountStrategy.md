# Rule #

Reference count is incremented on allocation, and dup. Reference count is decremented on drop, object field overwrite and other object loss.


# Details #

See:
  * ref\_inc\_o, ref\_dec\_o