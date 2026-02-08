#!/usr/bin/env python

import glob
import os
import sys

try:
    sys.path.append(glob.glob('../carla/dist/carla-*%d.%d-%s.egg' % (
        sys.version_info.major,
        sys.version_info.minor,
        'win-amd64' if os.name == 'nt' else 'linux-x86_64'))[0])
except IndexError:
    pass

import carla
import argparse
import random
import time
import numpy as np
import pygame
from pygame.locals import K_ESCAPE, K_q, K_SPACE, K_1, K_2, K_3, K_4, K_5, K_0, K_UP, K_DOWN

class CustomTimer:
    def __init__(self):
        try:
            self.timer = time.perf_counter
        except AttributeError:
            self.timer = time.time

    def time(self):
        return self.timer()

def spawn_vehicles_around_junction(world, vehicle_count=3, radius=25.0):
    """在交叉路口周围生成车辆，确保它们能相互看到"""
    map_obj = world.get_map()
    
    # 查找所有的交叉路口
    junctions = []
    waypoints = map_obj.generate_waypoints(2.0)  # 2米间隔的航路点
    
    for waypoint in waypoints:
        if waypoint.is_junction:
            # 获取交叉路口的边界框
            junction = waypoint.get_junction()
            if junction and junction.id not in [j.id for j in junctions]:
                junctions.append(junction)
    
    if not junctions:
        print("No junctions found!")
        return [], None
    
    # 选择一个交叉路口
    selected_junction = random.choice(junctions)
    center_location = selected_junction.bounding_box.location
    
    print(f"Selected junction at: {center_location}")
    print(f"Junction bounding box: {selected_junction.bounding_box}")
    
    # 获取交叉路口的所有航路点
    junction_waypoints = []
    for waypoint in waypoints:
        if waypoint.is_junction and waypoint.get_junction().id == selected_junction.id:
            junction_waypoints.append(waypoint)
    
    if len(junction_waypoints) < vehicle_count:
        print(f"Not enough waypoints in junction ({len(junction_waypoints)} found)")
        return [], None
    
    # 选择不同的航路点生成车辆
    selected_waypoints = random.sample(junction_waypoints, min(vehicle_count, len(junction_waypoints)))
    
    vehicles = []
    blueprint_library = world.get_blueprint_library()
    
    for i, waypoint in enumerate(selected_waypoints):
        try:
            spawn_location = waypoint.transform.location

            # 使用get_waypoint获取正确的偏角
            map_waypoint = map_obj.get_waypoint(spawn_location, project_to_road=False, lane_type=carla.LaneType.Driving)
            spawn_location = map_waypoint.transform.location
            spawn_location.z += 0.5  # 稍微抬升
            spawn_rotation = map_waypoint.transform.rotation
            
            spawn_transform = carla.Transform(spawn_location, spawn_rotation)
            
            # 选择车辆
            vehicle_bp = random.choice(blueprint_library.filter('vehicle.*'))
            if vehicle_bp.has_attribute('color'):
                colors = vehicle_bp.get_attribute('color').recommended_values
                vehicle_bp.set_attribute('color', random.choice(colors))
            
            vehicle = world.spawn_actor(vehicle_bp, spawn_transform)
            vehicles.append(vehicle)
            
            print(f"Vehicle {i+1} spawned:")
            print(f"  Location: {spawn_location}")
            print(f"  Rotation: {spawn_rotation}")
            
        except Exception as e:
            print(f"Failed to spawn vehicle {i+1}: {e}")
    
    return vehicles, selected_junction

