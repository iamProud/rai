/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#pragma once

#include "frame.h"
#include "feature.h"

struct PairCollision;

namespace rai {

//===========================================================================

enum ForceExchangeType { FXT_none=-1, FXT_poa=0, FXT_torque=1, FXT_force, FXT_forceZ, FXT_poaOnly };

///Description of a ForceExchange
struct ForceExchange : Dof, NonCopyable, GLDrawer {
  Frame &a, &b;
  ForceExchangeType type;
  double scale=1.;
  double force_to_torque = 0.;
 private:
  PairCollision* __coll=0;
 public:

  arr poa;
  arr force;
  arr torque;

  ForceExchange(Frame& a, Frame& b, ForceExchangeType _type, const ForceExchange* copyContact=nullptr);
  ~ForceExchange();

  void setZero();
  uint getDimFromType();
  void setDofs(const arr& q, uint n);
  arr calcDofsFromConfig() const;
  String name() const{ return STRING("fex-" <<a.name <<'-' <<b.name); }

  virtual double sign(Frame *f) const { if(&a==f) return 1.; return -1.; }
  virtual void kinPOA(arr& y, arr& J) const;
  virtual void kinForce(arr& y, arr& J) const;
  virtual void kinTorque(arr& y, arr& J) const;

  PairCollision* coll();

  void glDraw(OpenGL&);
  virtual void write(ostream& os) const;
};
stdOutPipe(ForceExchange)

//===========================================================================

rai::ForceExchange* getContact(rai::Frame* a, rai::Frame* b, bool raiseErrorIfNonExist=true);

} //rai
