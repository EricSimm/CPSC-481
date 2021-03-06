#include <ros/ros.h>
#include <turtlesim/Spawn.h>
#include <turtlesim/Kill.h>
#include <turtlesim/Pose.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <ros/master.h>
#include <boost/algorithm/string.hpp>
#include <geometry_msgs/Twist.h>
#include <algorithm>
#include <vector>
//#include "std_msgs/String.h"
#include "std_msgs/Int32MultiArray.h"

using namespace std;









ros::Publisher velocity_publisher;
ros::Subscriber pose_subscriber, t1Pose, t2Pose, t3pose, x1Pose, x2Pose, x3Pose, x4Pose;
turtlesim::Pose turtlesim_pose;
turtlesim::Pose T1;
turtlesim::Pose T2;
turtlesim::Pose T3;
turtlesim::Pose X1;
turtlesim::Pose X2;
turtlesim::Pose X3;
turtlesim::Pose X4;

const double PI = 3.14159265;

void move(double speed, double distance, bool isForward);
void rotate(double angular_speed, double angle, bool clockwise);
double degreestoradians(double angle_in_degrees);
double radianstodegrees(double radians);
double getDistance(double x1, double y1, double x2, double y2);
double getDistance(turtlesim::Pose t1, turtlesim::Pose t2);
double getDistance(double x1, double y1, turtlesim::Pose turtle);
void checkPos(double& x, double& y, int count);
bool moveGoal(turtlesim::Pose goal_pose, double distance_tolerance, ros::NodeHandle nh);
bool moveGoal(turtlesim::Pose goal_turtle, string name, float goal_x, float goal_y, double distance_tolerance, ros::NodeHandle nh);
void poseCallback(const turtlesim::Pose::ConstPtr & pose_message);
void updateTTurtles(ros::NodeHandle nh);
void updateXTurtles(ros::NodeHandle nh);
double orientate(double desired_radians);
bool moveDistance(double distance, ros::NodeHandle nh, string name, turtlesim::Pose goal_turtle, double tolerance);
//bool calculateNewDistance(double distance, Pattern* pattern);



//figures out which point the turtle will end up after a set distance, considers borders
bool moveDistance(double distance, ros::NodeHandle nh, string name, turtlesim::Pose goal_turtle, double tolerance)
{
	double xPos = turtlesim_pose.x + distance*cos(turtlesim_pose.theta), yPos = turtlesim_pose.y + distance*sin(turtlesim_pose.theta);
	double overflow;
	//cout << "distance: " << distance << endl;
	while(xPos > 11 || xPos < 0 || yPos < 0 || yPos > 11)
	{
		if(xPos > 11)
		{
			overflow = xPos -11;
			xPos = 11 - overflow;
		}
		if(xPos < 0)
		{
			overflow= abs(xPos);
			xPos = overflow;
		}
		if(yPos > 11)
		{
			overflow = yPos -11;
			yPos = 11 - overflow;
		}
		if (yPos < 0)
		{
			overflow = abs(yPos);
			yPos = overflow;
		}
	}
	cout << "xPos: " << xPos << " yPos: " << yPos << " distance: " << distance << endl;
	return moveGoal(goal_turtle, name, xPos, yPos, tolerance, nh);
}

//used to reorientate to a certain angle based off starting cordinates.
double orientate(double desired_radians)
{
	while(desired_radians > 2*PI)
	{
		desired_radians = desired_radians - (2*PI);
	}
	double final_radians = desired_radians - turtlesim_pose.theta;
	bool clockwise = ((final_radians <0)?true:false);
	rotate(1, abs(final_radians), clockwise);
	
}

//this moves the turtle a set distance
void move(double speed, double distance, bool isForward)
{
	geometry_msgs::Twist vel_msg;

	if(isForward)
		vel_msg.linear.x=abs(speed);
	else
		vel_msg.linear.x=-abs(speed);

	vel_msg.linear.y = 0;
	vel_msg.linear.z = 0;

	vel_msg.angular.x = 0;
	vel_msg.angular.z = 0;
	vel_msg.angular.y = 0;

	double t0 = ros::Time::now().toSec();
	double current_distance = 0;
	ros::Rate loop_rate(10000);

	do
	{
		velocity_publisher.publish(vel_msg);
		double t1 = ros::Time::now().toSec();
		current_distance = speed * (t1-t0);
		ros::spinOnce();
		loop_rate.sleep();
	}
	while(current_distance < distance);

	vel_msg.linear.x = 0;
	velocity_publisher.publish(vel_msg);

}

//turn a desired angle, angle calculations based off speed*time
void rotate(double angular_speed, double angle, bool clockwise)
{

	//initate position/speed/direction
	geometry_msgs::Twist vel_msg;

	vel_msg.linear.y = 0;
	vel_msg.linear.x = 0;
	vel_msg.linear.z = 0;

	vel_msg.angular.x = 0;
	vel_msg.angular.y = 0;

	if (clockwise)
		vel_msg.angular.z = -abs(angular_speed);
	else
		vel_msg.angular.z = abs(angular_speed);
	//accept negative angle
	if (angle < 0)
	{
		angle = -angle;
		vel_msg.angular.z = -vel_msg.angular.z;
	}

	double current_angle = 0.0;
	ros::Rate loop_rate(10000000);
	double t0 = ros::Time::now().toSec();

	//loop to we turn our desired angle, calculations based off time/speed
	while(current_angle<angle)
	{
		velocity_publisher.publish(vel_msg);
		double t1 = ros::Time::now().toSec();
		current_angle = angular_speed *(t1-t0);
		ros::spinOnce();
		loop_rate.sleep();
	}

	//stop moving
	vel_msg.angular.z = 0;
	velocity_publisher.publish(vel_msg);
}


