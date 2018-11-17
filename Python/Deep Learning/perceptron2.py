import numpy as np

def AND (x1,x2) :
    x = np.array([x1,x2])
    w = np.array([0.5,0.5])
    b = -0.7
    tmp = np.sum(x*w) + b
    if tmp <= 0 :
        return 0
    else :
        return 1

def NAND (x1, x2) :
    x = np.array([x1, x2])
    w = np.array([-0.5, -0.5])
    b = 0.7
    tmp = np.sum(w*x) + b
    if tmp <= 0 :
        return 0
    else :
        return 1


def OR (x1,x2) :
    x = np.array([x1, x2])
    w = np.array([0.5, 0.5])
    b = -0.2
    tmp = np.sum(w*x) + b
    if tmp <= 0 :
        return 0
    else :
        return 1

def XOR(x1, x2) :
    s1 = NAND(x1,x2)
    s2 = OR(x1, x2)
    y = AND(s1, s2)
    return y


while 1 :
    x1 = float(input("intput x1 . . . : "))
    x2 = float(input("input x2 . . . : "))

    menu = int(input("1: AND  2: NAND  3: OR  4: XOR"))
    if type(menu) != type(1) :
        print("plase int vaule")
        continue

    if menu == 1 :
        print("AND(%3.3f, %3.3f) : %d" %(x1, x2, AND(x1,x2)))
    elif menu == 2 :
        print("NAND(%3.3f, %3.3f) : %d" %(x1, x2, NAND(x1,x2)))
    elif menu == 3 :
        print("OR(%3.3f, %3.3f) : %d" %(x1, x2, OR(x1,x2)))
    elif menu == 4 :
        print("XOR(%3.3f, %3.3f) : %d" %(x1, x2, XOR(x1,x2)))
    else :
        print("Wrong")
