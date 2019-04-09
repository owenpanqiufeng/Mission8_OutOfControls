#include <ros/ros.h>
#include "std_msgs/Float64.h"
#include "std_msgs/UInt16.h"
#include <cmath>
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/PoseStamped.h>
#include <iostream>
#include <math.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/PositionTarget.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <nav_msgs/Odometry.h>
#include <ros/duration.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <ros/time.h>
#include "control_functions.hpp"

using namespace std;

// Set global variables
mavros_msgs::State current_state;
geometry_msgs::PoseStamped current_pose;
geometry_msgs::PoseStamped pose;
std_msgs::Float64 current_heading;
float GYM_OFFSET;
std_msgs::String qr;
bool moving = false;
std_msgs::String msg;
vector<float> c;
std::vector<std_msgs::UInt16> plist;
std_msgs::UInt16 p;

void voice_cb(const std_msgs::String::ConstPtr& voice)
{
    std_msgs::String v = *voice;
    msg = v;
}

void qr_cb(const std_msgs::String::ConstPtr& codes){

    qr = *codes;
}

void point_cb(const std_msgs::UInt16::ConstPtr& numPoints){
    std_msgs::UInt16 nPoints = *numPoints;
    int max = 0;
    int min = 0;
    //dropping highest and lowest point
    if(plist.size() == 5){
    	for(int i = 1; i < 5; i++){
    		if(plist[max].data < plist[i].data){
    			max = i;
    		}
    		else if(plist[min].data > plist[i].data){
    			min = i;
    		}
    	}
    	plist[max].data = 0;
    	plist[min].data = 0;
        std_msgs::UInt16 sum;
        sum.data = 0;
        for(int i = 0; i < 5; i++){
            sum.data+=plist[i].data;
            plist.pop_back();
        }
        p.data = sum.data/3;
    }
    plist.push_back(nPoints);
}

float deltaZ(const std_msgs::UInt16 n){
	return .0000000005*[](float x){return x * x * x;}((.2*(n.data-6400)));
}


//to be changed: function will accept a column of a 2D vector
void flyTo(float x, float y, float z){
	ROS_INFO("1");
	set_destination(x,y,z, 0);
	float tol = .2;
	ROS_INFO("2");
	//to be added: obstacle avoidance
	ros::Time start = ros::Time::now();
	while(!(check_waypoint_reached(tol)) && (ros::Time::now() - start).toSec() < 60){
		ros::spinOnce();
		ros::Duration(0.5).sleep();
	}

    ROS_INFO("Done moving forward.");

}

//to be changed: function will accept a column of a 2D vector
void QRcode(float x, float y, float z){
	float tol = .2;
	float r = .1;
    float t = 0;
    
   	ros::Time start = ros::Time::now();
    while(qr.data == "null" && (ros::Time::now() - start).toSec() < 60){
    	//if(z + deltaZ(p) >= .5 && z + deltaZ(p) <= 1){
            set_destination(x + r*cos(t), y + r*sin(t), z,0);// + deltaZ(p), 0);
        //}
        ros::spinOnce();
        ros::Duration(0.1).sleep();
        t+=.1;
    }
    ROS_INFO("Got QR Code.");
}



int main(int argc, char** argv)
{
	qr.data = "null";
	msg.data = "nothin";
    ros::init(argc, argv, "outtaControls");
    ros::NodeHandle nh;

    // the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(20.0);
    init_publisher_subscriber(nh);
    ros::Subscriber voiceRecognition = nh.subscribe<std_msgs::String>("Android", 10, voice_cb);
    ros::Subscriber QR = nh.subscribe<std_msgs::String>("CV", 10, qr_cb);
    ros::Subscriber points = nh.subscribe<std_msgs::UInt16>("Points", 5, point_cb);

    wait4start();

    initialize_local_frame();

    wait4connect();

    ros::spinOnce();

    while(ros::ok()){
    	ros::spinOnce();
    	//add obstacle avoidance here too 
    	if(msg.data == "takeoff"){
    		takeoff(1);
    	}
    	else if(msg.data == "qr code"){
    		c.push_back(0); //This will be later refactored
			c.push_back(5); //to store the locations of 
			c.push_back(1); //all 4 qr codes
			flyTo(c[0],c[1], c[2]);
			QRcode(c[0],c[1], c[2]);
		}
		else if(msg.data == "land"){	
			flyTo(0,0,1);
			land();
    	}
    	msg.data = "nuthin";
    }
    
    return 0;
}