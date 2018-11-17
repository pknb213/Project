import numpy as np

def identify_function(x) :
    return x

def init_network() :
    network = {}
    #

    return network

def forward(network, x) :
    #
    y = identify_function(x) #tmp

    return y


network = init_network()
x = np.array([1.0, 0.5])
y = forward(network, x)
print(y)
