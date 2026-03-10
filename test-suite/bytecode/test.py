import dis

code = """

x = 1
y = 2


print(x)
print(y)
"""

dis.dis(code)