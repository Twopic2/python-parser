import dis

def f(x):
    return x + 1

dis.dis(dis.getsource(f))
