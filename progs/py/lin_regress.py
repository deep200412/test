#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
#import scipy
from scipy import stats

#print("nump version = "+ np.version.version)
#print("scipy version = " + scipy.__version__)

x = np.linspace(0,10,11)
y = 2*x*x + 3*x + 5
plt.scatter(x,y)

line_model = stats.linregress(x,y)
print(line_model)
line_fit = line_model.slope * x + line_model.intercept
plt.plot(x, line_fit)
plt.show()
