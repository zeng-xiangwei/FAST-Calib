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

    bool readCloudFromBag(const std::string& bag_path, const std::string& lidar_topic) {

        try
        {
            // 创建bag读取器
            rosbag2_cpp::Reader reader;
            reader.open(bag_path);

            // 设置序列化格式
            rclcpp::Serialization<sensor_msgs::msg::PointCloud2> serialization;

            while (reader.has_next())
            {
                auto bag_message = reader.read_next();

                // 检查是否是目标topic
                if (bag_message->topic_name != lidar_topic)
                {
                    continue;
                }

                // 反序列化消息
                auto ros_msg = std::make_shared<sensor_msgs::msg::PointCloud2>();
                rclcpp::SerializedMessage extracted_serialized_msg(*bag_message->serialized_data);
                serialization.deserialize_message(&extracted_serialized_msg, ros_msg.get());

                // 转换为PCL点云
                pcl::PointCloud<pcl::PointXYZ>::Ptr frame(new pcl::PointCloud<pcl::PointXYZ>);
                pcl::fromROSMsg(*ros_msg, *frame);
                *cloud_input_ += *frame;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading bag file: " << e.what() << std::endl;
            return false;
        }

        return true;
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