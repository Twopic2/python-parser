import dis

code = """
x = 1

if x:
    x
"""


dis.dis(code)