#pragma once

#include "komo.h"

namespace rai {

//default - transcription as sparse, but non-factored NLP
struct Conv_KOMO_NLP : NLP {
  KOMO& komo;
  bool sparse;

  arr quadraticPotentialLinear, quadraticPotentialHessian;

  Conv_KOMO_NLP(KOMO& _komo, bool sparse=true);

  virtual arr getInitializationSample(const arr& previousOptima= {});
  virtual void evaluate(arr& phi, arr& J, const arr& x);
  virtual void getFHessian(arr& H, const arr& x);

  virtual void report(ostream& os, int verbose, const char* msg=0);
};

//this treats EACH PART and force-dof as its own variable
struct Conv_KOMO_FactoredNLP : NLP_Factored {
  KOMO& komo;

  //redundant to NLP_Factored::variableDims -- but sub can SUBSELECT!; in addition: dofs and names
  struct VariableIndexEntry { uint dim; DofL dofs; String name; };
  rai::Array<VariableIndexEntry> __variableIndex;
  uintA subVars;

  //redundant to NLP_Factored::featureDims*  -- but sub can SUBSELECT!
  struct FeatureIndexEntry { /*uint dim;*/ uintA vars; shared_ptr<GroundedObjective> ob; };
  rai::Array<FeatureIndexEntry> __featureIndex;
  uintA subFeats;

  virtual uint varsN() { if(subVars.N) return subVars.N; return __variableIndex.N; }
  virtual uint featsN() { if(subVars.N) return subFeats.N; return __featureIndex.N; }
  VariableIndexEntry& vars(uint var_id){ if(subVars.N) return __variableIndex(subVars(var_id)); else return __variableIndex(var_id); }
  FeatureIndexEntry& feats(uint feat_id){ if(subVars.N) return __featureIndex(subFeats(feat_id)); else return __featureIndex(feat_id); }

  Conv_KOMO_FactoredNLP(KOMO& _komo, const rai::Array<DofL>& varDofs);

  virtual void subSelect(const uintA& activeVariables, const uintA& conditionalVariables);
  virtual uint numTotalVariables(){ return __variableIndex.N; }

  virtual rai::String getVariableName(uint var_id){ return vars(var_id).name; }

  ///-- signature/structure of the mathematical problem
//    virtual arr getInitializationSample();
  virtual arr getInitializationSample(const arr& previousOptima);
  virtual arr  getSingleVariableInitSample(uint var_id);
  virtual void randomizeSingleVariable(uint var_id);

  ///--- evaluation
  virtual void setSingleVariable(uint var_id, const arr& x); //set a single variable block
  virtual void evaluateSingleFeature(uint feat_id, arr& phi, arr& J, arr& H); //get a single feature block

  void evaluate(arr& phi, arr& J, const arr& x);
  void report(ostream& os, int verbose, const char* msg=0);
};

}//namespace