class MultiVehicleDisplayManager:
    def __init__(self, grid_size, window_size, vehicle_count=2, sensors_per_vehicle=3):
        pygame.init()
        pygame.font.init()
        self.display = pygame.display.set_mode(window_size, pygame.HWSURFACE | pygame.DOUBLEBUF)
        
        # 设置窗口标题
        pygame.display.set_caption("CARLA: Multiple Vehicles with Sensors (Same World)")
        
        self.grid_size = grid_size
        self.window_size = window_size
        self.vehicle_count = vehicle_count
        self.sensors_per_vehicle = sensors_per_vehicle
        self.sensor_list = []
        
        # 为每辆车分配不同的颜色用于区分
        self.vehicle_colors = [
            (255, 0, 0),    # 红色 - 车1
            (0, 255, 0),    # 绿色 - 车2
            (0, 0, 255),    # 蓝色 - 车3
            (255, 255, 0),  # 黄色 - 车4
            (255, 0, 255),  # 紫色 - 车5
        ]
        
        # 字体用于显示车辆ID
        self.font = pygame.font.SysFont('Arial', 20)
        self.small_font = pygame.font.SysFont('Arial', 14)
        
        # 添加标志位，确保不会操作已销毁的对象
        self.is_destroyed = False

    def get_window_size(self):
        return [int(self.window_size[0]), int(self.window_size[1])]

    def get_display_size(self):
        return [int(self.window_size[0]/self.grid_size[1]), int(self.window_size[1]/self.grid_size[0])]

    def get_display_offset(self, gridPos):
        dis_size = self.get_display_size()
        return [int(gridPos[1] * dis_size[0]), int(gridPos[0] * dis_size[1])]

    def add_sensor(self, sensor, vehicle_id=0):
        sensor.vehicle_id = vehicle_id  # 标记传感器属于哪辆车
        self.sensor_list.append(sensor)

    def render(self):
        if not self.render_enabled() or self.is_destroyed:
            return

        try:
            # 清空屏幕
            self.display.fill((0, 0, 0))
            
            # 绘制网格线
            dis_size = self.get_display_size()
            for i in range(1, self.grid_size[0]):
                pygame.draw.line(self.display, (50, 50, 50), 
                                (0, i * dis_size[1]), 
                                (self.window_size[0], i * dis_size[1]), 1)
            for i in range(1, self.grid_size[1]):
                pygame.draw.line(self.display, (50, 50, 50), 
                                (i * dis_size[0], 0), 
                                (i * dis_size[0], self.window_size[1]), 1)
            
            # 渲染所有传感器
            for s in self.sensor_list:
                if s and hasattr(s, 'render'):
                    s.render()
                    
                    # 在传感器图像上添加车辆ID标签
                    offset = self.get_display_offset(s.display_pos)
                    if s.surface is not None:
                        # 绘制边框显示车辆颜色
                        border_rect = pygame.Rect(offset[0], offset[1], 
                                                 s.surface.get_width(), s.surface.get_height())
                        pygame.draw.rect(self.display, self.vehicle_colors[s.vehicle_id % len(self.vehicle_colors)], 
                                       border_rect, 3)
                        
                        # 添加车辆ID文本
                        text = self.font.render(f"Vehicle {s.vehicle_id+1}", True, 
                                               self.vehicle_colors[s.vehicle_id % len(self.vehicle_colors)])
                        text_rect = text.get_rect(topleft=(offset[0] + 5, offset[1] + 5))
                        self.display.blit(text, text_rect)
                        
                        # 添加传感器类型文本
                        sensor_text = self.font.render(s.sensor_type, True, (255, 255, 255))
                        sensor_text_rect = sensor_text.get_rect(bottomleft=(offset[0] + 5, offset[1] + s.surface.get_height() - 5))
                        self.display.blit(sensor_text, sensor_text_rect)
                        
                        # 添加传感器位置信息
                        if hasattr(s, 'sensor_position'):
                            pos_text = self.small_font.render(s.sensor_position, True, (200, 200, 200))
                            pos_text_rect = pos_text.get_rect(bottomleft=(offset[0] + 5, offset[1] + s.surface.get_height() - 25))
                            self.display.blit(pos_text, pos_text_rect)

            # 添加标题
            title = self.font.render("Same World: Multiple Vehicles with Sensors at Junction", True, (255, 255, 255))
            title_rect = title.get_rect(center=(self.window_size[0]//2, 20))
            self.display.blit(title, title_rect)
            
            # 添加说明
            instructions = [
                "SPACE: Toggle collision test mode",
                "ESC/Q: Quit",
                f"Vehicles: {self.vehicle_count} | Sensors per vehicle: {self.sensors_per_vehicle}"
                "1-N: Follow vehicle 1-N",
                "0: Free camera (no follow)",
                "UP/DOWN: Adjust camera height"
            ]
            
            for i, instruction in enumerate(instructions):
                inst_text = self.small_font.render(instruction, True, (255, 200, 0))
                inst_rect = inst_text.get_rect(bottomleft=(10, self.window_size[1] - 10 - i * 20))
                self.display.blit(inst_text, inst_rect)

            pygame.display.flip()
        except Exception as e:
            # 忽略渲染错误，特别是在清理期间
            if not self.is_destroyed:
                print(f"Warning: Render error: {e}")

    def destroy(self):
        if self.is_destroyed:
            return
            
        print("Destroying display manager...")
        try:
            # 先清空传感器列表，但不销毁传感器（传感器由VehicleSensorManager销毁）
            self.sensor_list = []
            
            # 清理pygame
            pygame.quit()
            
            self.is_destroyed = True
            print("Display manager destroyed.")
        except Exception as e:
            print(f"Error destroying display manager: {e}")

    def render_enabled(self):
        return self.display is not None and not self.is_destroyed

class VehicleSensorManager:
    def __init__(self, world, display_man, vehicle, vehicle_id, sensor_configs):
        self.world = world
        self.display_man = display_man
        self.vehicle = vehicle
        self.vehicle_id = vehicle_id
        self.sensors = []
        self.is_destroyed = False
        
        # 为车辆创建传感器
        self.create_sensors(sensor_configs)
    
    def create_sensors(self, sensor_configs):
        for i, config in enumerate(sensor_configs):
            sensor_type = config['type']
            transform = config['transform']
            sensor_options = config.get('options', {})
            sensor_position = config.get('position', 'Unknown')
            
            sensor = self.init_sensor(sensor_type, transform, sensor_options)
            if sensor:
                # 计算显示位置
                display_pos = [self.vehicle_id, i]  # 每行对应一辆车，每列对应一个传感器
                sensor_wrapper = SensorWrapper(sensor, self.display_man, sensor_type, display_pos, self.vehicle_id, sensor_position)
                self.sensors.append(sensor_wrapper)
    
    def init_sensor(self, sensor_type, transform, sensor_options):
        try:
            if sensor_type == 'RGBCamera':
                camera_bp = self.world.get_blueprint_library().find('sensor.camera.rgb')
                
                # 设置摄像头属性
                disp_size = self.display_man.get_display_size()
                camera_bp.set_attribute('image_size_x', str(disp_size[0]))
                camera_bp.set_attribute('image_size_y', str(disp_size[1]))
                
                for key in sensor_options:
                    camera_bp.set_attribute(key, sensor_options[key])
                
                # 将传感器附加到车辆上
                camera = self.world.spawn_actor(camera_bp, transform, attach_to=self.vehicle)
                return camera
                
            elif sensor_type == 'LiDAR':
                lidar_bp = self.world.get_blueprint_library().find('sensor.lidar.ray_cast')
                
                # 设置默认LIDAR属性
                lidar_bp.set_attribute('range', '50')
                lidar_bp.set_attribute('channels', '32')
                lidar_bp.set_attribute('points_per_second', '50000')
                lidar_bp.set_attribute('rotation_frequency', '10')
                
                for key in sensor_options:
                    lidar_bp.set_attribute(key, sensor_options[key])
                
                lidar = self.world.spawn_actor(lidar_bp, transform, attach_to=self.vehicle)
                return lidar
            
            elif sensor_type == 'SemanticLiDAR':
                lidar_bp = self.world.get_blueprint_library().find('sensor.lidar.ray_cast_semantic')
                lidar_bp.set_attribute('range', '50')
                
                for key in sensor_options:
                    lidar_bp.set_attribute(key, sensor_options[key])
                
                lidar = self.world.spawn_actor(lidar_bp, transform, attach_to=self.vehicle)
                return lidar
            
            else:
                print(f"Unknown sensor type: {sensor_type}")
                return None
        except Exception as e:
            print(f"Failed to create sensor {sensor_type}: {e}")
            return None
    
    def destroy(self):
        if self.is_destroyed:
            return
            
        print(f"Destroying sensor manager for vehicle {self.vehicle_id}...")
        
        # 首先停止所有传感器
        for sensor_wrapper in self.sensors:
            try:
                sensor_wrapper.stop()
            except:
                pass
        
        # 然后销毁所有传感器
        for sensor_wrapper in self.sensors:
            try:
                sensor_wrapper.destroy()
            except:
                pass
        
        self.sensors = []
        self.is_destroyed = True
        print(f"Sensor manager for vehicle {self.vehicle_id} destroyed.")

class SensorWrapper:
    def __init__(self, sensor, display_man, sensor_type, display_pos, vehicle_id, sensor_position="Unknown"):
        self.sensor = sensor
        self.display_man = display_man
        self.sensor_type = sensor_type
        self.display_pos = display_pos
        self.vehicle_id = vehicle_id
        self.sensor_position = sensor_position
        self.surface = None
        self.timer = CustomTimer()
        self.is_destroyed = False
        self.is_stopped = False
        
        # 根据传感器类型设置回调
        if sensor_type == 'RGBCamera':
            self.sensor.listen(self.save_rgb_image)
        elif sensor_type == 'LiDAR':
            self.sensor.listen(self.save_lidar_image)
        elif sensor_type == 'SemanticLiDAR':
            self.sensor.listen(self.save_semanticlidar_image)
        
        # 添加到显示管理器
        self.display_man.add_sensor(self, vehicle_id)
    
    def stop(self):
        """停止传感器监听"""
        if not self.is_stopped:
            try:
                # 停止传感器监听
                if self.sensor and hasattr(self.sensor, 'stop'):
                    self.sensor.stop()
                self.is_stopped = True
            except Exception as e:
                print(f"Warning: Error stopping sensor: {e}")
    
    def save_rgb_image(self, image):
        if self.is_destroyed:
            return
            
        try:
            t_start = self.timer.time()
            
            image.convert(carla.ColorConverter.Raw)
            array = np.frombuffer(image.raw_data, dtype=np.dtype("uint8"))
            array = np.reshape(array, (image.height, image.width, 4))
            array = array[:, :, :3]
            array = array[:, :, ::-1]  # 转换BGR到RGB
            
            if self.display_man.render_enabled() and not self.is_destroyed:
                self.surface = pygame.surfarray.make_surface(array.swapaxes(0, 1))
        except Exception as e:
            # 忽略已销毁传感器的回调错误
            if not self.is_destroyed:
                print(f"Warning: Error saving RGB image: {e}")
    
    def save_lidar_image(self, image):
        if self.is_destroyed:
            return
            
        try:
            t_start = self.timer.time()
            
            disp_size = self.display_man.get_display_size()
            lidar_range = 2.0 * 50.0  # 使用50米范围
            
            points = np.frombuffer(image.raw_data, dtype=np.dtype('f4'))
            points = np.reshape(points, (int(points.shape[0] / 4), 4))
            lidar_data = np.array(points[:, :2])
            lidar_data *= min(disp_size) / lidar_range
            lidar_data += (0.5 * disp_size[0], 0.5 * disp_size[1])
            lidar_data = np.fabs(lidar_data)
            lidar_data = lidar_data.astype(np.int32)
            lidar_data = np.reshape(lidar_data, (-1, 2))
            lidar_img_size = (disp_size[0], disp_size[1], 3)
            lidar_img = np.zeros(lidar_img_size, dtype=np.uint8)
            
            # 使用车辆颜色绘制点云
            vehicle_color = self.display_man.vehicle_colors[self.vehicle_id % len(self.display_man.vehicle_colors)]
            lidar_img[tuple(lidar_data.T)] = vehicle_color
            
            if self.display_man.render_enabled() and not self.is_destroyed:
                self.surface = pygame.surfarray.make_surface(lidar_img)
        except Exception as e:
            # 忽略已销毁传感器的回调错误
            if not self.is_destroyed:
                print(f"Warning: Error saving LiDAR image: {e}")
    
    def save_semanticlidar_image(self, image):
        if self.is_destroyed:
            return
            
        try:
            t_start = self.timer.time()
            
            disp_size = self.display_man.get_display_size()
            lidar_range = 2.0 * 50.0
            
            points = np.frombuffer(image.raw_data, dtype=np.dtype('f4'))
            points = np.reshape(points, (int(points.shape[0] / 6), 6))
            lidar_data = np.array(points[:, :2])
            lidar_data *= min(disp_size) / lidar_range
            lidar_data += (0.5 * disp_size[0], 0.5 * disp_size[1])
            lidar_data = np.fabs(lidar_data)
            lidar_data = lidar_data.astype(np.int32)
            lidar_data = np.reshape(lidar_data, (-1, 2))
            lidar_img_size = (disp_size[0], disp_size[1], 3)
            lidar_img = np.zeros(lidar_img_size, dtype=np.uint8)
            
            vehicle_color = self.display_man.vehicle_colors[self.vehicle_id % len(self.display_man.vehicle_colors)]
            lidar_img[tuple(lidar_data.T)] = vehicle_color
            
            if self.display_man.render_enabled() and not self.is_destroyed:
                self.surface = pygame.surfarray.make_surface(lidar_img)
        except Exception as e:
            # 忽略已销毁传感器的回调错误
            if not self.is_destroyed:
                print(f"Warning: Error saving Semantic LiDAR image: {e}")
    
    def render(self):
        if self.is_destroyed:
            return
            
        try:
            if self.surface is not None:
                offset = self.display_man.get_display_offset(self.display_pos)
                self.display_man.display.blit(self.surface, offset)
        except Exception as e:
            # 忽略渲染错误
            pass
    
    def destroy(self):
        if self.is_destroyed:
            return
            
        print(f"Destroying {self.sensor_type} sensor for vehicle {self.vehicle_id}...")
        
        self.is_destroyed = True
        
        try:
            # 停止传感器
            self.stop()
            
            # 销毁传感器
            if self.sensor and self.sensor.is_alive:
                self.sensor.destroy()
                
            self.sensor = None
            self.surface = None
            print(f"{self.sensor_type} sensor for vehicle {self.vehicle_id} destroyed.")
        except Exception as e:
            print(f"Warning: Error destroying sensor: {e}")

def update_spectator(world, vehicles, follow_vehicle_index, camera_height=50.0, use_top_down=True):
    """
    更新spectator的位置和视角
    follow_vehicle_index: -1表示不跟随任何车辆，0~n表示跟随第n辆车
    camera_height: 相机高度（米）
    use_top_down: 是否使用俯视视角
    """
    if not vehicles or follow_vehicle_index < 0 or follow_vehicle_index >= len(vehicles):
        return
    
    try:
        vehicle = vehicles[follow_vehicle_index]
        vehicle_transform = vehicle.get_transform()
        vehicle_location = vehicle_transform.location
        
        if use_top_down:
            # 俯视视角：在车辆正上方，向下看
            spectator_location = carla.Location(
                x=vehicle_location.x,
                y=vehicle_location.y,
                z=vehicle_location.z + camera_height  # 在车辆上方指定高度
            )
            
            # 俯视视角：pitch=-90度（向下看），yaw=0度（朝北），可以根据需要调整
            spectator_rotation = carla.Rotation(pitch=-90, yaw=0, roll=0)
        else:
            # 第三人称视角：在车辆后方和上方
            forward_vector = vehicle_transform.get_forward_vector()
            right_vector = vehicle_transform.get_right_vector()
            
            spectator_location = carla.Location(
                x=vehicle_location.x - forward_vector.x * 10 + right_vector.x * 3,
                y=vehicle_location.y - forward_vector.y * 10 + right_vector.y * 3,
                z=vehicle_location.z + 5  # 稍微抬高
            )
            
            # 看向车辆
            spectator_rotation = carla.Rotation(
                pitch=-15,
                yaw=vehicle_transform.rotation.yaw,
                roll=0
            )
        
        spectator_transform = carla.Transform(spectator_location, spectator_rotation)
        spectator = world.get_spectator()
        spectator.set_transform(spectator_transform)
        
    except Exception as e:
        print(f"Failed to update spectator: {e}")

def run_demo(args, client):
    """运行多车多传感器的演示"""
    
    display_manager = None
    vehicles = []
    vehicle_sensor_managers = []
    selected_junction = None
    timer = CustomTimer()
    
    # 相机控制变量
    follow_vehicle_index = 0  # 默认跟随第一辆车
    camera_height = 50.0  # 默认相机高度（米）
    camera_min_height = 20.0  # 最小相机高度
    camera_max_height = 100.0  # 最大相机高度
    camera_height_step = 5.0  # 相机高度调整步长
    
    # 获取世界
    world = client.get_world()
    original_settings = world.get_settings()
        
    try:
        # 设置同步模式
        if args.sync:
            traffic_manager = client.get_trafficmanager(args.tm_port)
            settings = world.get_settings()
            traffic_manager.set_synchronous_mode(True)
            settings.synchronous_mode = True
            settings.fixed_delta_seconds = 0.05
            world.apply_settings(settings)
        
        # 创建显示管理器
        vehicle_count = args.vehicles
        sensors_per_vehicle = 3  # 每辆车3个传感器
        grid_size = [vehicle_count, sensors_per_vehicle]
        display_manager = MultiVehicleDisplayManager(
            grid_size=grid_size, 
            window_size=[args.width, args.height],
            vehicle_count=vehicle_count,
            sensors_per_vehicle=sensors_per_vehicle
        )
        
        # 在交叉路口周围生成车辆
        vehicles, selected_junction = spawn_vehicles_around_junction(world, vehicle_count=args.vehicles, radius=25.0)
        
        if not vehicles:
            print("Warning: Failed to spawn vehicles around junction. Using fallback method.")
            # 回退到原始方法
            vehicles = []
            spawn_points = world.get_map().get_spawn_points()
            blueprint_library = world.get_blueprint_library()
            
            for i in range(vehicle_count):
                try:
                    vehicle_bp = random.choice(blueprint_library.filter('vehicle.*'))
                    if vehicle_bp.has_attribute('color'):
                        colors = vehicle_bp.get_attribute('color').recommended_values
                        vehicle_bp.set_attribute('color', random.choice(colors))
                    
                    spawn_point = spawn_points[i % len(spawn_points)]
                    if i > 0:
                        spawn_point.location.x += random.uniform(-5, 5)
                        spawn_point.location.y += random.uniform(-5, 5)
                    
                    vehicle = world.spawn_actor(vehicle_bp, spawn_point)
                    vehicles.append(vehicle)
                    print(f"Fallback: Spawned vehicle {i+1} at {spawn_point.location}")
                except Exception as e:
                    print(f"Failed to spawn fallback vehicle {i+1}: {e}")
        
        if not vehicles:
            print("Error: No vehicles spawned. Exiting.")
            return
        
        # 为生成的车辆设置自动驾驶
        # for vehicle in vehicles:
        #     try:
        #         vehicle.set_autopilot(True, args.tm_port)
        #     except Exception as e:
        #         print(f"Warning: Failed to set autopilot for vehicle: {e}")
        
        print(f"Successfully spawned {len(vehicles)} vehicles around junction")
        
        # 为每辆车创建传感器
        sensor_configs = [
            {
                'type': 'RGBCamera',
                'transform': carla.Transform(carla.Location(x=1.5, z=2.4), carla.Rotation(pitch=0)),
                'options': {'fov': '90'},
                'position': 'Front'
            },
            {
                'type': 'RGBCamera',
                'transform': carla.Transform(carla.Location(x=-1.5, z=2.4), carla.Rotation(pitch=0, yaw=180)),
                'options': {'fov': '90'},
                'position': 'Rear'
            },
            {
                'type': 'LiDAR',
                'transform': carla.Transform(carla.Location(x=0, z=2.4)),
                'options': {'channels': '32', 'range': '50', 'points_per_second': '100000', 'rotation_frequency': '20'},
                'position': 'Roof'
            }
        ]
        
        for i, vehicle in enumerate(vehicles):
            try:
                sensor_manager = VehicleSensorManager(world, display_manager, vehicle, i, sensor_configs)
                vehicle_sensor_managers.append(sensor_manager)
            except Exception as e:
                print(f"Failed to create sensor manager for vehicle {i}: {e}")
        
        print("All sensors created. Starting simulation...")
        
        # 添加调试：显示交叉路口边界框
        if selected_junction:
            try:
                debug = world.debug
                center_location = selected_junction.bounding_box.location
                debug.draw_box(
                    selected_junction.bounding_box,
                    selected_junction.bounding_box.rotation,
                    thickness=0.1,
                    color=carla.Color(255, 0, 0),
                    life_time=30.0
                )
                debug.draw_string(
                    center_location,
                    f"Junction {selected_junction.id}",
                    draw_shadow=False,
                    color=carla.Color(255, 255, 0),
                    life_time=30.0
                )
                print(f"Debug: Junction {selected_junction.id} highlighted in red")
            except Exception as e:
                print(f"Warning: Failed to draw junction debug info: {e}")
        
        # 设置初始相机视角（跟随第一辆车，俯视视角）
        update_spectator(world, vehicles, follow_vehicle_index, camera_height, use_top_down=True)
        print(f"Camera: Following vehicle {follow_vehicle_index+1} at height {camera_height}m (top-down view)")
        
        # 仿真循环
        clock = pygame.time.Clock()
        is_running = True
        test_collision_mode = False
        
        while is_running:
            # 处理事件
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    is_running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == K_ESCAPE or event.key == K_q:
                        is_running = False
                    elif event.key == K_SPACE:
                        # 测试碰撞：将所有车辆移动到同一位置
                        test_collision_mode = not test_collision_mode
                        if test_collision_mode:
                            print("TEST MODE: Moving all vehicles to the same location to test collision...")
                            # 将所有车辆移动到交叉路口中心
                            if vehicles and selected_junction:
                                target_location = selected_junction.bounding_box.location
                                target_location.z += 1.0  # 稍微抬升
                                for i, vehicle in enumerate(vehicles):
                                    try:
                                        # 稍微偏移以避免完全重叠
                                        offset_transform = carla.Transform(
                                            carla.Location(
                                                target_location.x + i * 2.0,
                                                target_location.y,
                                                target_location.z
                                            ),
                                            carla.Rotation(0, 0, 0)
                                        )
                                        vehicle.set_transform(offset_transform)
                                        vehicle.set_autopilot(False)  # 关闭自动驾驶
                                    except Exception as e:
                                        print(f"Warning: Failed to move vehicle {i}: {e}")
                        else:
                            print("Returning to normal mode...")
                            for vehicle in vehicles:
                                try:
                                    vehicle.set_autopilot(True, args.tm_port)  # 重新启用自动驾驶
                                except Exception as e:
                                    print(f"Warning: Failed to set autopilot: {e}")
                    
                    # 相机控制
                    elif event.key == K_0:
                        follow_vehicle_index = -1
                        print("Camera: Free mode (not following any vehicle)")
                    elif event.key == K_1 and len(vehicles) >= 1:
                        follow_vehicle_index = 0
                        print(f"Camera: Following vehicle 1 at height {camera_height}m")
                    elif event.key == K_2 and len(vehicles) >= 2:
                        follow_vehicle_index = 1
                        print(f"Camera: Following vehicle 2 at height {camera_height}m")
                    elif event.key == K_3 and len(vehicles) >= 3:
                        follow_vehicle_index = 2
                        print(f"Camera: Following vehicle 3 at height {camera_height}m")
                    elif event.key == K_4 and len(vehicles) >= 4:
                        follow_vehicle_index = 3
                        print(f"Camera: Following vehicle 4 at height {camera_height}m")
                    elif event.key == K_5 and len(vehicles) >= 5:
                        follow_vehicle_index = 4
                        print(f"Camera: Following vehicle 5 at height {camera_height}m")
                    elif event.key == K_UP:
                        camera_height = min(camera_height + camera_height_step, camera_max_height)
                        print(f"Camera: Height increased to {camera_height}m")
                    elif event.key == K_DOWN:
                        camera_height = max(camera_height - camera_height_step, camera_min_height)
                        print(f"Camera: Height decreased to {camera_height}m")
            
            # Carla Tick
            try:
                if args.sync:
                    world.tick()
                else:
                    world.wait_for_tick()
            except Exception as e:
                print(f"Warning: Error during tick: {e}")
                break
            
            # 更新相机视角（如果跟随车辆）
            if follow_vehicle_index >= 0 and follow_vehicle_index < len(vehicles):
                update_spectator(world, vehicles, follow_vehicle_index, camera_height, use_top_down=True)
            
            # 渲染接收到的数据
            try:
                display_manager.render()
            except Exception as e:
                print(f"Warning: Error rendering: {e}")
            
            # 控制帧率
            clock.tick(30)
        
        print("Simulation ended.")
        
    except KeyboardInterrupt:
        print("\nSimulation interrupted by user.")
    except Exception as e:
        print(f"\nError during simulation: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # 清理 - 按正确顺序进行
        print("\nCleaning up...")
        
        # 1. 首先停止所有传感器
        print("Stopping all sensors...")
        for manager in vehicle_sensor_managers:
            try:
                manager.destroy()
            except Exception as e:
                print(f"Warning: Error destroying sensor manager: {e}")
        
        # 2. 然后销毁显示管理器
        print("Destroying display manager...")
        if display_manager:
            try:
                display_manager.destroy()
            except Exception as e:
                print(f"Warning: Error destroying display manager: {e}")
        
        # 3. 最后销毁所有车辆
        print(f"Destroying {len(vehicles)} vehicles...")
        if vehicles:
            try:
                # 批量销毁车辆
                destroy_commands = []
                for vehicle in vehicles:
                    if vehicle and vehicle.is_alive:
                        destroy_commands.append(carla.command.DestroyActor(vehicle))
                
                if destroy_commands:
                    client.apply_batch(destroy_commands)
                
                print("All vehicles destroyed.")
            except Exception as e:
                print(f"Warning: Error destroying vehicles: {e}")
        
        # 恢复原始设置
        try:
            if original_settings:
                world.apply_settings(original_settings)
                print("Original settings restored.")
        except Exception as e:
            print(f"Warning: Error restoring settings: {e}")
        
        print("Cleanup complete.")

def main():
    argparser = argparse.ArgumentParser(
        description='CARLA Multi-Vehicle Sensors Demo')
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
        '--vehicles',
        type=int,
        default=4,
        help='Number of vehicles to spawn (default: 4)')
    argparser.add_argument(
        '--tm-port',
        metavar='P',
        default=8000,
        type=int,
        help='Port to communicate with TM (default: 8000)')
    argparser.add_argument(
        '--res',
        metavar='WIDTHxHEIGHT',
        default='1280x720',
        help='window resolution (default: 1280x720)')
    argparser.add_argument(
        '--camera-height',
        type=float,
        default=50.0,
        help='Initial camera height in meters (default: 50.0)')
    
    args = argparser.parse_args()
    
    # 解析分辨率
    args.width, args.height = [int(x) for x in args.res.split('x')]
    
    try:
        # 连接到CARLA服务器
        client = carla.Client(args.host, args.port)
        client.set_timeout(10.0)
        
        # 运行演示
        run_demo(args, client)
        
    except KeyboardInterrupt:
        print('\nCancelled by user. Bye!')
    except Exception as e:
        print(f'\nError: {e}')
        import traceback
        traceback.print_exc()

def set_synchronous_mode_bkup(client, args):
    world = client.get_world()
    settings = world.get_settings()
    settings.synchronous_mode = False
    world.apply_settings(settings)

    traffic_manager = client.get_trafficmanager(args.tm_port)
    traffic_manager.set_synchronous_mode(False)

if __name__ == '__main__':
    main()
