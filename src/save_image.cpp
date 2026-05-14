#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>

#include <opencv2/opencv.hpp>

class DepathImageToPointCloud : public rclcpp::Node {
public:

    DepathImageToPointCloud() : Node("image_get") {
        color_image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/color/image_raw", 1,
            [this](const sensor_msgs::msg::Image::SharedPtr msg) {
                ColorImageCallback(msg);
            });
        
        image_file_ = this->declare_parameter("image_file", "");
    }

    void ColorImageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(), "Received color image");
        cv_bridge::CvImagePtr cv_ptr =
            cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        color_image_ = cv_ptr->image;

        if (image_file_.empty()) {
            RCLCPP_ERROR(this->get_logger(), "Image file is empty");
            return;
        }
        cv::imwrite(image_file_, color_image_);
    }

private:
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr color_image_sub_;
    cv::Mat color_image_;
    std::string image_file_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);

    std::shared_ptr<DepathImageToPointCloud> node =
        std::make_shared<DepathImageToPointCloud>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}