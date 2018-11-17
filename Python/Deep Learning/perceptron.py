# - Perceptron : AND Gate materialization
w1, w2, theta = 0.5, 0.5, 0.7
def AND(x1, x2):
    tmp = x1*w1 + x2*w2
    if tmp <= theta:
        return False
    elif tmp > theta:
        return True

while 1 :
    x1 = float(input("intput x1 . . . : "))
    x2 = float(input("input x2 . . . : "))

    print("AND %3.3f + %3.3f : %f < %f (%s)" %(x1*w1, x2*w2, theta, x1*w1 + x2*w2, AND(x1,x2)))
