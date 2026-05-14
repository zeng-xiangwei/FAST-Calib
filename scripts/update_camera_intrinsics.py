#!/usr/bin/env python3
"""
从 /camera_info 话题读取相机内参并更新 qr_params.yaml
使用方式: ros2 run fast_calib update_camera_intrinsics.py [--yaml PATH] [--topic TOPIC]
"""

import argparse
import sys
import yaml
import os

from ament_index_python.packages import get_package_share_directory

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CameraInfo


class CameraIntrinsicsUpdater(Node):
    def __init__(self, yaml_path, topic):
        super().__init__('update_camera_intrinsics')
        self.yaml_path = yaml_path
        self.subscription = self.create_subscription(
            CameraInfo,
            topic,
            self.camera_info_callback,
            10
        )
        self.get_logger().info(f'Listening to {topic} for camera intrinsics...')
        self.got_data = False

    def camera_info_callback(self, msg):
        if self.got_data:
            return  # 只处理第一条消息

        # 从 CameraInfo 的 K 矩阵中获取内参
        # K = [fx  0 cx]
        #     [0  fy cy]
        #     [0   0  1]
        fx = msg.k[0]
        fy = msg.k[4]
        cx = msg.k[2]
        cy = msg.k[5]

        self.get_logger().info(f'Received camera intrinsics:')
        self.get_logger().info(f'  fx = {fx}')
        self.get_logger().info(f'  fy = {fy}')
        self.get_logger().info(f'  cx = {cx}')
        self.get_logger().info(f'  cy = {cy}')

        # 更新 YAML 文件
        self.update_yaml(fx, fy, cx, cy)
        self.got_data = True

    def update_yaml(self, fx, fy, cx, cy):
        try:
            with open(self.yaml_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            # 更新内参
            data['fast_calib']['ros__parameters']['fx'] = fx
            data['fast_calib']['ros__parameters']['fy'] = fy
            data['fast_calib']['ros__parameters']['cx'] = cx
            data['fast_calib']['ros__parameters']['cy'] = cy

            with open(self.yaml_path, 'w', encoding='utf-8') as f:
                yaml.dump(data, f, default_flow_style=False, allow_unicode=True)

            self.get_logger().info(f'Successfully updated {self.yaml_path}')
        except Exception as e:
            self.get_logger().error(f'Error updating YAML: {e}')


def main():
    parser = argparse.ArgumentParser(description='Update camera intrinsics from /camera_info topic')
    parser.add_argument('--yaml', '-y', 
                        default=None,
                        help='Path to qr_params.yaml file (default: auto-detect from package share)')
    parser.add_argument('--topic', '-t',
                        default='/camera_info',
                        help='Camera info topic name')
    
    args = parser.parse_args()

    # 使用 FindPackageShare 方式获取配置文件路径
    if args.yaml is None:
        try:
            pkg_share = get_package_share_directory('fast_calib')
            args.yaml = os.path.join(pkg_share, 'config', 'qr_params.yaml')
        except Exception as e:
            print(f'Error: Could not find fast_calib package: {e}')
            sys.exit(1)
    
    print(f'Config path: {args.yaml}')
    
    rclpy.init()
    updater = CameraIntrinsicsUpdater(args.yaml, args.topic)

    while rclpy.ok() and not updater.got_data:
        rclpy.spin_once(updater, timeout_sec=0.1)

    rclpy.shutdown()


if __name__ == '__main__':
    main()