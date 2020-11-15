#!/bin/bash

echo "To start up the basic robot system!"

roslaunch rikirobot bringup.launch &
#pid="$pid $!"

sleep 10s

echo "To start up the LIDAR and robot base!"
roslaunch rikirobot ydlidar_slam.launch &
#pid="$pid $!"

#sleep 10s 

echo "To teleoperate the robot!"
#rosrun teleop_twist_keyboard teleop_twist_keyboard.py 

#sleep 10s 

echo "To start up the RPI camera!"

roslaunch raspicam_node camerav2_1280x960.launch &
#pid="$pid $!"

trap "echo Killing all processes.; kill -2 TERM $pid; exit" SIGINT SIGTERM

sleep 24h
