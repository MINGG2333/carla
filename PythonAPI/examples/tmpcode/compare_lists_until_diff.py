import csv

def compare_csv_files(file1, file2, file2_skip_from=0, file2_skip_to=0):
    """
    比较两个 CSV 文件中除第 2 列以外的数据，判断相同的行数。
    
    :param file1: 第一个 CSV 文件路径
    :param file2: 第二个 CSV 文件路径
    :return: 从头开始相同的行数
    """
    try:
        with open(file1, 'r', encoding='utf-8') as f1, open(file2, 'r', encoding='utf-8') as f2:
            reader1 = csv.reader(f1)
            reader2 = csv.reader(f2)
            
            # 初始化计数器和行迭代器
            row_count = 1+0
            for _ in range(row_count):
                next(reader1, None)
                next(reader2, None)
            
            for row_n0, (row1, row2) in enumerate(zip(reader1, reader2)):
                # if row_n0 + 1 == file2_skip_from:
                #     for _ in range(file2_skip_to - file2_skip_from):
                #         row1 = next(reader1, None)
                #         row_count += 1

                # 移除第 2 列（索引 1）后比较
                selected_cols = 5 if 'UWheeledVehicle4W' in file1 else 4
                data1 = row1[:selected_cols] if 'UWheeledVehicle4W' in file1 else row1[:selected_cols] + row1[-4:-3]
                data2 = row2[:selected_cols] if 'UWheeledVehicle4W' in file1 else row2[:selected_cols] + row2[-4:-3]

                while (data1[-1]!=data2[-1]):
                    row1 = next(reader1, None)
                    if row1 == None:
                        break
                    row_count += 1
                    data1 = row1[:selected_cols] if 'UWheeledVehicle4W' in file1 else row1[:selected_cols] + row1[-4:-3]

                if data1 != data2:
                    break
                row_count += 1
            
            return row_count
    except Exception as e:
        print(f"比较文件时出错: {e}")
        return -1

# 示例调用
file1 = "data/20250530_212024_vehicle3_css0_VehicleDynamics.csv"  # 替换为你的第一个 CSV 文件路径
file2 = "data/20250530_212102_vehicle3_css0_VehicleDynamics.csv"  # 替换为你的第二个 CSV 文件路径

same_row_count = compare_csv_files(file1, file2, file2_skip_from=0, file2_skip_to=0)
if same_row_count >= 0:
    print(f"相同的frame no.是: {0} - {same_row_count-2}")
else:
    print("比较过程中发生错误。")