//converts degrees to radians
double degreestoradians(double angle_in_degrees){
	return angle_in_degrees *PI /180.0;
}

double radianstodegrees(double radians)
{
	return radians/(PI/180.0);
}

//these get the distance between two points
double getDistance(double x1, double y1, double x2, double y2)
{
	return sqrt(pow((x1-x2),2)+pow((y1-y2),2));
}

double getDistance(turtlesim::Pose t1, turtlesim::Pose t2)
{
	return sqrt(pow((t1.x-t2.x),2)+pow((t1.y-t2.y),2));
}

double getDistance(double x1, double y1, turtlesim::Pose turtle)
{
	return sqrt(pow((x1-turtle.x),2)+pow((y1-turtle.y),2));
}


//finds the future x and y values based on our orientation
void checkPos(double& x, double& y, int count)
{

	y = .2*sin(turtlesim_pose.theta + (0.0872665 * count)) + turtlesim_pose.y;
	x = .2*cos(turtlesim_pose.theta + (0.0872665 * count)) + turtlesim_pose.x;

}

//finds the shortest distance to all villain
double villainDist(double x, double y)
{
  double distX1 = getDistance(x, y, X1);
  double distX2 = getDistance(x, y, X2);
  double distX3 = getDistance(x, y, X3);

  double m1 = min(distX1, distX2);
  return min(m1, distX3);
}



//moves turtle1 towards the goal_pose within the distance tolerance
//also, avoids villain turtles and the borders
bool moveGoal(turtlesim::Pose goal_pose, double distance_tolerance, ros::NodeHandle nh)
{
	geometry_msgs::Twist vel_msg;

	ros::Rate loop_rate(400);
	do{
		//double Kp = 1.0;
		//double Ki = 0.02;

		double x = turtlesim_pose.x;
		double y = turtlesim_pose.y;
		//predict where we will move in the next step
		checkPos(x, y, 0);

		//see how close future position is to the closest villain
		double distV = villainDist(x, y);
		//check to see if we have been captured
		if (villainDist(turtlesim_pose.x, turtlesim_pose.y) <= .5)
		{
			ROS_INFO_STREAM("Too close to villain, captured" );
			return false;
		}
		//check all villain turtles and border
		//move away if we are going to hit them
		if(distV <= 0.5 ||x > 11 || y > 11 || x < 0 || y < 0)
		{
			vel_msg.linear.x = 0;
			vel_msg.angular.z = 0;
			velocity_publisher.publish(vel_msg);
			double futureX, futureY;
			int factorToTurn = 9;
			while(1){

				checkPos(futureX, futureY, factorToTurn);

				if (futureX < 11 && futureY < 11 && futureX > 0 && futureY > 0 && villainDist(futureX,futureY) > 0.5)
					break;
				factorToTurn += 9;
			}
			rotate(4, 0.0872665*factorToTurn, true);
			move(1, 0.3, true);
		}
		//continue on if we don't need to avoid anything
		if(distV > .5)
		{
			vel_msg.linear.x = 1;
			vel_msg.linear.y = 0;
			vel_msg.linear.z = 0;

			vel_msg.angular.x = 0;
			vel_msg.angular.y = 0;
			vel_msg.angular.z = 6*(atan2(goal_pose.y-turtlesim_pose.y, goal_pose.x-turtlesim_pose.x)-turtlesim_pose.theta);
		}
		//currently not used, but we afraid to delete it
		else
		{

			vel_msg.linear.x = 0;
			vel_msg.angular.z = 0;
			velocity_publisher.publish(vel_msg);

			double x = 0.0;
			double y = 0.0;
			int count = 1;
			bool isNeg = false;
			while(distV <= .5 || x > 11 || y > 11 || x < 0 || y < 0)
			{
				isNeg = false;
				checkPos(x,y, count);
				distV = villainDist(x, y);

				if(distV > .5 && x < 11 && y < 11 && x > 0 && y > 0)
					break;
				isNeg = true;
				checkPos(x, y, -count);
				distV = villainDist(x, y);
				count++;
			}
			vel_msg.linear.x = 1;
			vel_msg.linear.y = 0;
			vel_msg.linear.z = 0;

			vel_msg.angular.x = 0;
			vel_msg.angular.y = 0;
			double theta = 0.0;
			if(isNeg)
				theta = turtlesim_pose.theta + (0.0872665 * -count);
			else
				theta = turtlesim_pose.theta + (0.0872665 * count);
			vel_msg.angular.z = 6*(atan2(y-turtlesim_pose.y, x-turtlesim_pose.x)-theta);
			/*double theta = 0.0;
			if(isNeg)
				theta = turtlesim_pose.theta + (0.174533 * -count);
			else
				theta = turtlesim_pose.theta + (0.174533 * count);
			rotate(4, theta, 1);
			move(4, 0.45, true);*/

		}
		//publish our message
		velocity_publisher.publish(vel_msg);

		ros::spinOnce();
		loop_rate.sleep();
	}while(getDistance(turtlesim_pose.x, turtlesim_pose.y, goal_pose.x, goal_pose.y)>distance_tolerance);
	//once we get within our distance_tolerance, we get out of the loop
	//stop the turtle
	vel_msg.linear.x = 0;
	vel_msg.angular.z = 0;
	velocity_publisher.publish(vel_msg);
	return true;
}

