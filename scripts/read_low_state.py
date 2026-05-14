#!/usr/bin/env python3
"""
读取 /low_state 话题中的关节角并写入 YAML 文件
使用方式: ros2 run fast_calib read_low_state.py [--yaml PATH] [--topic TOPIC]

关节索引 (来自 h10_w):
  - TORSO = 17
  - HEAD_PITCH = 15  
  - HEAD_YAW = 14
"""

import argparse
import sys
import yaml
import os
import time

from ament_index_python.packages import get_package_share_directory

import rclpy
from rclpy.node import Node

from h10_w.msg import LowState


class LowStateReader(Node):
    def __init__(self, yaml_path, topic):
        super().__init__('read_low_state')
        self.yaml_path = yaml_path
        self.got_data = False
        
        self.subscription = self.create_subscription(
            LowState,
            topic,
            self.low_state_callback,
            10
        )
        self.get_logger().info(f'Listening to {topic} for joint states...')

    def low_state_callback(self, msg):
        if self.got_data:
            return  # 只处理第一条消息

        # 读取关节角
        # TORSO = 17, HEAD_PITCH = 15, HEAD_YAW = 14
        torso_height = msg.joint_motor_state[17].q
        tilt_angle = msg.joint_motor_state[15].q
        pan_angle = msg.joint_motor_state[14].q

        self.get_logger().info(f'Received joint states:')
        self.get_logger().info(f'  torso_height = {torso_height}')
        self.get_logger().info(f'  tilt_angle = {tilt_angle}')
        self.get_logger().info(f'  pan_angle = {pan_angle}')

        # 写入 YAML 文件
        self.update_yaml(torso_height, tilt_angle, pan_angle)
        self.got_data = True

    def update_yaml(self, torso_height, tilt_angle, pan_angle):
        try:
            with open(self.yaml_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            # 更新关节角
            data['init_torso_height'] = torso_height
            data['init_tilt_angle'] = tilt_angle
            data['init_pan_angle'] = pan_angle

            with open(self.yaml_path, 'w', encoding='utf-8') as f:
                yaml.dump(data, f, default_flow_style=False, allow_unicode=True)

            self.get_logger().info(f'Successfully updated {self.yaml_path}')
        except Exception as e:
            self.get_logger().error(f'Error updating YAML: {e}')


def main():
    parser = argparse.ArgumentParser(description='Read joint states from /low_state topic')
    parser.add_argument('--yaml', '-y',
                        default=None,
                        help='Path to output YAML file (default: auto-detect from package share)')
    parser.add_argument('--topic', '-t',
                        default='/low_state',
                        help='Low state topic name')
    
    args = parser.parse_args()

    # 使用 FindPackageShare 方式获取配置文件路径
    if args.yaml is None:
        try:
            pkg_share = get_package_share_directory('fast_calib')
            args.yaml = os.path.join(pkg_share, 'config', 'joint_init.yaml')
        except Exception as e:
            print(f'Error: Could not find fast_calib package: {e}')
            sys.exit(1)
    
    print(f'Output path: {args.yaml}')
    
    rclpy.init()
    reader = LowStateReader(args.yaml, args.topic)

    max_spin_time = 10
    start = time.time()
    while rclpy.ok() and not reader.got_data:
        rclpy.spin_once(reader, timeout_sec=0.1)
        if time.time() - start > max_spin_time:
            print(f'Error: No data received within {max_spin_time} seconds')
            break

    rclpy.shutdown()


if __name__ == '__main__':
    main()