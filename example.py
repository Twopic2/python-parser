def greet(name):
    message = "Hello, " + name
    print(message)
    return message

c = 1
c += 1

def switch_case(value):
    match value:
        case 1:
            return "one"
        case 2:
            return "two"

x = 42
y = 3.14
greet("World")