bool captureTarget(string name, turtlesim::Pose goal_pose, double distance_tolerance,ros::NodeHandle nh)
{
	//cout << "distance is" << getDistance(turtlesim_pose.x, turtlesim_pose.y, goal_pose.x, goal_pose.y) << endl;
	if(getDistance(turtlesim_pose.x, turtlesim_pose.y, goal_pose.x, goal_pose.y)<=distance_tolerance)
	{
		ros::ServiceClient kClient = nh.serviceClient<turtlesim::Kill>("kill");
		turtlesim::Kill::Request reqk;
    	turtlesim::Kill::Response respk;
    	reqk.name = name;
    	if (!kClient.call(reqk, respk))
       		ROS_ERROR_STREAM("Error: Failed to kill " << reqk.name.c_str() << "\n");
    	else
    	   	ROS_INFO_STREAM("turtle captured");
		return true;
	}
	return false;
}

//same as move goal but moves to goal position rather than goal turtle, if get within turtle capture and stop
//update villain turtles
//returns true if captured target turtle
//else returns false
bool moveGoal(turtlesim::Pose goal_turtle, string name, float goal_x, float goal_y, double distance_tolerance, ros::NodeHandle nh)
{
	updateTTurtles(nh);
	updateXTurtles(nh);
	if(name == "T1")
		goal_turtle = T1;
	else if (name == "T2")
		goal_turtle = T2;
	else
		goal_turtle = T3;
	geometry_msgs::Twist vel_msg;

	ros::Rate loop_rate(400);

	do{
		//double Kp = 1.0;
		//double Ki = 0.02;

		double x = turtlesim_pose.x;
		double y = turtlesim_pose.y;
		//predict where we will move in the next step
		checkPos(x, y, 0);

		//see how close future position is to the closest villain
		double distV = villainDist(x, y);
		// update: capture problem
		//updateTTurtles(nh);
		//check to see if we have been captured
		if(captureTarget(name, goal_turtle, 0.5,nh ) == true)
		{
			vel_msg.linear.x = 0;
			vel_msg.angular.z = 0;
			velocity_publisher.publish(vel_msg);
			return true;
		}
		//check all villain turtles and border
		//move away if we are going to hit them
		if(distV <= 0.5 ||x > 11 || y > 11 || x < 0 || y < 0)
		{
			vel_msg.linear.x = 0;
			vel_msg.angular.z = 0;
			velocity_publisher.publish(vel_msg);
			double futureX, futureY;
			int factorToTurn = 9;
			while(1){
				updateXTurtles(nh);
				checkPos(futureX, futureY, factorToTurn);

				if (futureX < 11 && futureY < 11 && futureX > 0 && futureY > 0 && villainDist(futureX,futureY) > 0.5)
					break;
				factorToTurn += 9;
			}
			rotate(4, 0.0872665*factorToTurn, true);
			move(1, 0.3, true);
		}
		//continue on if we don't need to avoid anything
		if(distV > .5)
		{
			vel_msg.linear.x = 1;
			vel_msg.linear.y = 0;
			vel_msg.linear.z = 0;

			vel_msg.angular.x = 0;
			vel_msg.angular.y = 0;
			vel_msg.angular.z = 6*(atan2(goal_y-turtlesim_pose.y, goal_x-turtlesim_pose.x)-turtlesim_pose.theta);
		}
		//currently not used, but we afraid to delete it
		else
		{

			vel_msg.linear.x = 0;
			vel_msg.angular.z = 0;
			velocity_publisher.publish(vel_msg);

			double x = 0.0;
			double y = 0.0;
			int count = 1;
			bool isNeg = false;
			while(distV <= .5 || x > 11 || y > 11 || x < 0 || y < 0)
			{
				isNeg = false;
				checkPos(x,y, count);
				distV = villainDist(x, y);

				if(distV > .5 && x < 11 && y < 11 && x > 0 && y > 0)
					break;
				isNeg = true;
				checkPos(x, y, -count);
				distV = villainDist(x, y);
				count++;
			}
			vel_msg.linear.x = 1;
			vel_msg.linear.y = 0;
			vel_msg.linear.z = 0;

			vel_msg.angular.x = 0;
			vel_msg.angular.y = 0;
			double theta = 0.0;
			if(isNeg)
				theta = turtlesim_pose.theta + (0.0872665 * -count);
			else
				theta = turtlesim_pose.theta + (0.0872665 * count);
			vel_msg.angular.z = 6*(atan2(y-turtlesim_pose.y, x-turtlesim_pose.x)-theta);
			/*double theta = 0.0;
			if(isNeg)
				theta = turtlesim_pose.theta + (0.174533 * -count);
			else
				theta = turtlesim_pose.theta + (0.174533 * count);
			rotate(4, theta, 1);
			move(4, 0.45, true);*/

		}
		//publish our message
		velocity_publisher.publish(vel_msg);

		ros::spinOnce();
		loop_rate.sleep();
	}while(getDistance(turtlesim_pose.x, turtlesim_pose.y, goal_x, goal_y)>distance_tolerance);
	//once we get within our distance_tolerance, we get out of the loop
	//stop the turtle
	vel_msg.linear.x = 0;
	vel_msg.angular.z = 0;
	velocity_publisher.publish(vel_msg);
	return false;

}


void poseCallback(const turtlesim::Pose::ConstPtr & pose_message){
	turtlesim_pose.x=pose_message->x;
	turtlesim_pose.y=pose_message->y;
	turtlesim_pose.theta=pose_message->theta;
}

