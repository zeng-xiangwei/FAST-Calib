#!/bin/bash


kill_roslaunch_process() {
  local ROSLAUNCH="$1"
  local PID=$(ps aux | grep "$ROSLAUNCH" | grep -v grep | awk '{print $2}')
  
  if [ -z "$PID" ]; then
    echo "No 【$ROSLAUNCH process found."
  else
    echo "Killing 【$ROSLAUNCH process with PID $PID"
    pkill -P -2 $PID
    pkill -P $PID  # 杀死所有子进程
    kill $PID
    # Optionally, check if the process was killed successfully
    if [ $? -eq 0 ]; then
      echo "【$ROSLAUNCH process killed successfully."
    else
      echo "Failed to kill 【$ROSLAUNCH process. Using kill -9."
      pkill -9 -P $PID
      kill -9 $PID
      echo "【$ROSLAUNCH process forcibly killed."
    fi
  fi
}

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 0. 执行该脚本前，需要 source ros2 环境
# 1. 在 jetson 中执行 rs-enumerate-devices -c，记录相机内参
# 2. 在车端或本机执行 livox 驱动，可以是 pointcloud2 类型或 livox 自定义类型
# 3. 在车端执行相机驱动
# 4. 获取一帧图像

ros2 daemon stop
ros2 daemon start

CUR_SHELL_DIR=$(cd `dirname $0`; pwd)
PROJECT_DIR="/home/xiangweizeng/3D_slam/fast_calib_ws"
echo "start save one image"
cd $PROJECT_DIR
source ./install/setup.bash
ros2 launch image_get image_get.launch.py image_file:=$CUR_SHELL_DIR/current.png &
sleep 3
kill_roslaunch_process "ros2 launch image_get image_get.launch.py"
echo -e "${GREEN}saved image to $CUR_SHELL_DIR/current.png.${NC}"
sleep 3

# 5. 录制数据 ros2 bag
echo "start record livox lidar"
cd $CUR_SHELL_DIR
ros2bag_file_name="livox_lidar"
ros2 bag record /livox/lidar -o $ros2bag_file_name &
sleep 20
kill_roslaunch_process "ros2 bag record"
sleep 5
echo -e "${GREEN}saved ros2 bag to ${CUR_SHELL_DIR}/${ros2bag_file_name}.${NC}"
ros2 bag info ${CUR_SHELL_DIR}/$ros2bag_file_name

# 6. 记录 low_state
echo "start save low_state"
cd $PROJECT_DIR
source ./install/setup.bash
low_state_file=${CUR_SHELL_DIR}/low_state.txt
ros2 topic echo /low_state > $low_state_file &
sleep 5.0
kill_roslaunch_process "ros2 topic echo /low_state"
echo -e "${GREEN}saved low_state to $low_state_file.${NC}"

# 7. 启动标定程序. 需要根据标定位置以及数据路径调整 qr_params.yaml 中的参数
# cd $PROJECT_DIR
# source ./install/setup.bash
# ros2 launch fast_calib calib.launch.py 

