import dis

code = """

add(x):
    return x + 1

y = 0

z = add(y)

"""

dis.dis(code)