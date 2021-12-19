/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#include "kin.h"

class btRigidBody;

namespace rai {
  struct Bullet_Options {
    RAI_PARAM("bullet/", double, defaultFriction, 1.)
    RAI_PARAM("bullet/", double, defaultRestitution, .1)
    RAI_PARAM("bullet/", double, contactStiffness, 1e4)
    RAI_PARAM("bullet/", double, contactDamping, 1e-1)
  };
}//namespace

struct BulletInterface {
  struct BulletInterface_self* self=0;

  BulletInterface(rai::Configuration& C, int verbose=1, bool yAxisGravity=false);
  ~BulletInterface();

  void step(double tau=.01);

  void pushKinematicStates(const FrameL& frames);
  void pushFullState(const FrameL& frames, const arr& frameVelocities=NoArr);
  void pullDynamicStates(FrameL& frames, arr& frameVelocities=NoArr);

  void changeObjectType(rai::Frame* f, int _type, const arr& withVelocity={});

  void saveBulletFile(const char* filename);
  class btDiscreteDynamicsWorld* getDynamicsWorld();
};


struct BulletBridge{
  class btDiscreteDynamicsWorld* dynamicsWorld;
  rai::Array<class btRigidBody*> actors;

  BulletBridge(class btDiscreteDynamicsWorld* _dynamicsWorld);

  void getConfiguration(rai::Configuration& C);
  void pullPoses(rai::Configuration& C, bool alsoStaticAndKinematic);
};