void poseCallbackT1(const turtlesim::Pose::ConstPtr & pose_message){
	T1.x=pose_message->x;
	T1.y=pose_message->y;
	T1.theta=pose_message->theta;
	T1.linear_velocity = pose_message->linear_velocity;
	T1.angular_velocity=pose_message->angular_velocity;
}

void poseCallbackT2(const turtlesim::Pose::ConstPtr & pose_message){
	T2.x=pose_message->x;
	T2.y=pose_message->y;
	T2.theta=pose_message->theta;
	T2.angular_velocity=pose_message->angular_velocity;
	T2.linear_velocity = pose_message->linear_velocity;
}

void poseCallbackT3(const turtlesim::Pose::ConstPtr & pose_message){
	T3.x=pose_message->x;
	T3.y=pose_message->y;
	T3.theta=pose_message->theta;
	T3.angular_velocity=pose_message->angular_velocity;
	T3.linear_velocity = pose_message->linear_velocity;
}

void poseCallbackX1(const turtlesim::Pose::ConstPtr & pose_message){
	X1.x=pose_message->x;
	X1.y=pose_message->y;
	X1.theta=pose_message->theta;
}

void poseCallbackX2(const turtlesim::Pose::ConstPtr & pose_message){
	X2.x=pose_message->x;
	X2.y=pose_message->y;
	X2.theta=pose_message->theta;
}

void poseCallbackX3(const turtlesim::Pose::ConstPtr & pose_message){
	X3.x=pose_message->x;
	X3.y=pose_message->y;
	X3.theta=pose_message->theta;
}

void poseCallbackX4(const turtlesim::Pose::ConstPtr & pose_message){
	X4.x=pose_message->x;
	X4.y=pose_message->y;
	X4.theta=pose_message->theta;
}


void updateTurtles(ros::NodeHandle nh)
{
  ros::Rate loopT(2);
  t1Pose = nh.subscribe("/T1/pose", 10, poseCallbackT1);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  t2Pose = nh.subscribe("/T2/pose", 10, poseCallbackT2);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  t3pose = nh.subscribe("/T3/pose", 10, poseCallbackT3);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  x1Pose = nh.subscribe("/X1/pose", 10, poseCallbackX1);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  x2Pose = nh.subscribe("/X2/pose", 10, poseCallbackX2);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  x3Pose = nh.subscribe("/X3/pose", 10, poseCallbackX3);
  ros::spinOnce();
  ros::spinOnce();
  ros::Duration(.5).sleep();

  /*ros::spinOnce();
  loopT.sleep();

  ros::spinOnce();
  loopT.sleep();*/

}


//update all the positions of target turtles
void updateTTurtles(ros::NodeHandle nh)
{
	//cout << "start update" << endl;
  ros::Rate loopT(2);
  t1Pose = nh.subscribe("/T1/pose", 10, poseCallbackT1);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  t2Pose = nh.subscribe("/T2/pose", 10, poseCallbackT2);
  ros::spinOnce();
  ros::Duration(.5).sleep();
  t3pose = nh.subscribe("/T3/pose", 10, poseCallbackT3);
  ros::spinOnce();
  ros::Duration(.5).sleep();
	//cout << "end update" << endl;

}

//update all the positions of villain turtles
void updateXTurtles(ros::NodeHandle nh)
{
	ros::Rate loopT(2);
	x1Pose = nh.subscribe("/X1/pose", 10, poseCallbackX1); 
	ros::spinOnce();
  	ros::Duration(.5).sleep();
	x2Pose = nh.subscribe("/X2/pose", 10, poseCallbackX2);
	ros::spinOnce();
  	ros::Duration(.5).sleep();
	x3Pose = nh.subscribe("/X3/pose", 10, poseCallbackX3);
	ros::spinOnce();
  	ros::Duration(.5).sleep();
}




class Pattern{
public:
	turtlesim::Pose turtlePose;
	float min_x, min_y, max_x, max_y, initial_x, initial_y;
	float theta;
	float second_x, second_y;
	float rotation;
	double dist;
	float speed;
	float initialTheta;
	string type;
	Pattern(turtlesim::Pose turtle){
		turtlePose = turtle;
		min_x = turtle.x;
		min_y = turtle.y;
		max_x = turtle.x;
		max_y = turtle.y;
		second_x = turtle.x;
		second_y = turtle.y;
		initial_x = turtle.x;
		initial_y = turtle.y;
		theta = turtle.theta;
		dist = 0.0;
		speed = turtle.linear_velocity;
		rotation = 0;
	}
};

void findMinMax(Pattern* pattern, turtlesim::Pose turtle){
	if(pattern->min_x > turtle.x){
		pattern->min_x = turtle.x;
	}
	if(pattern->min_y > turtle.y){
		pattern->min_y = turtle.y;
		//cout << "min_y" << pattern->min_y << endl;
	}
	if(pattern->max_x < turtle.x){
		pattern->max_x = turtle.x;
	}
	if(pattern->max_y < turtle.y){
		pattern->max_y = turtle.y;
		//cout << "max_y" << pattern->max_y << endl;
	}
}

void assignPattern(Pattern* pattern){
	if(pattern->max_x - pattern->min_x < 0.1 ){
		pattern->type = "vertical";
	}else{
		pattern->type = "horizontal";
	}

	if(pattern->rotation != 0){
		pattern->type = "rotation";
	}

}

