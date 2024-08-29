#/usr/local/bin/python

#Problem:
#Given a rock wall and rectangular area to fence,
#find the max area that can be fenced with total wire length = 100
#i.e 2x + y = 100 where y is the length of rock wall

import math
import numpy as np
import pandas as pd
import scipy.optimize as spo

def f(xy):
    l = xy[0]
    b = xy[1]
    area = l*b
    return -area

#constraints
cons = ({'type':'eq', 'fun': lambda xy: (2*xy[0] + xy[1] - 100)})

#bounds
bnds = ( (1,100), (1,100))

#initial guess
xy_start = [10,90]

#Optimization
result = spo.minimize(f, xy_start, options = {'disp':True},
                      constraints = cons,
                      bounds = bnds)

if result.success:
    xy = result.x
    l = xy[0]
    b = xy[1]
    area = l*b
    print("l = %f, b = %f, max area = %f" %(l,b,area))
else:
    print("Error: Unable to find max area")

