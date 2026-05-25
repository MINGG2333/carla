import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 从 CSV 文件中读取数据
# 假设 CSV 文件有两列：X 和 Y
df = pd.read_csv('tmpcode/imp_alone_skip_step.csv')

# 提取 X 和 Y 列
x = df['X']
y = df['Y']

# 获取唯一的 X 值
unique_x = np.unique(x)

# 计算每个 X 值对应的 Y 值的均值和标准差
y_means = []
y_stds = []
for ux in unique_x:
    y_values = y[x == ux]  # 获取当前 X 值对应的所有 Y 值
    y_means.append(np.mean(y_values))  # 计算均值
    y_stds.append(np.std(y_values))   # 计算标准差

# 绘制曲线
plt.plot(unique_x, y_means, label='Mean improvement', color='blue')

# 绘制透明阴影区域（表示 Y 的分布）
plt.fill_between(unique_x, 
                 np.array(y_means) - np.array(y_stds),  # 下边界
                 np.array(y_means) + np.array(y_stds),  # 上边界
                 color='blue', alpha=0.2, label='Distribution')

# 添加标题和标签
# plt.title('Y vs X with Distribution')
plt.xlabel('The Length of CSS', fontdict={'size': 18})
plt.ylabel('Speed Improvement on CSS', fontdict={'size': 20})

# 显示图例
fontdict_prop = {
    # 'family' : 'Times New Roman',
    # 'weight' : 'normal',
    'size'   : 18,
}
plt.legend(prop=fontdict_prop)

# 显示图形
plt.show()
print()