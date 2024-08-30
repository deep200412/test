#/usr/local/bin/python

#Problem:
#Model delay value as a quadratic function of 2 parameters x1 and x2
#and find the value of the these parameters which gives the least prediction error
 
import numpy as np
import pandas as pd
import scipy.optimize as spo

def f(soln, yval, pfunc_2d_array):
   x1 = soln[0]
   x2 = soln[1]
   err = 0.0
   for i in range(len(yval)) :
     pfunc_array = pfunc_2d_array[i]
     pfunc1 = pfunc_array[0]
     pfunc2 = pfunc_array[1]
     y = yval[i]
     err1 = (y - pfunc1(x1) - pfunc2(x2))**2
     err += err1
   cost = err
   return cost


pfunc_2d_array =  []
#initial guess
start = [0,0]
#for slew-1, load-1
x1 = [-1,0,1]
y1 = [0.00081885, 0.0, -0.00078095]
x2 = [-1,0,1]
y2 = [0.00056158, 0.0, -0.00052295]
pfunc1 = np.poly1d(np.polyfit(x1,y1,2))
pfunc2 = np.poly1d(np.polyfit(x2,y2,2))
pfunc_array_1 = []
pfunc_array_1.append(pfunc1)
pfunc_array_1.append(pfunc2)
pfunc_2d_array.append(pfunc_array_1)

#for slew-1, load-4
x1 = [-1.0, 0.0, 1.0] 
y1 = [0.00072615, 0.0, -0.0008964]
x2 = [-1.0, 0.0, 1.0]
y2 = [0.00546738, 0.0, -0.00517479]
pfunc1 = np.poly1d(np.polyfit(x1,y1,2))
pfunc2 = np.poly1d(np.polyfit(x2,y2,2))
pfunc_array_2 = []
pfunc_array_2.append(pfunc1)
pfunc_array_2.append(pfunc2)
pfunc_2d_array.append(pfunc_array_2)

#for slew-4, load-1
x1 = [-1.0, 0.0, 1.0]
y1 = [0.01063634, 0, -0.01029697]
x2 = [-1.0, 0.0, 1.0]
y2 = [0.00071916 ,0 , -0.00045198]
pfunc1 = np.poly1d(np.polyfit(x1,y1,2))
pfunc2 = np.poly1d(np.polyfit(x2,y2,2))
pfunc_array_3 = []
pfunc_array_3.append(pfunc1)
pfunc_array_3.append(pfunc2)
pfunc_2d_array.append(pfunc_array_3)

#for slew-4, load-4
x1 = [-1.0, 0.0, 1.0]
y1 = [.01073216, 0, -0.01058420]
x2 = [-1.0, 0.0, 1.0]
y2 = [.00543647 ,0, -0.00485300]
pfunc1 = np.poly1d(np.polyfit(x1,y1,2))
pfunc2 = np.poly1d(np.polyfit(x2,y2,2))
pfunc_array_4 = []
pfunc_array_4.append(pfunc1)
pfunc_array_4.append(pfunc2)
pfunc_2d_array.append(pfunc_array_4)


yval = [-0.00054,-0.003917,-0.0097649,-0.01312599]

#Optimization
result = spo.minimize(f, start, args = (yval, pfunc_2d_array), options = {'disp':True})


if result.success:
    x = result.x
    print("x = " + str(x))
else:
    print("Error: Unable to find max area")

#prediction
x1 = x[0]
x2 = x[1]

for i in range(len(yval)):
  pfunc1 = pfunc_2d_array[i][0]
  pfunc2 = pfunc_2d_array[i][1]
  yval_pred = pfunc1(x1) + pfunc2(x2)
  print("Predicted yval%d = %f" %(i, yval_pred))

