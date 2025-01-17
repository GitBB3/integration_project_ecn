﻿//integ_CTC is a Computed Torque Control node designed for INTEG project for Centrale Nantes Robotics
//Thibault LEBLANC & Julien COUPEAUX & Luca MIMOUNI & Baptiste LARDINOIT, Version 1.0.4, March 2024

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/node.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include "std_msgs/msg/float64_multi_array.hpp"
#include "std_msgs/msg/float64.hpp"
#include "yaml-cpp/yaml.h"
#include <Eigen/Core>
#include "test.cpp"

using namespace std::placeholders;

class ComputedTorqueControl : public rclcpp::Node {
public:

    ComputedTorqueControl()
        : Node("computed_torque_control") {

        // Initialisation des subscriptions (états réel et désiré), des publishers(commandes calculées), du timer


        desired_jointstate_subscriber_ = create_subscription<sensor_msgs::msg::JointState>(
                    "/scara/desired_joint_states",
                    10,
                    [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
                        desiredJointStateCallback(msg);
                    }); // A ajuster en fonction du nom des topics des gens qui font la trajectoire

        joint_state_subscriber_ = create_subscription<sensor_msgs::msg::JointState>(
                    "/scara/joint_states",
                    10,
                    [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
                        jointStateCallback(msg);
                    });

        kp_ = create_subscription<std_msgs::msg::Float64>(
                    "/kp",
                    10,
                    [this](const std_msgs::msg::Float64::SharedPtr msg) {
                    kpCallback(msg);
        });

        kd_ = create_subscription<std_msgs::msg::Float64>(
                    "/kd",
                    10,
                    [this](const std_msgs::msg::Float64::SharedPtr msg) {
                    kdCallback(msg);
                    });

        computed_torque_publisher_joint1_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
            "/scara/joint_1_cmd_effort", 10);

        computed_torque_publisher_joint2_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
            "/scara/joint_2_cmd_effort", 10);

        timer_ = create_wall_timer(std::chrono::milliseconds(50), std::bind(&ComputedTorqueControl::ComputeTorque, this));
}

private:
    //Définition des variables globales
    double kp;
    double kd;
    double real_pos1 ;
    double real_pos2 ;
    double real_vel1 ;
    double real_vel2 ;
    double pd1;
    double pd2;
    double vd1;
    double vd2;
    double ad1;
    double ad2;


    //Callback pour le Kp, utile dans le cas d'une optimisation par sliderpublisher par exemple ou pour un calcul externe du Kp
    void kpCallback(std_msgs::msg::Float64::SharedPtr k){
        kp=k->data;
    }

    //Callback pour le Kd, utile dans le cas d'une optimisation par sliderpublisher par exemple ou pour un calcul externe du Kd
    void kdCallback(std_msgs::msg::Float64::SharedPtr k){
        kd=k->data;
    }

    //Callback pour le l'état réel des liaisons
    void jointStateCallback(const sensor_msgs::msg::JointState::SharedPtr joint_state){

        real_pos1 = joint_state->position[1];
        real_pos2 = joint_state->position[2];
        real_vel1 = joint_state->velocity[1];
        real_vel2 = joint_state->velocity[2];
    }

    //Callback pour l'état désiré, la consigne publiée par exemple par les nodes des packages de trajectoire
    void desiredJointStateCallback(const sensor_msgs::msg::JointState::SharedPtr desired_joint_state) {

        pd1 = desired_joint_state->position[1];
        pd2 = desired_joint_state->position[2];
        vd1 = desired_joint_state->velocity[1];
        vd2 = desired_joint_state->velocity[2];
        ad1 = desired_joint_state->effort[1];
        ad2 = desired_joint_state->effort[2];
    }


    //Callback pour le calcul et la publication des couples calculés.
    void ComputeTorque(){

        //CallBack par timer

        //définition des variables pour le modèle du robot
        double g = 0;
        double m1 = 7.1 ;
        double m2 = 3.18 ;
        double l1 = 0.28 ;
        double mx1=l1*m1;

        double Ja=0.356567;
        double zz2=0.044085;
        double mx2=0.246391;
        double MXa=mx1+l1*m2;
        double Fv1=0.369366;
        double Fs2=0.409582;
        double Fv2=0.200203;

        //Définition du vecteur des paramètres identifiés, les paramètres vu comme sans influence dans une étude préalable sont omis.
        Eigen::Vector2d parameters;
        parameters(0)=Ja;
        parameters(1)=zz2;
        parameters(2)=mx2;
        parameters(3)=MXa;
        parameters(4)=Fv1;
        parameters(5)=Fs2;
        parameters(6)=Fv2;

        //définition des erreurs, sous régulateur proportionnel
        double e1=(pd1-real_pos1)*kp;
        double e2=(pd2-real_pos2)*kp;
        double ev1=(vd1-real_vel1)*kd;
        double ev2=(vd2-real_vel2)*kd;

        //définition de la matrice du modèle dynamique
        Eigen::MatrixXd model;
        model(0,0)=ad1;
        model(1,0)=0;
        model(0,1)=model(1,1)=ad1+ad2;
        model(0,2)=l1*(2*cos(e2)*ad1+cos(e2)*ad2-sin(e2)*ev2*ev2-2*sin(e2)*ev1*ev2)+g*cos(e1+e2);
        model(1,2)=l1*(cos(e2)*ad1+sin(e2)*ev1*ev1)+g*cos(e1+e2);
        model(0,3)=g*cos(e1);
        model(1,3)=0;
        model(0,4)=ev1;
        model(1,4)=0;
        model(0,5)=0;
        auto signe = [](double x) { return (x >= 0) ? 1 : -1; };
        model(1,5)=signe(ev2);
        model(0,6)=0;
        model(1,6)=ev2;


        // déifinition variables porteuses des couples résultats
        std_msgs::msg::Float64MultiArray computed_torque_msg_joint1;
        std_msgs::msg::Float64MultiArray computed_torque_msg_joint2;

        // Calcul des couples
        Eigen::Vector2d computed_torque;
        computed_torque=model*parameters;

        computed_torque_msg_joint1.data[0] = computed_torque(0,0);
        computed_torque_msg_joint2.data[0] = computed_torque(1,0);


        // Publication des couples
        computed_torque_publisher_joint1_->publish(computed_torque_msg_joint1);
        computed_torque_publisher_joint2_->publish(computed_torque_msg_joint2);
    }

    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_subscriber_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr desired_jointstate_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr kp_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr kd_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr computed_torque_publisher_joint1_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr computed_torque_publisher_joint2_;
    rclcpp::TimerBase::SharedPtr timer_;


    rclcpp::TimerBase::SharedPtr controlTimer_;

    std::vector<std::vector<double>> inverse_dynamics_model_;
};

int main(int argc, char *argv[]) {
        rclcpp::init(argc, argv);
        auto ctc{std::make_shared<ComputedTorqueControl>()};
        auto test{std::make_shared<crash_test>()};

        rclcpp::executors::SingleThreadedExecutor exec;
        exec.add_node(ctc);
        exec.add_node(test);
        exec.spin();

        rclcpp::shutdown();
        return 0;
}
