#!/usr/local/bin/python
import numpy as np
from scipy.optimize import curve_fit

#x = np.linspace(0,5, num=6)
#y = 2*x + 3*x**2 + 10
x = [0, 1, 2, 3, 4 , 5]
y = [5, 10, 21, 38, 61, 90]

def test(x,a,b,c):
    return a*x + b*x**2 + c

param, param_cov = curve_fit(test, x, y)

print("a = %f" %param[0])
print("b = %f" %param[1])
print("c = %f" %param[2])
