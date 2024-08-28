#/usr/local/bin/python
import numpy as np
import pandas as pd
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression
#import matplotlib
#import matplotlib.pyplot as plt

#my_df = {
#  "p1": [10,30,40],
#  "p2": [1,1.5,2.3],
#  "value": [70,80,90]
#}
my_df = pd.read_csv('data.csv')

X = my_df[['p1','p2']].values
y = my_df['value'].values

poly = PolynomialFeatures(degree=2)
X_poly = poly.fit_transform(X)

model = LinearRegression()
model.fit(X_poly,y)

new_data = np.array([[20,1.4]])
new_data_poly = poly.transform(new_data)

predicted_value = model.predict(new_data_poly)
print("Predicted value :")
print(predicted_value)
