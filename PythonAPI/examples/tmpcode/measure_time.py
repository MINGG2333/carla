import numpy as np
import pandas as pd
import glob
import os

class VehicleDynamicsAnalyzer:
    def __init__(self, name, data_dir="data"):
        '''name: e.g., vehicle1_css2'''
        # 验证数据目录是否存在
        if not os.path.isdir(data_dir):
            raise FileNotFoundError(f"数据目录 {data_dir} 不存在")

        # 构建文件匹配模式
        pattern = os.path.join(data_dir, f"*_{name}_VehicleDynamics.csv")
        
        # 获取匹配文件列表（按字母排序）
        matched_files = sorted(glob.glob(pattern))
        
        if not matched_files:
            raise FileNotFoundError(f"在 {data_dir} 中未找到符合格式要求的CSV文件")
        
        # 选择第一个匹配文件并记录日志
        self.target_file = matched_files[-1]
        print(f"已选择文件: data/{os.path.basename(self.target_file)}")

        # 读取数据文件
        self.df = pd.read_csv(self.target_file)
        
        # 数据校验
        self._validate_data()

    def _validate_data(self):
        """执行数据完整性检查"""
        required_columns = {'frame_count', 'current_time'}
        missing = required_columns - set(self.df.columns)
        if missing:
            raise ValueError(f"缺失必要列: {missing}")

        if not self.df['frame_count'].is_unique:
            duplicates = self.df[self.df.duplicated('frame_count', keep=False)]
            raise ValueError(f"发现重复frame_count值:\n{duplicates}")

        self.df.set_index('frame_count', inplace=True)

    def get_vehicle_data(self, frame_id, data_field):
        """获取指定帧的车辆数据"""
        if data_field not in self.df.columns:
            available = ', '.join(self.df.columns)
            raise ValueError(f"无效字段 '{data_field}'，可用字段: {available}")

        try:
            return self.df.loc[frame_id, data_field]
        except KeyError:
            valid_frames = self.df.index.unique().tolist()
            raise KeyError(f"无效帧ID {frame_id}，有效范围: {min(valid_frames)}-{max(valid_frames)}")

    def analyze_frame_intervals(self, expected_step=2):
        """分析帧间隔特征"""
        sorted_frames = sorted(self.df.index)
        
        intervals = []
        valid_diffs = []
        
        for i in range(1, len(sorted_frames)):
            prev_frame = sorted_frames[i-1]
            current_frame = sorted_frames[i]
            frame_diff = current_frame - prev_frame
            
            if frame_diff == expected_step:
                try:
                    prev_time = self.df.loc[prev_frame, 'current_time']
                    current_time = self.df.loc[current_frame, 'current_time']
                    time_diff = current_time - prev_time

                    current_pos = (
                        self.df.loc[current_frame, 'location_x'],
                        self.df.loc[current_frame, 'location_y'],
                        self.df.loc[current_frame, 'location_z']
                    )
                    current_yaw = self.df.loc[current_frame, 'rotation_yaw']
                    
                    intervals.append((prev_frame, current_frame, time_diff, current_pos, current_yaw))
                    valid_diffs.append(time_diff)
                except KeyError as e:
                    print(f"数据不完整，缺失帧 {e} 的时间数据")
        
        # 生成统计报告
        stats = {
            'total_pairs': len(valid_diffs),
            'time_stats': {
                'mean': np.mean(valid_diffs) if valid_diffs else 0,
                'max': np.max(valid_diffs) if valid_diffs else 0,
                'min': np.min(valid_diffs) if valid_diffs else 0,
                'std': np.std(valid_diffs) if valid_diffs else 0
            },
            'frame_step': expected_step,
        }
        
        # 打印统计摘要
        print(f"\n帧间隔分析报告（步长={expected_step}）")
        print(f"有效间隔对数量: {stats['total_pairs']}")
        print(f"时间间隔均值: {stats['time_stats']['mean']:.4f}s")
        print(f"最大时间差: {stats['time_stats']['max']:.4f}s")
        print(f"最小时间差: {stats['time_stats']['min']:.4f}s")
        
        return intervals

    def compare_intervals(self, intervals):
        """
        根据给定的intervals列表，计算当前实例中相同prev_frame和current_frame的time_diff
        :param intervals: 来自另一个实例的intervals列表，格式为 [(prev_frame, current_frame, time_diff), ...]
        :return: 包含相同帧对的time_diff列表，格式为 [(prev_frame, current_frame, time_diff), ...]
        """
        compared_intervals = []
        
        for prev_frame, current_frame, time_diff_skip, current_pos_skip, current_yaw_skip in intervals:
            try:
                # 获取当前实例中的时间差
                prev_time = self.df.loc[prev_frame, 'current_time']
                current_time = self.df.loc[current_frame, 'current_time']
                time_diff = current_time - prev_time
                
                current_pos = (
                    self.df.loc[current_frame, 'location_x'],
                    self.df.loc[current_frame, 'location_y'],
                    self.df.loc[current_frame, 'location_z']
                )
                current_yaw = self.df.loc[current_frame, 'rotation_yaw']

                # 记录结果
                compared_intervals.append((prev_frame, current_frame, time_diff, time_diff_skip, time_diff / time_diff_skip,
                                           np.linalg.norm(np.array(current_pos) - np.array(current_pos_skip)), abs(current_yaw-current_yaw_skip)))
            except KeyError:
                # 如果当前实例中缺失帧数据，跳过
                print(f"警告：缺失帧 {prev_frame} 或 {current_frame} 的数据")
                continue
        
        return compared_intervals

