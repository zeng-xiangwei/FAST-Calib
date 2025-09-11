from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.conditions import IfCondition
from launch.actions import SetEnvironmentVariable
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    image_file = DeclareLaunchArgument('image_file', default_value='')

    image_get_node = Node(
        package='fast_calib',
        executable='save_image',
        name='save_image',
        output='screen',
        parameters=[{'image_file': LaunchConfiguration('image_file'),}]
    )
    
    return LaunchDescription([
        image_file,
        image_get_node
    ]) 	