void jPattern(ros::NodeHandle nh, Pattern* pattern1, Pattern* pattern2, Pattern* pattern3)
{
	bool findAngular1 = false, findAngular2 = false, findAngular3 = false;
	bool findLinear1 = false, findLinear2 = false, findLinear3 = false;
	bool angularChanged1 = false, angularChanged2 = false, angularChanged3 = false;
	bool linearChanged1 = false, linearChanged2 = false, linearChanged3 = false;
	bool find1 = false, find2 = false, find3 = false;
	double t0, t1;

	//need a while loop here
	while(!find1 || !find2 || !find3)
	{
		updateTTurtles(nh);

		if(T1.linear_velocity != 0 || T1.angular_velocity != 0)
		{
			cout << "checking T1" << endl;
			while(!findAngular1 || !findLinear1)
			{
				
				updateTTurtles(nh);
				findMinMax(pattern1, T1);
				if(T2.linear_velocity != 0 || T3.linear_velocity != 0)
				{
					findAngular1 = true;
				}

				if(T1.linear_velocity != 0)
				{
					if(!linearChanged1)
					{
						t0 = ros::Time::now().toSec();
						pattern1->speed = T1.linear_velocity;
						cout << "pattern1->speed: " << pattern1->speed << endl;
					}
					linearChanged1 = true;
				}

				if(T1.linear_velocity == 0 && linearChanged1)
				{
					if(!findLinear1)
					{
						t1 = ros::Time::now().toSec();
						pattern1->dist = abs((t1-t0) * pattern1->speed);
						cout << "pattern1->dist: " << pattern1->dist << endl;
					}
					findLinear1 = true;
				}

				if(T1.angular_velocity != 0)
				{
					if(!angularChanged1)
					{
						cout << "second_x: " << T1.x << " second_y: " << T1.y << endl;
						pattern1->second_x = T1.x;
						pattern1->second_y = T1.y;
					}
					angularChanged1 = true;
				}

				if(T1.angular_velocity == 0 && angularChanged1)
				{
					float newTheta = T3.theta;

					findAngular1 = true;

					pattern1->rotation = newTheta - pattern1->theta;

					cout << "pattern2->rotation: " << pattern2->rotation << endl;

				}

			}
			if(findAngular1 && findLinear1)
			{
				cout << "pattern1->min_y: " << pattern1->min_y << endl; 
				find1 = true;
			}
		}

		if(T2.linear_velocity != 0 || T2.angular_velocity != 0)
		{
			
			cout << "checking T2" << endl;
			while(!findAngular2 || !findLinear2)
			{
				updateTTurtles(nh);
				findMinMax(pattern2, T2);
				if(T1.linear_velocity != 0 || T3.linear_velocity != 0)
				{
					findAngular2 = true;
				}

				if(T2.linear_velocity != 0)
				{
					if(!linearChanged2)
					{
						t0 = ros::Time::now().toSec();
						pattern2->speed = T2.linear_velocity;
						cout << "pattern2->speed: " << T2.linear_velocity << endl;

					}
					linearChanged2 = true;	

				}

				if(T2.linear_velocity == 0 && linearChanged2)
				{
					if(!findLinear2)
					{
						t1 = ros::Time::now().toSec();
						pattern2->dist = abs((t1-t0) * pattern2->speed);
						cout << "pattern2->dist: " << pattern2->dist << endl;
					}
					findLinear2 = true;

				}
				
				if(T2.angular_velocity != 0)
				{
					if(!angularChanged2)
					{
						cout << "second_x: " << T2.x << " second_y: " << T2.y << endl;
						pattern3->second_x = T2.x;
						pattern3->second_y = T2.y;
					}
					angularChanged2 = true;

				}

				if(T2.angular_velocity == 0 && angularChanged2)
				{
					float newTheta = T2.theta;

					findAngular2 = true;

					pattern2->rotation = newTheta - pattern2->theta;
					cout << "pattern2->rotation: " << pattern2->rotation << endl;

				}

			}
			if(findAngular2 && findLinear2)
				find2 = true;
		}

		if(T3.linear_velocity != 0 || T3.angular_velocity != 0)
		{
			cout << "checking T3" << endl;
			while(!findAngular3 || !findLinear3)
			{
				updateTTurtles(nh);
				findMinMax(pattern3, T3);
				if(T2.linear_velocity != 0 || T1.linear_velocity != 0)
				{
					findAngular3 == true;
				}

				if(T3.linear_velocity != 0)
				{
					if(!linearChanged3)
					{
						t0 = ros::Time::now().toSec();
						pattern3->speed = T3.linear_velocity;
						cout << "pattern3->speed: " << T3.linear_velocity << endl;

					}
					linearChanged3 = true;	

				}

				if(T3.linear_velocity == 0 && linearChanged3)
				{
					if(!findLinear3)
					{
						t1 = ros::Time::now().toSec();
						pattern3->dist = abs((t1-t0) * pattern3->speed);
						cout << "pattern3->dist: " << pattern3->dist << endl;
					}
					findLinear3 = true;

				}

				if(T3.angular_velocity != 0)
				{
					if(!angularChanged3)
					{
						cout << "second_x: " << T3.x << " second_y: " << T3.y << endl;
						pattern3->second_x = T3.x;
						pattern3->second_y = T3.y;
					}
					angularChanged3 = true;

				}

				if(T3.angular_velocity == 0 && angularChanged3)
				{
					float newTheta = T3.theta;

					findAngular3 = true;

					pattern3->rotation = newTheta - pattern3->theta;
					cout << "pattern3->rotation: " << pattern3->rotation << endl;

				}

			}
			if(findAngular3 && findLinear3)
				find3 = true;

		}
	}
	assignPattern(pattern1);
	cout << "T1 type: " << pattern1->type << endl;
	assignPattern(pattern2);
	cout << "T2 type: " << pattern2->type << endl;
	assignPattern(pattern3);
	cout << "T3 type: " << pattern3->type << endl;
}

