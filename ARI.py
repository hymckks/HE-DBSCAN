from sklearn import metrics

labels_true = [0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1]
labels_pred = [0, 0, 0, 0, 0, 0, 0, 0, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1, 2, 2]    #0.444444

score = metrics.adjusted_rand_score(labels_true, labels_pred)
print(score)