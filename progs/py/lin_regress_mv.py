#!/usr/bin/python3
#linear regression using multiple features

import numpy as np
import pandas as pd
from sklearn import linear_model

my_df = pd.read_csv('data.csv')
print(my_df)
#print(my_df[['area','bedroom','age']].values)
#print(my_df.price)

reg = linear_model.LinearRegression()
reg.fit(my_df[['area','bedroom','age']].values,my_df.price.values)

print("Coeff:")
print(reg.coef_)
print("Intercept:")
print(reg.intercept_)
res = reg.predict([[3500,2,20]])
print("Predicted price: ")
print(res)
