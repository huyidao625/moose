/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/
#ifndef EXTERNALFORCEDENSITYMATERIAL_H
#define EXTERNALFORCEDENSITYMATERIAL_H

#include "Material.h"
#include "DerivativeMaterialInterface.h"

//Forward Declarations
class ExternalForceDensityMaterial;
class Function;

template<>
InputParameters validParams<ExternalForceDensityMaterial>();

/**
 * This Material calculates the force density acting on a particle/grain
 * due to interaction between particles
 */
class ExternalForceDensityMaterial : public DerivativeMaterialInterface<Material>
{
public:
  ExternalForceDensityMaterial(const InputParameters & parameters);

protected:
  virtual void computeQpProperties();

private:
  Function & _force_x;
  Function & _force_y;
  Function & _force_z;

  /// concentration field considered to be the density of particles
  VariableValue & _c;
  VariableName _c_name;
  /// stiffness constant
  Real _k;

  unsigned int _ncrys;
  std::vector<VariableValue *> _vals;

  /// force density material
  MaterialProperty<std::vector<RealGradient> > & _dF;
  /// first order derivative of force density material w.r.t c
  MaterialProperty<std::vector<RealGradient> > & _dFdc;
};

#endif //EXTERNALFORCEDENSITYMATERIAL_H