if __name__ == "__main__":
    total_vehicles = 1
    skip_time_steps = 2
    try:
        print()
        analyzer = VehicleDynamicsAnalyzer(name=f'vehicle{total_vehicles}_css{skip_time_steps}')  # 自动使用data目录
        
        # # 示例查询
        # frame = 150
        # print(f"帧 {frame} 数据:")
        # print(f"时间戳: {analyzer.get_vehicle_data(frame, 'current_time')}")
        # print(f"X坐标: {analyzer.get_vehicle_data(frame, 'location_x')}")
        # print(f"航向角: {analyzer.get_vehicle_data(frame, 'heading')}")
        print()

        intervals = analyzer.analyze_frame_intervals(expected_step=skip_time_steps)
        
        # # 打印结果
        # print(f"找到 {len(intervals)} 个间隔对")
        # for start_frame, end_frame, time_diff in intervals[:5]:  # 打印前5个结果
        #     print(f"帧 {start_frame} 到 {end_frame}，时间差: {time_diff:.3f} 秒")
        print()

        analyzer_css0 = VehicleDynamicsAnalyzer(name=f'vehicle{total_vehicles}_css{0}')  # 自动使用data目录
        print()

        compared_intervals = analyzer_css0.compare_intervals(intervals)

        # # 打印结果
        # print(f"找到 {len(compared_intervals)} 个匹配的间隔对")
        # for prev_frame, current_frame, time_diff, time_diff_skip, imp in compared_intervals[:5]:  # 打印前5个结果
        #     print(f"帧 {prev_frame} 到 {current_frame}，时间差: {time_diff:.3f} / {time_diff_skip:.3f} 秒, imp: {imp:.3f}")
        print()

    except Exception as e:
        print(f"初始化失败: {str(e)}")

    compared_intervals = np.array(compared_intervals)

    compared_time_diff = compared_intervals[:, 2]
    compared_time_diff_skip = compared_intervals[:, 3]
    compared_imp = compared_intervals[:, 4]
    compared_pos = compared_intervals[:, 5]
    compared_yaw = compared_intervals[:, 6]

    compared_pos = compared_pos[:len(compared_pos)//5]
    compared_yaw = compared_yaw[:len(compared_yaw)//5]
    print(f'{np.mean(compared_time_diff):.3f}')
    print(f'{np.mean(compared_time_diff_skip):.3f}')
    print(f'{np.mean(compared_imp):.3f}')
    print(f'{np.mean(compared_pos):.3f}')
    print(f'{np.mean(compared_yaw[compared_yaw < 180]):.3f}')
    print()