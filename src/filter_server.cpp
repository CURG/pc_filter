#include "ros/ros.h"
#include <pcl/ros/conversions.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/filters/passthrough.h>
#include <pcl_ros/transforms.h>
#include <tf/transform_listener.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/io/pcd_io.h>

ros::Publisher filtered_pc_pub;
tf::TransformListener *tf_listener;

void filterCallback(const sensor_msgs::PointCloud2::ConstPtr& original_pc)
{
  //  pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_p (new pcl::PointCloud<pcl::PointXYZRGB>);
  //  pcl::fromROSMsg (*original_pc, *cloud_p);
  //  pcl::io::savePCDFileASCII ("test_pcd.pcd", *cloud_p);

  sensor_msgs::PointCloud2::Ptr cloud_transformed(new sensor_msgs::PointCloud2());
  sensor_msgs::PointCloud2::Ptr cloud_filtered(new sensor_msgs::PointCloud2());
  //sensor_msgs::PointCloud2 cloud_filtered;
  sensor_msgs::PointCloud2::Ptr cloud_transformed_back(new sensor_msgs::PointCloud2());

  //  ROS_INFO("START");
  std::string original_frame = original_pc->header.frame_id;
  //supposed to work, but not well with the time stamp
  tf_listener->waitForTransform ("/world", "/camera_rgb_optical_frame", ros::Time::now(), ros::Duration(2.0));
  /*
  pcl_ros::transformPointCloud("/world",
		      *original_pc,
		      *cloud_transformed,
		      *tf_listener);
		      
  cloud_transformed->header.frame_id = "/world";
  */
  //so instead, let's do it on our own
  tf::StampedTransform transform;
  tf_listener->waitForTransform ("/world", original_pc->header.frame_id, ros::Time(0), ros::Duration(1.0));
  tf_listener->lookupTransform ("/world", original_pc->header.frame_id, ros::Time(0), transform);
  Eigen::Matrix4f eigen_transform;
  pcl_ros::transformAsMatrix (transform, eigen_transform);
  pcl_ros::transformPointCloud (eigen_transform, *original_pc, *cloud_transformed);
  cloud_transformed->header.frame_id = "/world";
  /*
// ROS_INFO("DONE");
  
  cloud_transformed->header.frame_id = "/world";
  // Create the filtering object
  pcl::PassThrough<sensor_msgs::PointCloud2>pass;
  pass.setInputCloud(cloud_transformed);
  pass.setFilterFieldName ("z");
  pass.setFilterLimits(-0.05, .50);
  //pass.setFilterLimitsNegative (true);
  pass.filter(*cloud_transformed);
  */
  //std::cout << *cloud_transformed << std::endl;
  pcl::PassThrough<sensor_msgs::PointCloud2>pass;
  pass.setInputCloud(cloud_transformed);
  pass.setFilterFieldName ("x");
  pass.setFilterLimits (-1.0, 0.15);
  //pass.setFilterLimitsNegative (true);
  pass.filter(*cloud_transformed);

  pass.setInputCloud(cloud_transformed);
  pass.setFilterFieldName ("y");
  pass.setFilterLimits (-0.15, 1.0);
  //pass.setFilterLimitsNegative (true);
  pass.filter(*cloud_filtered);
  
  //transform back to camera's coordinate frame
  pcl_ros::transformPointCloud("/camera_rgb_optical_frame",
		      *cloud_filtered,
		      *cloud_transformed_back,
		      *tf_listener);
		      
  //  tf_listener->lookupTransform ("/camera_rgb_optical_frame", cloud_filtered->header.frame_id, ros::Time(0), transform);
  // pcl_ros::transformAsMatrix (transform, eigen_transform);
  //pcl_ros::transformPointCloud (eigen_transform, *cloud_filtered, *cloud_transformed_back);
  cloud_transformed_back->header.frame_id = "/camera_rgb_optical_frame";
  //done transform back
  
  filtered_pc_pub.publish(*cloud_transformed_back);
  
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "filteredpcserver");
  ros::NodeHandle n;
  tf::TransformListener tfl;
  tf_listener = &tfl;
  filtered_pc_pub = n.advertise<sensor_msgs::PointCloud2>("filtered_pc", 1);
  ros::Subscriber original_pc_sub = n.subscribe("/camera/depth_registered/points", 1, filterCallback);

  ros::spin();
  return 0;
}