//learns pattern based on time moving and figures out the distance each turtle moved
void learnPat(ros::NodeHandle nh, Pattern* pattern1, Pattern* pattern2, Pattern* pattern3){
	bool T1Pattern = false, T2Pattern = false, T3Pattern = false, T3PatternTheta=false, t1Finisher = true, t2Finisher = true, t3Finisher = true;
	bool T3MoveAgain = false;
	bool thetaFinisher = false;
	cout << "start sleeping" << endl;
	ros::Duration duration(1./24.);
	float pastT1x = T1.x, pastT1y = T1.y, pastT2x = T2.x, pastT2y = T2.y, pastT3x = T3.x, pastT3y = T3.y, pastT3theta = T3.theta;
	float originalTheta = T3.theta;
	double t0,t1;
	double distance;

	while(!T1Pattern|| !T2Pattern || !T3Pattern)//|| !T3PatternTheta)
	{
		updateTTurtles(nh);
		while(T1.linear_velocity != 0){
			updateTTurtles(nh);
			findMinMax(pattern1, T1);
			if(!T1Pattern)
			{
				pattern1->speed = T1.linear_velocity;
				t0 = ros::Time::now().toSec();
			}
			T1Pattern = true;
		}
		if(T1Pattern && t1Finisher)
		{
			t1 = ros::Time::now().toSec();
			pattern1->dist = abs((t1-t0) * pattern1->speed);
			t1Finisher = false;
		}
		while(T2.linear_velocity != 0)
		{
			updateTTurtles(nh);
			findMinMax(pattern2, T2);
			if(!T2Pattern)
			{
				pattern2->speed = T2.linear_velocity;
				t0 = ros::Time::now().toSec();
			}
			T2Pattern = true;
		}
		if(T2Pattern && t2Finisher)
		{
			t1 = ros::Time::now().toSec();
			pattern2->dist = abs((t1-t0) * pattern2->speed);
			t2Finisher = false;
		}
		while(!T3Pattern)
		{
			updateTTurtles(nh);
			cout << "T3.linear_velocity = " << T3.linear_velocity << endl; 
			while(T3.linear_velocity != 0 && !T3MoveAgain)
			{
				updateTTurtles(nh);
				findMinMax(pattern3, T3);
				if(!T3Pattern)
				{
					pattern3->speed = T3.linear_velocity;
					pattern3->theta = T3.theta;
					cout << "pattern3->speed: " << pattern3->speed << endl;
					t0 = ros::Time::now().toSec();
				}
				
				T3MoveAgain = true;
			}
			if(T3MoveAgain && t3Finisher && T3.linear_velocity == 0)
			{
				t1 = ros::Time::now().toSec();
				pattern3->dist = abs((t1-t0) * pattern3->speed);
				cout << "pattern3->dist: " << pattern3->dist << endl;
				t3Finisher = false;
			}
			if(T3.linear_velocity != 0 && T3MoveAgain)
			{
				pattern3->rotation = T3.theta - pattern3->theta;
				cout << "rotation: " << pattern3->rotation << endl;
				T3Pattern = true;

			}
			/*while(T3.angular_velocity != 0){
				if(T3PatternTheta == false)
				{
					cout << "second_x: " << T3.x << " second_y: " << T3.y << endl;
					pattern3->second_x = T3.x;
					pattern3->second_y = T3.y;
				}
				T3PatternTheta = true;
				updateTTurtles(nh);
					//<< "T3 theta: " << T3.theta << endl;
			//findMinMax(pattern3, T3);
				pastT3theta = T3.theta;
				thetaFinisher = true;
			//T3Pattern = false;
			//cout << "originalTheta: " << originalTheta << endl;
			//cout << "originalTheta - pastT3theta = " << originalTheta-pastT3theta << endl;
				pattern3->rotation = pastT3theta - originalTheta;
				if(T3.linear_velocity != 0)
				{
					cout << "T3.linear_velocity = " << T3.linear_velocity << endl;
					cout << "pattern3->rotation = " << pattern3->rotation << endl;

					break;
				}
				if(thetaFinisher == true && T3Pattern == true)
				{
					cout << "pattern3->rotation = " << pattern3->rotation << endl;
					break;
				}
			}
			cout << "angular: " << T3.angular_velocity << endl;*/

		}
		cout << "rotate: " << pattern3->rotation << endl;
	
	}
	cout << "pattern3->rotation = " << pattern3->rotation << endl;
	assignPattern(pattern1);
	cout << "T1 type: " << pattern1->type << endl;
	assignPattern(pattern2);
	cout << "T2 type: " << pattern2->type << endl;
	assignPattern(pattern3);
	cout << "T3 type: " << pattern3->type << endl;
}

