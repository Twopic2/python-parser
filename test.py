def greet(name):
    message = "Hello, " + name
    print(message)
    return message

def pass1():
    pass

def hello():
    def hello2():
        print("hello")
    print("hello")

c = 1
c += 1

plus = 3 * 2 - 1
plus2 = 1 + 2 * 3

while c:
    print("hello")

def switch_case(value):
    match value:
        case 1:
            return "one"
        case 2:
            return "two"
        
class new_world:
    def __init__(self, world):
        self.world = world
    
wrld = new_world("world")

for i in x:
    i += 1
    j = 0
    z = 1
    if i:
        break
    elif j:
        continue
    else:
        z

while i:
    if i:
        break
    elif j:
        continue
    else:
        z
         
if x == 2:
    print("no")
elif y:
    print("hi")
else:
    print("hello")

x = 42
y = 3.14
greet("World")

try:
  print("Hello")
except:
  print("Something went wrong")
else:
    print("else")

try:
  print(x)
except:
  print("Something went wrong")
finally:
  print("The 'try except' is finished")