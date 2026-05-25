import csv
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def all_rows_equal(arr):
    # 比较每一行是否与第一行相同
    return np.all(np.equal(arr, arr[0]))

def calc_tick_time_dict(csv_files):

    ticktime_data_list = []
    frame_count_data_list = []
    # 遍历每个CSV文件
    for i, file in enumerate(csv_files):
        # 读取CSV文件
        df = pd.read_csv(file)
        
        # 提取ticktime列的数据
        ticktime_data = df['tick_time'].values
        frame_count_data = df['frame_count'].values

        ticktime_data_list.append(ticktime_data)
        frame_count_data_list.append(frame_count_data)


    ticktime_data_list = np.array(ticktime_data_list)
    frame_count_data_list = np.array(frame_count_data_list)

    assert all_rows_equal(frame_count_data_list) == True

    ticktime_data_average = np.average(ticktime_data_list, axis=0)
    tick_time_dict = dict(zip(frame_count_data_list[0], ticktime_data_average))

    return tick_time_dict

def calc_imp_list(skip_step, tick_time_dict_no_skip, tick_time_dict_skip_30):

    imp_list = []
    sample_num = 1
    index_restore = sample_num * (skip_step + 1)
    while (index_restore in tick_time_dict_no_skip):
        assert index_restore in tick_time_dict_skip_30
        cur_speed = skip_step / tick_time_dict_skip_30[index_restore]
        org_speed = skip_step / sum([tick_time_dict_no_skip[ii] for ii in range(index_restore - skip_step + 1, index_restore + 1)])
        imp = (cur_speed - org_speed) / org_speed
        imp_list.append(imp)
        sample_num += 1
        index_restore = sample_num * (skip_step + 1)

    return imp_list

if __name__ == '__main__':

    csv_files_no_skip = [
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
    tick_time_dict_no_skip = calc_tick_time_dict(csv_files_no_skip)

    skip_step = 2
    csv_files_skip_30 = [
        '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_174118_VehicleDynamics.csv',
        '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/20250105_174225_VehicleDynamics.csv'
    ]
    imp_list_skip_30 = []
    for csv_file_skip_30 in csv_files_skip_30:
        tick_time_dict_skip_30 = calc_tick_time_dict([csv_file_skip_30])
        imp_list_skip_30 += calc_imp_list(skip_step, tick_time_dict_no_skip, tick_time_dict_skip_30)

    coordinates = [(skip_step, imp_) for imp_ in imp_list_skip_30]
    csv_file = 'tmpcode/imp_alone_skip_step.csv'
    with open(csv_file, mode='a+', newline='') as file:
        writer = csv.writer(file)
        # writer.writerow(['X', 'Y'])

        writer.writerows(coordinates)

    print()