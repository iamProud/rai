/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#pragma once

#include "kin.h"
#include "cameraview.h"

namespace rai {

struct SimulationState;
struct SimulationImp;

//a non-threaded simulation with direct interface and stepping -- in constrast to BotSim, which is threaded (emulating real time) and has
//the default ctrl interface via low-level reference messages
struct Simulation {
  enum SimulatorEngine { _physx, _bullet, _kinematic };
  enum ControlMode { _none, _position, _velocity, _acceleration, _pdRef, _spline };
  enum ImpType { _closeGripper, _openGripper, _depthNoise, _rgbNoise, _adversarialDropper, _objectImpulses, _blockJoints, _noPenetrations };

  std::unique_ptr<struct Simulation_self> self;

  Configuration& C;
  double time;
  arr qDot;
  SimulatorEngine engine;
  Array<shared_ptr<SimulationImp>> imps; ///< list of (adversarial) imps doing things/perturbations/noise in addition to clean physics engine
  int verbose;
  FrameL grasps;

  Simulation(Configuration& _C, SimulatorEngine _engine, int _verbose=2);
  ~Simulation();

  //== controller interface

  //-- step the simulation, optionally send a low-level control, or use the spline reference
  void step(const arr& u_control={}, double tau=.01, ControlMode u_mode = _spline);

  //-- adapt the spline reference to genreate motion (should become the default way)
  void setMoveTo(const arr& q, double t, bool append=true);
  void move(const arr& path, const arr& t);

  //-- send a gripper command
  void openGripper(const char* gripperFrameName, double width=.075, double speed=.3);
  void closeGripper(const char* gripperFrameName, double width=.05, double speed=.3, double force=20.);
  void closeGripperGrasp(const char* gripperFrameName, const char* objectName, double width=.05, double speed=.3, double force=20.);

  //-- get state information
  const arr& get_q() { return C.getJointState(); }
  const arr& get_qDot();
  double getTimeToMove();
  double getGripperWidth(const char* gripperFrameName);
  bool getGripperIsGrasping(const char* gripperFrameName);
  bool getGripperIsClose(const char* gripperFrameName);
  bool getGripperIsOpen(const char* gripperFrameName);

  //-- get sensor information
  void getImageAndDepth(byteA& image, floatA& depth); ///< use this during stepping
  void getSegmentation(byteA& segmentation);
  CameraView& cameraview(); ///< use this if you want to initialize the sensor, etc
  rai::CameraView::Sensor& addSensor(const char* sensorName, const char* frameAttached=nullptr, uint width=640, uint height=360, double focalLength=-1., double orthoAbsHeight=-1., const arr& zRange= {}) {
    if(frameAttached && frameAttached[0]) {
      return cameraview().addSensor(sensorName, frameAttached, width, height, focalLength, orthoAbsHeight, zRange);
    } else {
      return cameraview().addSensor(sensorName);
    }
  }
  rai::CameraView::Sensor&  selectSensor(const char* name) { return cameraview().selectSensor(name); }
  byteA getScreenshot();


  //== ground truth interface
  rai::Frame* getGroundTruthFrame(const char* frame) { return C.getFrame("frame"); }


  //== perturbation/adversarial interface

  //what are really the fundamental perturbations? Their (reactive?) management should be realized by 'agents'. We need a method to add purturbation agents.
  void addImp(ImpType type, const StringA& frames, const arr& parameters);


  //== management interface

  //-- store and reset the state of the simulation
  shared_ptr<SimulationState> getState();
  void restoreState(const shared_ptr<SimulationState>& state);
  void setState(const arr& frameState, const arr& frameVelocities=NoArr);
  void pushConfigurationToSimulator(const arr& frameVelocities=NoArr);

  //-- post-hoc world manipulations
  void registerNewObjectWithEngine(rai::Frame* f);

  //allow writing pics for video
  uint& pngCount();
};

}
