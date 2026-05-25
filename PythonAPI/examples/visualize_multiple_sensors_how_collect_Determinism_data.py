#!/usr/bin/env python

# Copyright (c) 2020 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

"""
Script that render multiple sensors in the same pygame window

By default, it renders four cameras, one LiDAR and one Semantic LiDAR.
It can easily be configure for any different number of sensors. 
To do that, check lines 290-308.
"""

import csv
from datetime import datetime
import glob
import os
import sys
import traceback

# try:
#     sys.path.append(glob.glob('/xishu/ZHITAI/sda/carla_apollo_bridge/carla_bridge/carla_api/carla-*%d.%d-%s.egg' % (
#         sys.version_info.major,
#         sys.version_info.minor,
#         'win-amd64' if os.name == 'nt' else 'linux-x86_64'))[0])
# except IndexError:
#     pass

import carla
import argparse
import random
import time
import numpy as np


random.seed( 10 )

try:
    import pygame
    from pygame.locals import K_ESCAPE
    from pygame.locals import K_q
except ImportError:
    raise RuntimeError('cannot import pygame, make sure pygame package is installed')

class UtilityTimer:
    def __init__(self):
        self.tick_time = 0
        self.tock_time = 0

    def log_time_now(self):
        # 获取当前时间
        now = datetime.now()
        # 格式化为指定格式
        formatted_time = now.strftime("%Y.%m.%d-%H.%M.%S:%f")[:-3]  # 去掉毫秒的后三位
        return formatted_time

    def unix_time_now(self):
        # 获取当前 Unix 时间戳并转换为毫秒
        return int(time.time() * 1000)  # time.time() 返回秒，乘以 1000 变为毫秒

    def tick(self):
        # 记录开始时间
        self.tick_time = self.unix_time_now()

    def tock(self):
        # 记录结束时间并计算差值
        self.tock_time = self.unix_time_now()
        return self.tock_time - self.tick_time

