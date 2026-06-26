import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn.linear_model import LinearRegression

d_train = pd.read_csv('./data/train.csv')
d_test = pd.read_csv('./data/test.csv')

n_train = len(d_train)
n_test = len(d_test)
# targetの値
y_train = d_train.pop('popularity')
y_train = y_train.to_numpy()

columns_num = ["duration_ms", "danceability", "energy", "key", "loudness", "mode","speechiness","acousticness","instrumentalness","liveness","valence","tempo","time_signature","explicit"]

d_train = d_train.astype({'explicit': int}) # `explicit`の列の型をintにキャスト
d_test = d_test.astype({'explicit': int}) # `explicit`の列の型をintにキャスト

X_train_num = d_train[columns_num].to_numpy()
X_test_num = d_test[columns_num].to_numpy()

# 手順1：LinearRegressionのインスタンスの作成
lr = LinearRegression()
# 手順2：上で作ったオブジェクトの学習
lr.fit(X_train_num, y_train)
# 手順3：テストデータに対する予測
y_pred_test = lr.predict(X_test_num)

print(y_pred_test)

np.savetxt(X=y_pred_test, fname='./data/y_pred_lr.csv')

lr = LinearRegression(fit_intercept=False)
lr.fit(X_train_num, y_train)
# 手順1：LinearRegressionのインスタンスの作成．fit_intercept=Falseとすることで，バイアス項を使わない
lr_without_bias = LinearRegression(fit_intercept=False)
# 手順2：上で作ったオブジェクトの学習
lr_without_bias.fit(X_train_num, y_train)
# 手順3：テストデータに対する予測
y_pred_test_lr_without_bias = lr_without_bias.predict(X_test_num)
print(y_pred_test_lr_without_bias)

d_all = pd.concat([d_train, d_test], axis=0) # 訓練とテストを連結
columns_cat = ["track_genre"] # カテゴリカル変数の列名．ここでは"track_genre"のみを指定

d_all_onehot = pd.get_dummies(d_all, columns=columns_cat, dtype=int) # get_dummiesを使ってone-hotエンコーディング．columnsに指定された列のみone-hotエンコーディングし，出力としてint型を指定
d_train_onehot = d_all_onehot[:n_train] # d_all_onehotの訓練データ部分
d_test_onehot = d_all_onehot[n_train:] # d_all_onehotのテストデータ部分
X_train_onehot = d_train_onehot.select_dtypes(include=['int64', 'float64']).to_numpy() # 数値部分のみ取り出し，np.arrayに変換
X_test_onehot = d_test_onehot.select_dtypes(include=['int64', 'float64']).to_numpy()  # 数値部分のみ取り出し，np.arrayに変換

print(d_train_onehot)

lr.fit(X_train_onehot, y_train) # 学習
y_pred_test_lr_onehot = lr.predict(X_test_onehot)
print(y_pred_test_lr_onehot)

lr_without_bias.fit(X_train_onehot, y_train) # 学習
y_pred_test_lr_onehot_without_bias = lr_without_bias.predict(X_test_onehot)
print(y_pred_test_lr_onehot_without_bias)