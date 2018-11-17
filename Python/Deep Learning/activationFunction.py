#  Kind of Activation Funtion - Sigmoid Function, ReLU Function(Rectified Line Unit)
import numpy as np
import matplotlib.pyplot as plt

def sigmoid(x) :
    return 1 / (1 + np.exp(-x))

def relu(x) :
    return np.maximum(0,x)

x1 = float(input("input x1 . . . : "))
x2 = float(input("input x2 . . . : "))
x3 = float(input("input x2 . . . : "))
x = np.arange(x1,x2,x3)

print(x)
print("Sigmoid Function : ")
print(sigmoid(x))


y = sigmoid(x)

plt.plot(x,y)
plt.ylim(-0.1,1.1)
plt.show()
