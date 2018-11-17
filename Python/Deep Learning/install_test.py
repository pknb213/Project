import numpy as np
import matplotlib.pyplot as plt

class Man:
    def __init__(self, name):
        self.name = name
        print("Initialized")

    def hello(self):
        print("Hello " + self.name + "!")

    def goodbye(self):
        print("goodbye " + self.name + "!")

m = Man("Cheon")
m.hello()
m.goodbye()

a = np.array([[1,3,5,7],[2,4,6,8]])

print (a)
print("------------")
print(a.shape)
print("------------")
print(a.dtype)
print("------------")
a = a*2.5
print("a*2 : ")
print(a)
print("------------")
print(a.dtype)

b = a.flatten()
print(b)

x = np.arange(0,6,0.1) # 0 ~ 6, 0.1 interval
y = np.sin(x)

plt.plot(x,y)
plt.show()
