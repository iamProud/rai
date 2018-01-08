#pragma once

#ifndef MLR_ROS
# error "Sorry, you can include this only when compiling against ROS"
#endif

#include <tf/transform_listener.h>
#include <tf/tf.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <std_msgs/ColorRGBA.h>
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Float64MultiArray.h>
#include <geometry_msgs/WrenchStamped.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/LaserScan.h>
#include <std_msgs/String.h>
#include <sensor_msgs/JointState.h>
#include <visualization_msgs/MarkerArray.h>

//===========================================================================

#include <Core/thread.h>
#include <Core/array.h>
#include <Geo/geo.h>
#include <Kin/kin.h>
#include <Control/ctrlMsg.h>
#include "msgs/JointState.h"
#ifdef MLR_PCL
#  include <PCL/conv.h>
#endif

/*
 * TODO:
 * a single RosCom class; constructor checks rosInit and spawns singleton spinner
 * has publish/subscribe(var&) method, stores all pub/subs and cleans them on destruction
 */

//===========================================================================
//
// utils
//


void rosCheckInit(const char* node_name="pr2_module");
bool rosOk();
struct RosInit{ RosInit(const char* node_name="mlr_module"); };

//-- ROS <--> MLR
std_msgs::String    conv_string2string(const mlr::String&);
mlr::String         conv_string2string(const std_msgs::String&);
std_msgs::String    conv_stringA2string(const StringA& strs);
mlr::Transformation conv_transform2transformation(const tf::Transform&);
mlr::Transformation conv_transform2transformation(const geometry_msgs::Transform&);
mlr::Transformation conv_pose2transformation(const geometry_msgs::Pose&);
mlr::Vector         conv_point2vector(const geometry_msgs::Point& p);
mlr::Quaternion     conv_quaternion2quaternion(const geometry_msgs::Quaternion& q);
void                conv_pose2transXYPhi(arr& q, uint qIndex, const geometry_msgs::PoseWithCovarianceStamped &pose);
arr                 conv_pose2transXYPhi(const geometry_msgs::PoseWithCovarianceStamped &pose);
double              conv_time2double(const ros::Time& time);
timespec            conv_time2timespec(const ros::Time&);
arr                 conv_wrench2arr(const geometry_msgs::WrenchStamped& msg);
byteA               conv_image2byteA(const sensor_msgs::Image& msg);
uint16A             conv_image2uint16A(const sensor_msgs::Image& msg);
floatA              conv_laserScan2arr(const sensor_msgs::LaserScan& msg);
#ifdef MLR_PCL
Pcl                 conv_pointcloud22pcl(const sensor_msgs::PointCloud2& msg);
#endif
arr                 conv_points2arr(const std::vector<geometry_msgs::Point>& pts);
arr                 conv_colors2arr(const std::vector<std_msgs::ColorRGBA>& pts);
CtrlMsg             conv_JointState2CtrlMsg(const marc_controller_pkg::JointState& msg);
arr                 conv_JointState2arr(const sensor_msgs::JointState& msg);
mlr::KinematicWorld conv_MarkerArray2KinematicWorld(const visualization_msgs::MarkerArray& markers);
std_msgs::Float32MultiArray conv_floatA2Float32Array(const floatA&);
std_msgs::Float64MultiArray conv_arr2Float64Array(const arr&);

//-- MLR -> ROS
geometry_msgs::Pose conv_transformation2pose(const mlr::Transformation&);
geometry_msgs::Transform conv_transformation2transform(const mlr::Transformation&);
std::vector<geometry_msgs::Point> conv_arr2points(const arr& pts);
marc_controller_pkg::JointState   conv_CtrlMsg2JointState(const CtrlMsg& ctrl);
floatA conv_Float32Array2FloatA(const std_msgs::Float32MultiArray&);
arr conv_Float32Array2arr(const std_msgs::Float32MultiArray &msg);
visualization_msgs::Marker conv_Shape2Marker(const mlr::Shape& sh);
visualization_msgs::MarkerArray conv_Kin2Markers(const mlr::KinematicWorld& K);

//-- get transformations
mlr::Transformation ros_getTransform(const std::string& from, const std::string& to, tf::TransformListener& listener);
mlr::Transformation ros_getTransform(const std::string& from, const std_msgs::Header& to, tf::TransformListener& listener, tf::Transform* returnRosTransform=NULL);
bool ros_getTransform(const std::string& from, const std::string& to, tf::TransformListener& listener, mlr::Transformation& result);


