import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 假设你有一个包含CSV文件路径的列表
csv_files = [
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_152138_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_152704_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_152858_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_153003_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_174140_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_174257_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_174928_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_175130_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_175319_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_175458_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_175803_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_180003_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_180205_VehicleDynamics.csv',
    '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_180518_VehicleDynamics.csv'

]

# 创建一个空的DataFrame来存储统计信息
stats_table = pd.DataFrame(columns=['File', 'Mean', 'Variance', 'Min', 'Max'])

# 创建一个图形
plt.figure(figsize=(10, 6))

# 定义颜色和透明度
alpha = 0.5  # 设置透明度

# 遍历每个CSV文件
for i, file in enumerate(csv_files):
    # 读取CSV文件
    df = pd.read_csv(file)
    
    # 提取ticktime列的数据
    ticktime_data = df['tick_time']
    
    # 计算统计量
    mean = np.mean(ticktime_data)  # 均值
    variance = np.var(ticktime_data)  # 方差
    min_value = np.min(ticktime_data)  # 最小值
    max_value = np.max(ticktime_data)  # 最大值
    # 将统计信息添加到表格中
    stats_table.loc[i] = [file, mean, variance, min_value, max_value]
    
    # 打印统计量
    print(f"File {i+1} ({file}):")
    print(f"  Mean: {mean:.2f}")
    print(f"  Variance: {variance:.2f}")
    print(f"  Min: {min_value:.2f}")
    print(f"  Max: {max_value:.2f}")
    print("-" * 30)
    
    # 绘制数据分布图
    plt.hist(ticktime_data, bins=30, alpha=alpha, label=f'File {i+1}')

# 打印统计表格
print("Statistics Table:")
print(stats_table)

# 添加图例
plt.legend()

# 添加标题和标签
plt.title('Ticktime Distribution Comparison')
plt.xlabel('Ticktime')
plt.ylabel('Frequency')

# 显示图形
plt.show()
print()