/*void learnPattern(ros::NodeHandle nh){
	bool T1Pattern = false, T2Pattern = false, T3Pattern = false, T3PatternTheta=false;
	cout << "start sleeping" << endl;
	ros::Duration duration(1./24.);
	float pastT1x = T1.x, pastT1y = T1.y, pastT2x = T2.x, pastT2y = T2.y, pastT3x = T3.x, pastT3y = T3.y, pastT3theta = T3.theta;
	float originalTheta = T3.theta;
	Pattern* pattern1 = new Pattern(T1);
	Pattern* pattern2 = new Pattern(T2);
	Pattern* pattern3 = new Pattern(T3);
	//Pattern* pattern2 = new Pattern(T3);

	t3pose = nh.subscribe("/T3/pose", 10, poseCallbackT3);
	ros::spinOnce();
	cout <<"original T3" << T3.theta << endl;

	while(!T1Pattern || !T2Pattern || !T3Pattern || !T3PatternTheta)
	{
		//
		updateTTurtles(nh);

		//cout << "T1 y: " << T1.y << endl;
		// while condition (false || )
		while(T1.x != pastT1x || T1.y != pastT1y){
			//cout << "T1 ";
			//updateTTurtles(nh);
			t1Pose = nh.subscribe("/T1/pose", 10, poseCallbackT1);
		  ros::spinOnce();
			findMinMax(pattern1, T1);
			pastT1x = T1.x;
			pastT1y = T1.y;
			//cout << "T1.y: " << T1.y << " pastT1y: " << pastT1y << endl;
			T1Pattern = true;
		}
		//cout << "T1 loop end" << endl;
		while(T2.x != pastT2x || T2.y != pastT2y){
			t2Pose = nh.subscribe("/T2/pose", 10, poseCallbackT2);
		  ros::spinOnce();
			findMinMax(pattern2, T2);
			pastT2x = T2.x;
			pastT2y = T2.y;
			T2Pattern = true;
		}

		while(T3.x != pastT3x || T3.y != pastT3y){
			t3pose = nh.subscribe("/T3/pose", 10, poseCallbackT3);
			ros::spinOnce();
			//cout << "T3 theta: " << T3.theta << endl;
			//findMinMax(pattern3, T3);
			pastT3x = T3.x;
			pastT3y = T3.y;
			//pastT3theta = T3.theta;
			T3Pattern = true;
		}

		while(T3.angular_velocity != 0){
			if(T3PatternTheta == false)
			{
				cout << "second_x: " << T3.x << " second_y: " << T3.y << endl;
				pattern3->second_x = T3.x;
				pattern3->second_y = T3.y;
			}
			t3pose = nh.subscribe("/T3/pose", 10, poseCallbackT3);
			ros::spinOnce();
			cout << "T3 angular: " << T3.angular_velocity << endl;
					//<< "T3 theta: " << T3.theta << endl;
			//findMinMax(pattern3, T3);
			pastT3theta = T3.theta;
			T3PatternTheta = true;
			//T3Pattern = false;
			//cout << "originalTheta: " << originalTheta << endl;
			//cout << "originalTheta - pastT3theta = " << originalTheta-pastT3theta << endl;
			pattern3->rotation = originalTheta-pastT3theta;

	}

	}
	cout << "big while loop end" << endl;
	assignPattern(pattern1);
	cout << "T1 type: " << pattern1->type << endl;
	assignPattern(pattern2);
	cout << "T2 type: " << pattern2->type << endl;
	assignPattern(pattern3);
	cout << "T3 type: " << pattern3->type << endl;

}*/

bool calculateNewDistance(double distance, Pattern* pattern)
{
	double xPos = pattern-> initial_x + distance*cos(pattern->theta), yPos = pattern->initial_y + distance*sin(pattern->theta);
	double overflow;

	while(xPos > 11 || xPos < 0 || yPos < 0 || yPos > 11)
	{
		if(xPos > 11)
		{
			overflow = xPos -11;
			xPos = 11 - overflow;
		}
		else if(xPos < 0)
		{
			overflow= abs(xPos);
			xPos = overflow;
		}
		if(yPos > 11)
		{
			overflow = yPos -11;
			yPos = 11 - overflow;
		}
		else if (yPos < 0)
		{
			overflow = abs(yPos);
			yPos = overflow;
		}
	}

	if(abs(xPos-pattern->second_x) <0.1 && abs(yPos-pattern->second_y) <0.1)
	{
		return true;
	}
	else
		return false;

}

void verticalChangePosition(double x, double& y, ros::NodeHandle nh)
{
	updateXTurtles(nh);
		if(villainDist(x,y) < 1)
		{
			if(y==11)
				y--;
			else
				y++;
		}
}