struct SubscriberType { virtual ~SubscriberType() {} }; ///< if types derive from RootType, more tricks are possible


//===========================================================================
//
// subscribing a message directly into an Var
//

template<class msg_type>
struct Subscriber : SubscriberType {
  Var<msg_type>& var;
  ros::NodeHandle *nh=NULL;
  ros::Subscriber sub;
  uint revision=0;
  Subscriber(Var<msg_type>& _var)
    : var(_var) {
    if(mlr::getParameter<bool>("useRos", true)){
      mlr::String topic_name = STRING("rai/" <<var.name);
      registry()->newNode<SubscriberType*>({"Subscriber", topic_name}, {var.registryNode}, this);
      LOG(0) <<"subscribing to topic '" <<topic_name <<"' <" <<typeid(msg_type).name() <<"> into var '" <<var.name <<'\'';
      nh = new ros::NodeHandle;
      sub  = nh->subscribe( topic_name.p, 100, &Subscriber::callback, this);
    }
  }
  ~Subscriber(){
    if(nh) delete nh;
  }
  void callback(const typename msg_type::ConstPtr& msg) { var.set() = *msg;  revision++; }
};


//===========================================================================
//
// subscribing a message into an MLR-type-var via a conv_* function
//

template<class msg_type>
struct Publisher : Thread {
  Var<msg_type> var;
  ros::NodeHandle *nh;
  ros::Publisher pub;
  mlr::String topic_name;

  Publisher(const Var<msg_type>& _var)
      : Thread(STRING("Publisher_"<<_var.name), -1.),
        var(this, _var, true),
        nh(NULL){
    if(mlr::getParameter<bool>("useRos", true)){
      mlr::String topic_name = STRING("rai/" <<var.name);
      LOG(0) <<"publishing to topic '" <<topic_name <<"' <" <<typeid(msg_type).name() <<">";
      nh = new ros::NodeHandle;
      pub = nh->advertise<msg_type>(topic_name.p, 1);
      mlr::wait(.1); //I hate this -- no idea why the publisher isn't ready right away..
//      threadOpen();
    }
  }
  ~Publisher(){
    threadClose();
    pub.shutdown();
    if(nh) delete nh;
  }
  void open(){}
  void close(){}
  void step(){
    if(nh){
      pub.publish(var.get()());
      LOG(0) <<"publishing to topic '" <<topic_name <<"' revision " <<var.getRevision() <<" of variable '" <<var.name <<'\'';
    }
  }
};


//===========================================================================
//
// subscribing a message into an MLR-type-var via a conv_* function
//

template<class msg_type, class var_type, var_type conv(const msg_type&)>
struct SubscriberConv : SubscriberType {
  Var<var_type> var;
  Var<mlr::Transformation> *frame;
  ros::NodeHandle *nh;
  ros::Subscriber sub;
  tf::TransformListener *listener;
  SubscriberConv(Var<var_type>& _var, const char* topic_name=NULL, Var<mlr::Transformation> *_frame=NULL)
    : var(NULL, _var), frame(_frame), nh(NULL), listener(NULL) {
    if(mlr::getParameter<bool>("useRos", true)){
      if(!topic_name) topic_name = var.name;
      nh = new ros::NodeHandle;
      if(frame) listener = new tf::TransformListener;
      registry()->newNode<SubscriberType*>({"Subscriber", topic_name}, {var.registryNode}, this);
      LOG(0) <<"subscribing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> into var '" <<var.name <<'\'';
      sub = nh->subscribe(topic_name, 1, &SubscriberConv::callback, this);
    }
  }
  SubscriberConv(const char* topic_name, const char* var_name, Var<mlr::Transformation> *_frame=NULL)
    : var(NULL, var_name), frame(_frame), nh(NULL), listener(NULL) {
    if(mlr::getParameter<bool>("useRos", true)){
      if(!topic_name) topic_name = var_name;
      nh = new ros::NodeHandle;
      if(frame) listener = new tf::TransformListener;
      registry()->newNode<SubscriberType*>({"Subscriber", topic_name}, {var.registryNode}, this);
      LOG(0) <<"subscribing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> into var '" <<var.name <<'\'';
      sub = nh->subscribe(topic_name, 1, &SubscriberConv::callback, this);
    }
  }
  ~SubscriberConv(){
    if(listener) delete listener;
    if(nh) delete nh;
  }
  void callback(const typename msg_type::ConstPtr& msg) {
    double time=conv_time2double(msg->header.stamp);
    var.set( time ) = conv(*msg);
    if(frame && listener){
      frame->set( time ) = ros_getTransform("/base_link", msg->header, *listener);
    }
  }
};


