class myclass:
  def load(fname):
    if fname == 'ok':
        r = 2 + fname
    else:
        r = 3 + fname
    return r

  def system(cmd):
    #os = 2;
    import os
    return os.system(cmd, 'hi', 'there')
    #load(cmd)