void horizontalChangePosition(double& x, double y, ros::NodeHandle nh)
{
	updateXTurtles(nh);
		if(villainDist(x,y) < 1)
		{
			if(x==11)
				x--;
			else
				x++;
		}
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "hw4");
  ros::NodeHandle nh;
  ros::Rate loopRate(2);
  ROS_INFO_STREAM("START TO PUBLISH");

  ros::Subscriber pose_sub;
  velocity_publisher = nh.advertise<geometry_msgs::Twist>("turtle1/cmd_vel", 10);
  ros::spinOnce();
  loopRate.sleep();
  pose_subscriber = nh.subscribe("/turtle1/pose", 10, poseCallback);
  ros::spinOnce();
  loopRate.sleep();

  ros::spinOnce();
  loopRate.sleep();

  cout << "turtle1: " << "(" << turtlesim_pose.x << ", " << turtlesim_pose.y << ")" << endl;



  //updateTurtles(nh);
  updateTTurtles(nh);
  updateXTurtles(nh);
  cout << "Begin subscribe test" << endl;
  cout << "T1: " << "(" << T1.x << ", " << T1.y << ")" << endl;
  cout << "T2: " << "(" << T2.x << ", " << T2.y << ")" << endl;
  cout << "T3: " << "(" << T3.x << ", " << T3.y << ")" << endl;
  cout << "X1: " << "(" << X1.x << ", " << X1.y << ")" << endl;
  cout << "X2: " << "(" << X2.x << ", " << X2.y << ")" << endl;
  cout << "X3: " << "(" << X3.x << ", " << X3.y << ")" << endl;


  bool capturedT1 = false;
  bool capturedT2 = false;
  bool capturedT3 = false;

  ros::ServiceClient kClient = nh.serviceClient<turtlesim::Kill>("kill");

	// move a little bit and start to learn the Patterns
   move(1, 0.6, true);

   Pattern* pattern1 = new Pattern(T1);
   Pattern* pattern2 = new Pattern(T2);
   Pattern* pattern3 = new Pattern(T3);
   //learnPat(nh, pattern1, pattern2, pattern3);
   jPattern(nh, pattern1, pattern2, pattern3);
   //cout << "rotation for T3: " << pattern3->rotation << " in degrees: " << radianstodegrees(pattern3->rotation) << endl;


  turtlesim::Pose current_target;
  string name;
  int capT = 0;
  bool capturedT = true;
  int min = 1000;
  int index = 0;
  double distT1, distT2, distT3;
  double t0 = ros::Time::now().toSec();
  double targetx,targety,targetMinX, targetMinY;
  bool success= false;
  Pattern * tPattern;
  for(int j=0; j < 3; j++){
  // find the minimal distance from turtle1 to all the target turtles.

  	updateTurtles(nh);
  	if(capturedT1)
  	{
  		distT1 = 1000;
  	}
  	else
  	{
  		distT1 = getDistance(turtlesim_pose.x,turtlesim_pose.y, T1);
  	}
    if(capturedT2)
    {
    	distT2 = 1000;
    }
    else
    {
    	distT2 = getDistance(turtlesim_pose.x,turtlesim_pose.y, T2);
    }
    if(capturedT3)
    {
    	distT3 = 1000;
    }
    else
    {
    	distT3 = getDistance(turtlesim_pose.x,turtlesim_pose.y, T3);
    }
    //might be a problem if T3 has already been captured and distT1 = distT2

    if(distT1 < distT2 && distT1 < distT2)
    { 
    	name = "T1";
    	capturedT1 = true;
    	current_target = T1;
    	tPattern = pattern1;

    }
    else if(distT2 < distT1 && distT2 < distT3)
    {
    	name = "T2";
    	capturedT2 = true;
    	current_target = T2;
    	tPattern = pattern2;
    }
    else
    {
    	name = "T3";
    	capturedT3 = true;
    	current_target = T3;
    	tPattern = pattern3;
    }
    if(tPattern->type == "vertical")
    {
    	targetx = tPattern->max_x;
   		targetMinY = 0;
    	targety = 11;
    }
    else if(tPattern->type == "horizontal")
    {
    	targetx = 11;
    	targetMinX = 0;
    	targety = tPattern->max_y;
    }
    //go after the closest turtle
    ROS_INFO_STREAM( "going after: " << name << endl);
    //remove the turtle we are looking for, as we dont want to try to find it again
	//capturedT = moveGoal(current_target, 0.5, nh);

	if(tPattern->type != "rotation")
	{
		success = false;
		//ERROR: If turtle does not move very much, we miss it
		
		while(!success){
			if(tPattern->type == "vertical")
			{
				targety = 11;
				verticalChangePosition(targetx, targety, nh);
				success = moveGoal(current_target, name, targetx, targety, 0.5, nh);
				if(success)
					break;
				orientate(degreestoradians(270));
				targetMinY = 0;
				verticalChangePosition(targetx,targetMinY, nh);
				success = moveGoal(current_target, name, targetx, targetMinY, 0.5, nh);
				if(!success)
					orientate(degreestoradians(90));
			}
			else
			{
				targetx = 11;
				horizontalChangePosition(targetx,targety,nh);
				success = moveGoal(current_target, name, targetx, targety, 0.5, nh);
				if(success)
					break;
				targetMinX = 0;
				horizontalChangePosition(targetMinX,targety,nh);
				orientate(degreestoradians(0));
				success = moveGoal(current_target, name, targetMinX, targety, 0.5, nh);
				if(!success)
					orientate(degreestoradians(180));
			}	


		}
		capT++;
	}
	else
	{
		success = false;
		//cout << "initial_x: " << tPattern->initial_x << " initial_y: " << tPattern->initial_y << endl;
		//cout << "second_x: " << tPattern->second_x << " second_y: " << tPattern->second_y << endl;
		while(!calculateNewDistance(tPattern->dist, tPattern))
		{
			tPattern->dist += 0.1;
		}
		cout << "tPattern->dist = " << tPattern->dist << endl;
		//tPattern->dist = calculateNewDistance(tPattern->dist, tPattern);

		success = moveGoal(current_target, name, tPattern->initial_x, tPattern->initial_y, 0.2, nh);
		float current_theta = tPattern->theta;
		orientate(current_theta);

		while(!success)
		{

			cout << "turtlesim_pose.x: " << turtlesim_pose.x << " turtlesim_pose.y: " << turtlesim_pose.y << endl; 
			cout << "rotation: " << tPattern->rotation << " current_theta: " << current_theta << endl;
			success = moveDistance(tPattern->dist, nh, name, current_target, 0.2);
			current_theta = current_theta + tPattern->rotation;
			orientate(current_theta);
		}
		if(success)
		{
			capT++;
		}
	}
	
}


	//ros::Rate loop_rate(100000);

	double t1 = ros::Time::now().toSec();
	ROS_INFO_STREAM( "Distance = time * velocity (1) = " << t1-t0 );
	ROS_INFO_STREAM( "Number of target turtles captured: " << capT );

  ROS_INFO_STREAM("Finished");

  //========================= end of calling moveGoal========================





  return 0;
}
