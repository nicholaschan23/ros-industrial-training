#include <rclcpp/rclcpp.hpp>
#include <myworkcell_core/srv/localize_part.hpp>

class ScanNPlan : public rclcpp::Node
{
public:
  ScanNPlan() : Node("scan_n_plan")
  {
    vision_client_ = this->create_client<myworkcell_core::srv::LocalizePart>("localize_part");
    // (parameter name, default value)
    // Parameter type is fixed from the type of he default value
    this->declare_parameter("base_frame", "world");
  }

  void start(const std::string& base_frame)
  {    
    RCLCPP_INFO(get_logger(), "Attempting to localize part");

    // Wait for service to be available
    if (!vision_client_->wait_for_service(std::chrono::seconds(5)))
    {
      RCLCPP_ERROR(get_logger(), "Unable to find localize_part service. Start vision_node first.");
      return;
    }

    // Create a request for the LocalizePart service call
    auto request = std::make_shared<myworkcell_core::srv::LocalizePart::Request>();
    request->base_frame = base_frame;
    RCLCPP_INFO_STREAM(get_logger(), "Requesting pose in base frame: " << base_frame);

    auto future = vision_client_->async_send_request(request);

    // Wait for response
    if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), future) != rclcpp::FutureReturnCode::SUCCESS)
    {
      // Never got response
      RCLCPP_ERROR(this->get_logger(), "Failed to receive LocalizePart service response");
      return;
    }

    // Check if response was successful
    auto response = future.get();
    if (! response->success)
    {
      RCLCPP_ERROR(this->get_logger(), "LocalizePart service failed");
      return;
    }

    RCLCPP_INFO(this->get_logger(), "Part Localized: x: %f, y: %f, z: %f",
        response->pose.position.x,
        response->pose.position.y,
        response->pose.position.z);
  }

private:
  // Planning components
  rclcpp::Client<myworkcell_core::srv::LocalizePart>::SharedPtr vision_client_;
};


int main(int argc, char **argv)
{
  // This must be called before anything else ROS-related
  rclcpp::init(argc, argv);

  // Create the ScanNPlan node
  auto app = std::make_shared<ScanNPlan>();
  std::string base_frame = app->get_parameter("base_frame").as_string();

  // Wait for the vision node to receive data
  rclcpp::sleep_for(std::chrono::seconds(2));

  app->start(base_frame);

  rclcpp::shutdown();
  return 0;
}