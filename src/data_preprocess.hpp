/* 
Developer: Chunran Zheng <zhengcr@connect.hku.hk>

This file is subject to the terms and conditions outlined in the 'LICENSE' file,
which is included as part of this source code package.
*/

#ifndef DATA_PREPROCESS_HPP
#define DATA_PREPROCESS_HPP

#include <Eigen/Core>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/readers/sequential_reader.hpp>
#include <opencv2/opencv.hpp>
#include "common_lib.h"

// livox_ros_driver2 message
#include <livox_ros_driver2/msg/custom_msg.hpp>


using namespace std;
using namespace cv;

class DataPreprocess
{
public:
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_input_;

    cv::Mat img_input_;

    DataPreprocess(Params &params)
        : cloud_input_(new pcl::PointCloud<pcl::PointXYZ>)
    {
        string bag_path = params.bag_path;
        string image_path = params.image_path;
        string lidar_topic = params.lidar_topic;

        img_input_ = cv::imread(params.image_path, cv::IMREAD_UNCHANGED);
        if (img_input_.empty()) 
        {
            std::cout << "Loading the image " << image_path << " failed" << std::endl;
            return;
        }

        if (isPCDFile(bag_path)) {
            if (!readCloudFromPCD(bag_path)) {
                return;
            }
        } else {
            if (!readCloudFromBag(bag_path, lidar_topic)) {
                return;
            }
        }

        std::cout << "Loaded " << cloud_input_->size() << "points from the rosbag." << std::endl;
    }

    /**
     * @brief 读取bag文件，自动判断消息类型（sensor_msgs::PointCloud2 或 livox_ros_driver2::CustomMsg）
     */
    bool readCloudFromBag(const std::string& bag_path, const std::string& lidar_topic) {
        
        // 首先尝试 livox_ros_driver2::CustomMsg
        cloud_input_->clear();
        if (tryReadLivoxCustomMsgBag(bag_path, lidar_topic)) {
            if (!cloud_input_->empty()) {
                std::cout << "Detected livox_ros_driver2::CustomMsg format" << std::endl;
                return true;
            }
        }

        // 失败后尝试 sensor_msgs::PointCloud2 读取
        cloud_input_->clear();
        if (tryReadSensorMsgsBag(bag_path, lidar_topic)) {
            if (!cloud_input_->empty()) {
                std::cout << "Detected sensor_msgs::PointCloud2 format" << std::endl;
                return true;
            }
        }
    
        
        std::cerr << "Failed to read cloud data from bag file" << std::endl;
        return false;
    }

    /**
     * @brief 尝试以 sensor_msgs::PointCloud2 格式读取bag
     */
    bool tryReadSensorMsgsBag(const std::string& bag_path, const std::string& lidar_topic) {
        try
        {
            rosbag2_cpp::Reader reader;
            reader.open(bag_path);

            rclcpp::Serialization<sensor_msgs::msg::PointCloud2> serialization;

            while (reader.has_next())
            {
                auto bag_message = reader.read_next();

                if (bag_message->topic_name != lidar_topic)
                {
                    continue;
                }

                auto ros_msg = std::make_shared<sensor_msgs::msg::PointCloud2>();
                rclcpp::SerializedMessage extracted_serialized_msg(*bag_message->serialized_data);
                serialization.deserialize_message(&extracted_serialized_msg, ros_msg.get());

                // 检查是否是有效的 PointCloud2
                if (ros_msg->width == 0 || ros_msg->height == 0) {
                    continue;
                }

                pcl::PointCloud<pcl::PointXYZ>::Ptr frame(new pcl::PointCloud<pcl::PointXYZ>);
                pcl::fromROSMsg(*ros_msg, *frame);
                
                // 检查是否有有效点
                if (frame->empty()) {
                    continue;
                }
                
                *cloud_input_ += *frame;
            }
            
            return !cloud_input_->empty();
        }
        catch (const std::exception &e)
        {
            std::cerr << "tryReadSensorMsgsBag failed: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 尝试以 livox_ros_driver2::CustomMsg 格式读取bag
     */
    bool tryReadLivoxCustomMsgBag(const std::string& bag_path, const std::string& lidar_topic) {
        try
        {
            rosbag2_cpp::Reader reader;
            reader.open(bag_path);

            rclcpp::Serialization<livox_ros_driver2::msg::CustomMsg> serialization;

            while (reader.has_next())
            {
                auto bag_message = reader.read_next();

                if (bag_message->topic_name != lidar_topic)
                {
                    continue;
                }

                auto ros_msg = std::make_shared<livox_ros_driver2::msg::CustomMsg>();
                rclcpp::SerializedMessage extracted_serialized_msg(*bag_message->serialized_data);
                serialization.deserialize_message(&extracted_serialized_msg, ros_msg.get());

                // 从 livox CustomMsg 提取点云
                for (const auto& point : ros_msg->points)
                {
                    pcl::PointXYZ p;
                    p.x = point.x;
                    p.y = point.y;
                    p.z = point.z;
                    
                    // 过滤无效点 (livox 常见处理)
                    if (std::isnan(p.x) || std::isnan(p.y) || std::isnan(p.z)) {
                        continue;
                    }
                    
                    cloud_input_->push_back(p);
                }
            }
            
            return !cloud_input_->empty();
        }
        catch (const std::exception &e)
        {
            std::cerr << "tryReadLivoxCustomMsgBag failed: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 直接读取 livox 自定义消息格式的 bag（保留作为独立函数）
     */
    bool readCloudFromLivoxBag(const std::string& bag_path, const std::string& lidar_topic) {
        return tryReadLivoxCustomMsgBag(bag_path, lidar_topic);
    }

    bool readCloudFromPCD(const std::string& pcd_file) {
        try {
            pcl::io::loadPCDFile(pcd_file, *cloud_input_);
        } catch (pcl::IOException& e) {
            std::cout << "LOADING BAG FAILED: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    bool isPCDFile(const std::string& path) {
        if (path.length() < 5) {
            return false;
        }

        // 检查是否以.pcd结尾(不区分大小写)
        std::string extension = path.substr(path.length() - 4);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        return extension == ".pcd";
    }
};

typedef std::shared_ptr<DataPreprocess> DataPreprocessPtr;

#endif