class VehicleDynamicsSaver:
    def __init__(self, output_file) -> None:
        self.frames = []
        self.frame_count = 0
        self.output_file = '/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data/Determinism/' + datetime.now().strftime("%Y%m%d_%H%M%S_") + output_file + '.csv'

    def record_data(self, data):
        data['current_time'] = round(time.time(), 4)
        data['FrameCount'] = self.frame_count
        self.frames.append(data)
        self.frame_count += 1

    def __del__(self, ):
        if not self.frames:
            print("没有数据需要保存。")
            return

        # 获取 CSV 文件的列名（假设所有帧有相同的键）
        fieldnames = list(self.frames[0].keys())

        # 写入 CSV 文件
        try:
            with open(self.output_file, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                writer.writeheader()  # 写入列名
                writer.writerows(self.frames)  # 写入所有帧的数据
            print(f"数据已成功保存到 {self.output_file}")
        except Exception as e:
            traceback.print_exc()
            print(f"保存文件时出错: {e}")



class CustomTimer:
    def __init__(self):
        try:
            self.timer = time.perf_counter
        except AttributeError:
            self.timer = time.time

    def time(self):
        return self.timer()

class DisplayManager:
    def __init__(self, grid_size, window_size):
        pygame.init()
        pygame.font.init()
        self.display = pygame.display.set_mode(window_size, pygame.HWSURFACE | pygame.DOUBLEBUF)

        self.grid_size = grid_size
        self.window_size = window_size
        self.sensor_list = []

    def get_window_size(self):
        return [int(self.window_size[0]), int(self.window_size[1])]

    def get_display_size(self):
        return [int(self.window_size[0]/self.grid_size[1]), int(self.window_size[1]/self.grid_size[0])]

    def get_display_offset(self, gridPos):
        dis_size = self.get_display_size()
        return [int(gridPos[1] * dis_size[0]), int(gridPos[0] * dis_size[1])]

    def add_sensor(self, sensor):
        self.sensor_list.append(sensor)

    def get_sensor_list(self):
        return self.sensor_list

    def render(self):
        if not self.render_enabled():
            return

        for s in self.sensor_list:
            s.render()

        pygame.display.flip()

    def destroy(self):
        for s in self.sensor_list:
            s.destroy()

    def render_enabled(self):
        return self.display != None

class SensorManager:
    def __init__(self, world, display_man, sensor_type, transform, attached, sensor_options, display_pos):
        self.surface = None
        self.world = world
        self.display_man = display_man
        self.display_pos = display_pos
        self.sensor = self.init_sensor(sensor_type, transform, attached, sensor_options)
        self.sensor_options = sensor_options
        self.timer = CustomTimer()

        self.time_processing = 0.0
        self.tics_processing = 0

        self.display_man.add_sensor(self)

    def init_sensor(self, sensor_type, transform, attached, sensor_options):
        if sensor_type == 'RGBCamera':
            camera_bp = self.world.get_blueprint_library().find('sensor.camera.rgb')
            disp_size = self.display_man.get_display_size()
            camera_bp.set_attribute('image_size_x', str(disp_size[0]))
            camera_bp.set_attribute('image_size_y', str(disp_size[1]))

            for key in sensor_options:
                camera_bp.set_attribute(key, sensor_options[key])

            camera = self.world.spawn_actor(camera_bp, transform, attach_to=attached)
            camera.listen(self.save_rgb_image)

            return camera

        elif sensor_type == 'LiDAR':
            lidar_bp = self.world.get_blueprint_library().find('sensor.lidar.ray_cast')
            lidar_bp.set_attribute('range', '100')
            lidar_bp.set_attribute('dropoff_general_rate', lidar_bp.get_attribute('dropoff_general_rate').recommended_values[0])
            lidar_bp.set_attribute('dropoff_intensity_limit', lidar_bp.get_attribute('dropoff_intensity_limit').recommended_values[0])
            lidar_bp.set_attribute('dropoff_zero_intensity', lidar_bp.get_attribute('dropoff_zero_intensity').recommended_values[0])

            for key in sensor_options:
                lidar_bp.set_attribute(key, sensor_options[key])

            lidar = self.world.spawn_actor(lidar_bp, transform, attach_to=attached)

            lidar.listen(self.save_lidar_image)

            return lidar
        
        elif sensor_type == 'SemanticLiDAR':
            lidar_bp = self.world.get_blueprint_library().find('sensor.lidar.ray_cast_semantic')
            lidar_bp.set_attribute('range', '100')

            for key in sensor_options:
                lidar_bp.set_attribute(key, sensor_options[key])

            lidar = self.world.spawn_actor(lidar_bp, transform, attach_to=attached)

            lidar.listen(self.save_semanticlidar_image)

            return lidar
        
        elif sensor_type == "Radar":
            radar_bp = self.world.get_blueprint_library().find('sensor.other.radar')
            for key in sensor_options:
                radar_bp.set_attribute(key, sensor_options[key])

            radar = self.world.spawn_actor(radar_bp, transform, attach_to=attached)
            radar.listen(self.save_radar_image)

            return radar
        
        else:
            return None

    def get_sensor(self):
        return self.sensor

    def save_rgb_image(self, image):
        t_start = self.timer.time()

        image.convert(carla.ColorConverter.Raw)
        array = np.frombuffer(image.raw_data, dtype=np.dtype("uint8"))
        array = np.reshape(array, (image.height, image.width, 4))
        array = array[:, :, :3]
        array = array[:, :, ::-1]

        if self.display_man.render_enabled():
            self.surface = pygame.surfarray.make_surface(array.swapaxes(0, 1))

        t_end = self.timer.time()
        self.time_processing += (t_end-t_start)
        self.tics_processing += 1

    def save_lidar_image(self, image):
        t_start = self.timer.time()

        disp_size = self.display_man.get_display_size()
        lidar_range = 2.0*float(self.sensor_options['range'])

        points = np.frombuffer(image.raw_data, dtype=np.dtype('f4'))
        points = np.reshape(points, (int(points.shape[0] / 4), 4))
        lidar_data = np.array(points[:, :2])
        lidar_data *= min(disp_size) / lidar_range
        lidar_data += (0.5 * disp_size[0], 0.5 * disp_size[1])
        lidar_data = np.fabs(lidar_data)  # pylint: disable=E1111
        lidar_data = lidar_data.astype(np.int32)
        lidar_data = np.reshape(lidar_data, (-1, 2))
        lidar_img_size = (disp_size[0], disp_size[1], 3)
        lidar_img = np.zeros((lidar_img_size), dtype=np.uint8)

        lidar_img[tuple(lidar_data.T)] = (255, 255, 255)

        if self.display_man.render_enabled():
            self.surface = pygame.surfarray.make_surface(lidar_img)

        t_end = self.timer.time()
        self.time_processing += (t_end-t_start)
        self.tics_processing += 1

    def save_semanticlidar_image(self, image):
        t_start = self.timer.time()

        disp_size = self.display_man.get_display_size()
        lidar_range = 2.0*float(self.sensor_options['range'])

        points = np.frombuffer(image.raw_data, dtype=np.dtype('f4'))
        points = np.reshape(points, (int(points.shape[0] / 6), 6))
        lidar_data = np.array(points[:, :2])
        lidar_data *= min(disp_size) / lidar_range
        lidar_data += (0.5 * disp_size[0], 0.5 * disp_size[1])
        lidar_data = np.fabs(lidar_data)  # pylint: disable=E1111
        lidar_data = lidar_data.astype(np.int32)
        lidar_data = np.reshape(lidar_data, (-1, 2))
        lidar_img_size = (disp_size[0], disp_size[1], 3)
        lidar_img = np.zeros((lidar_img_size), dtype=np.uint8)

        lidar_img[tuple(lidar_data.T)] = (255, 255, 255)

        if self.display_man.render_enabled():
            self.surface = pygame.surfarray.make_surface(lidar_img)

        t_end = self.timer.time()
        self.time_processing += (t_end-t_start)
        self.tics_processing += 1

    def save_radar_image(self, radar_data):
        t_start = self.timer.time()
        points = np.frombuffer(radar_data.raw_data, dtype=np.dtype('f4'))
        points = np.reshape(points, (len(radar_data), 4))

        t_end = self.timer.time()
        self.time_processing += (t_end-t_start)
        self.tics_processing += 1

    def render(self):
        if self.surface is not None:
            offset = self.display_man.get_display_offset(self.display_pos)
            self.display_man.display.blit(self.surface, offset)

    def destroy(self):
        self.sensor.destroy()

def run_simulation(args, client):
    """This function performed one test run using the args parameters
    and connecting to the carla client passed.
    """

    display_manager = None
    vehicle = None
    vehicle_list = []
    timer = CustomTimer()
    Utimer = UtilityTimer()
    cnt_tick = 0

    try:

        # Getting the world and
        world = client.get_world()
        original_settings = world.get_settings()

        if args.sync:
            traffic_manager = client.get_trafficmanager(8000)
            settings = world.get_settings()
            traffic_manager.set_synchronous_mode(True)
            settings.synchronous_mode = True
            settings.fixed_delta_seconds = 0.05
            world.apply_settings(settings)

# after tick 0,1,2
# set tick number = 0

        # Instanciating the vehicle to which we attached the sensors
        bp = world.get_blueprint_library().filter('charger_2020')[0]
        vehicle = world.spawn_actor(bp, world.get_map().get_spawn_points()[-1])
        vehicle_list.append(vehicle)
        vehicle.set_autopilot(True)

        spectator = world.get_spectator()
        ego_trans = vehicle.get_transform()
        # x
        # ------------y
        spectator.set_transform(carla.Transform(ego_trans.location + carla.Location(x=50, y=-50, z=250), carla.Rotation(pitch=-90))) # yaw=-90: make x to right ego_trans.rotation.yaw wave sharply

        # Display Manager organize all the sensors an its display in a window
        # If can easily configure the grid and the total window size
        display_manager = DisplayManager(grid_size=[2, 3], window_size=[args.width, args.height])

        # Then, SensorManager can be used to spawn RGBCamera, LiDARs and SemanticLiDARs as needed
        # and assign each of them to a grid position, 
        c = SensorManager(world, display_manager, 'RGBCamera', carla.Transform(carla.Location(x=0, z=2.4), carla.Rotation(yaw=-90)), 
                      vehicle, {}, display_pos=[0, 0])
        c1 = SensorManager(world, display_manager, 'RGBCamera', carla.Transform(carla.Location(x=0, z=2.4), carla.Rotation(yaw=+00)), 
                      vehicle, {}, display_pos=[0, 1])
        c2 = SensorManager(world, display_manager, 'RGBCamera', carla.Transform(carla.Location(x=0, z=2.4), carla.Rotation(yaw=+90)), 
                      vehicle, {}, display_pos=[0, 2])
        c3 = SensorManager(world, display_manager, 'RGBCamera', carla.Transform(carla.Location(x=0, z=2.4), carla.Rotation(yaw=180)), 
                      vehicle, {}, display_pos=[1, 1])

        l = SensorManager(world, display_manager, 'LiDAR', carla.Transform(carla.Location(x=0, z=2.4)), 
                      vehicle, {'channels' : '64', 'range' : '100',  'points_per_second': '250000', 'rotation_frequency': '20'}, display_pos=[1, 0])
        sl = SensorManager(world, display_manager, 'SemanticLiDAR', carla.Transform(carla.Location(x=0, z=2.4)), 
                      vehicle, {'channels' : '64', 'range' : '100', 'points_per_second': '100000', 'rotation_frequency': '20'}, display_pos=[1, 2])


        #Simulation loop
        call_exit = False
        time_init_sim = timer.time()
        end_tick = 300
        skip_num = 0
        skip_step = 0
        VEHICLE_CONFIG_PATH = 'config/myTotalVehicles.txt'
        NUM_VEHICLES = read_vehicle_count(VEHICLE_CONFIG_PATH)
        print(f"本次仿真将生成 {NUM_VEHICLES+1} 辆车辆 (NPCs+EGO)")
        vehicle_list.extend(generate_vehicles(world, NUM_VEHICLES))
        mysaver = VehicleDynamicsSaver(f'vehicle{NUM_VEHICLES+1}_css{skip_step}_VehicleDynamics')
        while True:
            print(f'\n\n[{Utimer.log_time_now()}][{cnt_tick} -> {cnt_tick+1}]')

            # debug
            # if cnt_tick <= 1:
            #     write_integers_to_file('config/myAB.txt', 1, 3)
            # elif cnt_tick <= 4:
            #     write_integers_to_file('config/myAB.txt', 4, 12)
            # debug end
            # if cnt_tick == 1 + (skip_step + 1) * skip_num:
            #     # myA >= cnt_tick
            #     write_integers_to_file('config/myAB.txt', cnt_tick, cnt_tick + skip_step)
            #     skip_num += 1

            # skip from tick int1 to tick int2
            a, b = read_integers_from_file('config/myAB.txt')
            assert a is not None
            assert b is not None
            if cnt_tick == a:
                print(f'\n[{Utimer.log_time_now()}][{cnt_tick} -> {b}]')
                cnt_tick = b - 1

                if b > end_tick:
                    break

            # Tick
            Utimer.tick()  # 开始计时
            # Carla Tick
            if args.sync:
                world.tick()
            else:
                world.wait_for_tick()
            elapsed_time = Utimer.tock()  # 结束计时
            print("Elapsed time in milliseconds:", elapsed_time)

            cnt_tick += 1

            # my recording
            ego_trans = vehicle.get_transform()
            # vel = vehicle.get_velocity()
            ctrl = vehicle.get_control()
            # physc = vehicle.get_physics_control()
            vehicle_data = {
                "location_x": ego_trans.location.x,
                "location_y": ego_trans.location.y,
                "location_z": ego_trans.location.z,
                "rotation_yaw": ego_trans.rotation.yaw,
                "heading": get_heading(ego_trans.rotation.yaw),

                # "velocity_x": vel.x,
                # "velocity_y": vel.y,
                # "velocity_z": vel.z,

                # !!! not FReplicatedVehicleState, that is, do not change when snapshot
                # "control_brake": ctrl.brake,
                # # "control_gear": ctrl.gear,
                # "control_hand_brake": ctrl.hand_brake,
                # # "control_manual_gear_shift": ctrl.manual_gear_shift,
                # # "control_reverse": ctrl.reverse,
                # "control_steer": ctrl.steer,
                # "control_throttle": ctrl.throttle,

                # "physics_clutch_strength": physc.clutch_strength,
                # "physics_damping_rate_full_throttle": physc.damping_rate_full_throttle,
                # "physics_damping_rate_zero_throttle_clutch_disengaged": physc.damping_rate_zero_throttle_clutch_disengaged,
                # "physics_damping_rate_zero_throttle_clutch_engaged": physc.damping_rate_zero_throttle_clutch_engaged,
                # "physics_drag_coefficient": physc.drag_coefficient,
                # "physics_final_ratio": physc.final_ratio,
                # "physics_gear_switch_time": physc.gear_switch_time,
                # "physics_mass": physc.mass,
                # "physics_max_rpm": physc.max_rpm,
                # "physics_moi": physc.moi,
                # "physics_use_gear_autobox": physc.use_gear_autobox,
                # "physics_use_sweep_wheel_collision": physc.use_sweep_wheel_collision,
            }
            for record_name in vehicle_data:
                if type(vehicle_data[record_name]) == float:
                    vehicle_data[record_name] = round(vehicle_data[record_name], 4)
            vehicle_data['frame_count'] = cnt_tick
            vehicle_data['tick_time'] = elapsed_time
            mysaver.record_data(vehicle_data)
            # my recording end

            # Render received data
            display_manager.render()

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    call_exit = True
                elif event.type == pygame.KEYDOWN:
                    if event.key == K_ESCAPE or event.key == K_q:
                        call_exit = True
                        break

            if call_exit:
                break

            if cnt_tick > end_tick:
                break

    except Exception as e:
        print('Error!')
        traceback.print_exc()
        print('')

    finally:
        mysaver = None
        del mysaver
        if display_manager:
            display_manager.destroy()

        client.apply_batch([carla.command.DestroyActor(x) for x in vehicle_list])

        original_settings.fixed_delta_seconds = 0.1
        world.apply_settings(original_settings)

        write_integers_to_file('config/myAB.txt', 1000, 12000)

def get_heading(yaw):
    heading = 'N' if abs(yaw) < 89.5 else ''
    heading += 'S' if abs(yaw) > 90.5 else ''
    heading += 'E' if 179.5 > yaw > 0.5 else ''
    heading += 'W' if -0.5 > yaw > -179.5 else ''
    return heading

def write_integers_to_file(file_path, int1, int2):
    # 打开文件并写入整数
    with open(file_path, 'w') as file:
        file.write(f"{int1}\n{int2}\n")
    print(f"整数{int1} {int2} 已写入文件 {file_path}")

def read_integers_from_file(file_path):
    try:
        with open(file_path, 'r') as file:
            # 读取文件中的所有行
            lines = file.readlines()
            # 将每行转换为整数
            int1 = int(lines[0].strip())
            int2 = int(lines[1].strip())
            return int1, int2
    except FileNotFoundError:
        print(f"文件 {file_path} 未找到")
    except ValueError:
        print("文件内容格式错误，无法转换为整数")
    except Exception as e:
        print(f"读取文件时发生错误: {e}")
    return None, None

def read_vehicle_count(file_path):
    """从指定文件读取车辆数量"""
    default_vehicles = 5  # 默认车辆数
    try:
        with open(file_path, 'r') as f:
            content = f.read().strip()
            return max(0, int(content))  # 确保至少0辆NPCs
    except FileNotFoundError:
        print(f"警告：配置文件 {file_path} 未找到，使用默认值 {default_vehicles}")
    except ValueError:
        print(f"警告：文件内容格式错误，使用默认值 {default_vehicles}")
    except Exception as e:
        print(f"读取配置文件出错: {str(e)}，使用默认值 {default_vehicles}")
    return default_vehicles

def generate_vehicles(world, NUM_VEHICLES):
    # 替换原有单个车辆生成代码
    vehicle_list = []
    spawn_points = world.get_map().get_spawn_points()

    # 添加生成点数量检查
    if NUM_VEHICLES > len(spawn_points):
        print(f"警告：需要生成 {NUM_VEHICLES} 辆车但地图只有 {len(spawn_points)} 个生成点")
        print("将尝试重复使用生成点并添加随机偏移")

    for i in range(1, NUM_VEHICLES+1):
        # 选择随机车型
        bp = random.choice(world.get_blueprint_library().filter('vehicle'))
        
        # 设置车辆属性
        if bp.has_attribute('color'):
            color = random.choice(bp.get_attribute('color').recommended_values)
            bp.set_attribute('color', color)
        
        # 获取生成点
        spawn_point = spawn_points[i % len(spawn_points)]
    
        # 如果生成点被重复使用，添加随机偏移
        if i >= len(spawn_points):
            spawn_point.location += carla.Location(
                x=random.uniform(-3, 3),
                y=random.uniform(-3, 3),
                z=0.2
            )
        
        # 生成车辆
        vehicle = world.spawn_actor(bp, spawn_point)
        vehicle.set_autopilot(True)
        vehicle_list.append(vehicle)

    return vehicle_list

def main():
    argparser = argparse.ArgumentParser(
        description='CARLA Sensor tutorial')
    argparser.add_argument(
        '--host',
        metavar='H',
        default='127.0.0.1',
        help='IP of the host server (default: 127.0.0.1)')
    argparser.add_argument(
        '-p', '--port',
        metavar='P',
        default=2000,
        type=int,
        help='TCP port to listen to (default: 2000)')
    argparser.add_argument(
        '--sync',
        action='store_true',
        help='Synchronous mode execution')
    argparser.add_argument(
        '--async',
        dest='sync',
        action='store_false',
        help='Asynchronous mode execution')
    argparser.set_defaults(sync=True)
    argparser.add_argument(
        '--res',
        metavar='WIDTHxHEIGHT',
        default='1280x720',
        help='window resolution (default: 1280x720)')

    args = argparser.parse_args()

    args.width, args.height = [int(x) for x in args.res.split('x')]

    try:
        client = carla.Client(args.host, args.port)
        client.set_timeout(12*60*60.0)

        run_simulation(args, client)

    except KeyboardInterrupt:
        print('\nCancelled by user. Bye!')


if __name__ == '__main__':

    main()