//===========================================================================
//
// subscribing a message into an MLR-type-var via a conv_* function
//

template<class msg_type, class var_type, var_type conv(const msg_type&)>
struct SubscriberConvNoHeader : SubscriberType {
  Var<var_type> var;
  ros::NodeHandle *nh;
  ros::Subscriber sub;
  SubscriberConvNoHeader(Var<var_type>& _var, const char* topic_name=NULL)
    : var(NULL, _var), nh(NULL) {
    if(mlr::getParameter<bool>("useRos", true)){
      if(!topic_name) topic_name = var.name;
      nh = new ros::NodeHandle;
      registry()->newNode<SubscriberType*>({"Subscriber", topic_name}, {var.registryNode}, this);
      LOG(0) <<"subscribing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> into var '" <<var.name <<'\'';
      sub = nh->subscribe( topic_name, 1, &SubscriberConvNoHeader::callback, this);
    }
  }
  SubscriberConvNoHeader(const char* var_name, const char* topic_name=NULL)
    : var(NULL, var_name), nh(NULL) {
    if(mlr::getParameter<bool>("useRos", true)){
      if(!topic_name) topic_name = var_name;
      nh = new ros::NodeHandle;
      registry()->newNode<SubscriberType*>({"Subscriber", topic_name}, {var.registryNode}, this);
      LOG(0) <<"subscribing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> into var '" <<var.name <<'\'';
      sub = nh->subscribe( topic_name, 1, &SubscriberConvNoHeader::callback, this);
    }
  }

  ~SubscriberConvNoHeader(){
    if(nh) delete nh;
  }
  void callback(const typename msg_type::ConstPtr& msg) {
    var.set() = conv(*msg);
  }
};


//===========================================================================
//
// subscribing a message into an MLR-type-var via a conv_* function
//

template<class msg_type, class var_type, msg_type conv(const var_type&)>
struct PublisherConv : Thread {
  Var<var_type> var;
  ros::NodeHandle *nh;
  ros::Publisher pub;
  mlr::String topic_name;

  PublisherConv(const Var<var_type>& _var, const char* _topic_name=NULL, double beatIntervalSec=-1.)
      : Thread(STRING("Publisher_"<<_var.name <<"->" <<_topic_name), beatIntervalSec),
        var(this, _var, beatIntervalSec<0.),
        nh(NULL),
        topic_name(_topic_name){
    if(mlr::getParameter<bool>("useRos", true)){
      if(!_topic_name) topic_name = var.name;
      LOG(0) <<"publishing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> from var '" <<var.name <<'\'';
      nh = new ros::NodeHandle;
      threadOpen();
    }
  }
  PublisherConv(const char* var_name, const char* _topic_name=NULL, double beatIntervalSec=-1.)
      : Thread(STRING("Publisher_"<<var_name <<"->" <<_topic_name), beatIntervalSec),
        var(this, var_name, beatIntervalSec<0.),
        nh(NULL),
        topic_name(_topic_name){
    if(mlr::getParameter<bool>("useRos", true)){
      if(!_topic_name) topic_name = var_name;
      LOG(0) <<"publishing to topic '" <<topic_name <<"' <" <<typeid(var_type).name() <<"> from var '" <<var.name <<'\'';
      nh = new ros::NodeHandle;
      threadOpen();
    }
  }
  ~PublisherConv(){
    threadClose();
    pub.shutdown();
    if(nh) delete nh;
  }
  void open(){
    if(nh){
      pub = nh->advertise<msg_type>(topic_name.p, 1);
      mlr::wait(.1); //I hate this -- no idea why the publisher isn't ready right away..
    }
  }
  void step(){
    if(nh){
      pub.publish(conv(var.get()));
      LOG(0) <<"publishing to topic '" <<topic_name <<"' revision " <<var.getRevision() <<" of variable '" <<var.name <<'\'';
      mlr::wait(1.);
    }
  }
  void close(){
  }
};

//===========================================================================

struct RosCom{
  struct RosCom_Spinner* spinner;
  RosCom(const char* node_name="rai_module");
  ~RosCom();
  template<class T> Subscriber<T> subscribe(Var<T>& v){ return Subscriber<T>(v); }
  template<class T> Publisher<T> publish(Var<T>& v){ return Publisher<T>(v); }
};
