#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>
#include <riki_msgs/Velocities.h>
#include <geometry_msgs/Vector3.h>
#include <sensor_msgs/Imu.h>
#include <math.h>

#define RIKI_MSG
//#define ROS_MSG

double vel_x = 0.0;
double vel_dt = 0.0;
double imu_dt = 0.0;
double imu_z = 0.0;

ros::Time last_loop_time(0.0);
ros::Time last_vel_time(0.0);
ros::Time last_imu_time(0.0);


#ifdef ROS_MSG
void vel_callback( const geometry_msgs::Vector3Stamped& vel) {
  //callback every time the robot's linear velocity is received
  ros::Time current_time = ros::Time::now();

  vel_x = vel.vector.x;

  vel_dt = (current_time - last_vel_time).toSec();
  last_vel_time = current_time;

}
#endif

#ifdef RIKI_MSG
void vel_callback( const riki_msgs::Velocities& vel) {
	//callback every time the robot's linear velocity is received
	ros::Time current_time = ros::Time::now();

	vel_x = vel.linear_x;
	//vel_y = vel.linear_y;

	vel_dt = (current_time - last_vel_time).toSec();
	last_vel_time = current_time;
}
#endif

void imu_callback( const sensor_msgs::Imu& imu){
  //callback every time the robot's angular velocity is received
  ros::Time current_time = ros::Time::now();
  //this block is to filter out imu noise
  if(imu.angular_velocity.z > -0.08 && imu.angular_velocity.z < 0.08){
    imu_z = 0.00;
  }else{
    imu_z = imu.angular_velocity.z;
  }

  imu_dt = (current_time - last_imu_time).toSec();
  last_imu_time = current_time;
}

int main(int argc, char** argv)
{

  double angular_scale, linear_scale;

  ros::init(argc, argv, "base_controller");
  ros::NodeHandle n;
  ros::NodeHandle nh_private_("~");
  ros::Subscriber sub = n.subscribe("raw_vel", 50, vel_callback);
  ros::Subscriber imu_sub = n.subscribe("imu/data", 50, imu_callback);
  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>("odom", 50);
  tf::TransformBroadcaster odom_broadcaster;

  nh_private_.getParam("angular_scale", angular_scale);
  nh_private_.getParam("linear_scale", linear_scale);

  double rate = 10.0;
  double x_pos = 0.0;
  double y_pos = 0.0;
  double theta = 0.0;

  ros::Rate r(rate);
  while(n.ok()){
    ros::spinOnce();
    ros::Time current_time = ros::Time::now();

    //linear velocity is the linear velocity published from the Teensy board(vel_x)
    double linear_velocity = vel_x;
    //angular velocity is the rotation in Z from imu_filter_madgwick's output
    double angular_velocity = imu_z;

    //calculate angular displacement  θ = ω * t
    double delta_theta = angular_velocity * imu_dt * angular_scale; //radians
    double delta_x = (linear_velocity * cos(theta)) * vel_dt * linear_scale; //m
    double delta_y = (linear_velocity * sin(theta)) * vel_dt * linear_scale; //m

    //calculate current position of the robot
    x_pos += delta_x;
    y_pos += delta_y;
    theta += delta_theta;

    //calculate robot's heading in quarternion angle
    //ROS has a function to calculate yaw in quaternion angle
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(theta);

    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";
    //robot's position in x,y, and z
    odom_trans.transform.translation.x = x_pos;
    odom_trans.transform.translation.y = y_pos;
    odom_trans.transform.translation.z = 0.0;
    //robot's heading in quaternion
    odom_trans.transform.rotation = odom_quat;
    odom_trans.header.stamp = current_time;
    //publish robot's tf using odom_trans object
    odom_broadcaster.sendTransform(odom_trans);

    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";
    //robot's position in x,y, and z
    odom.pose.pose.position.x = x_pos;
    odom.pose.pose.position.y = y_pos;
    odom.pose.pose.position.z = 0.0;
    //robot's heading in quaternion
    odom.pose.pose.orientation = odom_quat;

    odom.child_frame_id = "base_link";
    //linear speed from encoders
    odom.twist.twist.linear.x = linear_velocity;
    odom.twist.twist.linear.y = 0.0;
    odom.twist.twist.linear.z = 0.0;

    odom.twist.twist.angular.x = 0.0;
    odom.twist.twist.angular.y = 0.0;
    //angular speed from IMU
    odom.twist.twist.angular.z = imu_z;

    //TODO: include covariance matrix here

    odom_pub.publish(odom);

    last_loop_time = current_time;
    r.sleep();
  }
}
