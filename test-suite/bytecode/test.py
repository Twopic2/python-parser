import dis

code = """
z = 3 + 1

x = 3 * 3

x = 3 / 3
"""

dis.dis(code)