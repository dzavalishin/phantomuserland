def system(cmd):
    import os
    return os.system(cmd)

def load(fname):
    if fname == 'ok':
        r = 2 + fname
    else:
        r = 3 + fname
    return r
