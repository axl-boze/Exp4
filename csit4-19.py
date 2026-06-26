import numpy as np
import pandas as pd
from sklearn.linear_model import LinearRegression

d_train = pd.read_csv('./data/train.csv')
d_test = pd.read_csv('./data/test.csv')

n_train = len(d_train)

# targetの値
y_train = d_train.pop('popularity').to_numpy()

columns_num = [
    "duration_ms", "danceability", "energy", "key", "loudness", "mode",
    "speechiness", "acousticness", "instrumentalness", "liveness",
    "valence", "tempo", "time_signature", "explicit"
]

# explicitの列の型をintにキャスト
d_train = d_train.astype({'explicit': int})
d_test = d_test.astype({'explicit': int})

X_train_num = d_train[columns_num].to_numpy()
X_test_num = d_test[columns_num].to_numpy()


# 数値特徴量のみ：バイアスあり
lr = LinearRegression()
lr.fit(X_train_num, y_train)
y_pred_test_lr = lr.predict(X_test_num)

print(y_pred_test_lr)
np.savetxt('./data/y_pred_lr.csv', y_pred_test_lr, delimiter=',')

# 数値特徴量のみ：バイアスなし

lr_without_bias = LinearRegression(fit_intercept=False)
lr_without_bias.fit(X_train_num, y_train)
y_pred_test_lr_without_bias = lr_without_bias.predict(X_test_num)

print(y_pred_test_lr_without_bias)
np.savetxt('./data/y_pred_lr_without_bias.csv', y_pred_test_lr_without_bias, delimiter=',')


# track_genreをone-hotエンコーディング

d_all = pd.concat([d_train, d_test], axis=0)

columns_cat = ["track_genre"]

d_all_onehot = pd.get_dummies(d_all, columns=columns_cat, dtype=int)

d_train_onehot = d_all_onehot.iloc[:n_train]
d_test_onehot = d_all_onehot.iloc[n_train:]

X_train_onehot = d_train_onehot.select_dtypes(include=['number']).to_numpy()
X_test_onehot = d_test_onehot.select_dtypes(include=['number']).to_numpy()

print(d_train_onehot)

# one-hotあり：バイアスあり
lr = LinearRegression()
lr.fit(X_train_onehot, y_train)
y_pred_test_lr_onehot = lr.predict(X_test_onehot)

print(y_pred_test_lr_onehot)
np.savetxt('./data/y_pred_lr_onehot.csv', y_pred_test_lr_onehot, delimiter=',')

# one-hotあり：バイアスなし
lr_without_bias = LinearRegression(fit_intercept=False)
lr_without_bias.fit(X_train_onehot, y_train)
y_pred_test_lr_onehot_without_bias = lr_without_bias.predict(X_test_onehot)

print(y_pred_test_lr_onehot_without_bias)
np.savetxt('./data/y_pred_lr_onehot_without_bias.csv', y_pred_test_lr_onehot_without_bias, delimiter=',')