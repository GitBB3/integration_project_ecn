


#include <rclcpp/rclcpp.hpp>
#include <rclcpp/node.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <cmath>

class Desired_joint_states : public rclcpp::Node {
public:
    Desired_joint_states()
        : Node("desired_joint_states") {

        trajectory = this->create_publisher<sensor_msgs::msg::JointState>(
            "/scara/desired_joint_states", 10);

        controlTimer_ = this->create_wall_timer(
            std::chrono::milliseconds(10), std::bind(&Desired_joint_states::controlCallback, this));
    }

private:
    void controlCallback() {
        double t = this->get_clock()->now().seconds();
        auto joint_state_msg = std::make_unique<sensor_msgs::msg::JointState>();
        joint_state_msg->position.resize(3);
        joint_state_msg->velocity.resize(3);

        joint_state_msg->position[0] = (M_PI / 4) * std::sin(t * 0.1);
        joint_state_msg->position[1] = -(M_PI / 4) * std::sin(t * 0.1);
        joint_state_msg->velocity[0] = (M_PI / 4) * 0.1 * std::cos(t * 0.1);
        joint_state_msg->velocity[1] = -(M_PI / 4) * 0.1 * std::cos(t * 0.1);

        trajectory->publish(std::move(joint_state_msg));
    }

    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr trajectory;
    rclcpp::TimerBase::SharedPtr controlTimer_;
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Desired_joint_states>());
    rclcpp::shutdown();
    return 0;
}
