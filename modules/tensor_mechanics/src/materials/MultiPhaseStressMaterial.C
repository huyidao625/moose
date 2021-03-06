/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/
#include "MultiPhaseStressMaterial.h"
#include "RankTwoTensor.h"
#include "ElasticityTensorR4.h"

template<>
InputParameters validParams<MultiPhaseStressMaterial>()
{
  InputParameters params = validParams<Material>();
  params.addClassDescription("Compute a global stress form multiple phase stresses");
  params.addParam<std::vector<std::string> >("h", "Switching Function Materials that provide h(eta_i)");
  params.addRequiredParam<std::vector<std::string> >("phase_base", "Base names for the Phase strains");
  params.addParam<std::string>("base_name", "Base name for the computed global stress (optional)");
  return params;
}

MultiPhaseStressMaterial::MultiPhaseStressMaterial(const InputParameters & parameters) :
    Material(parameters),
    _h_list(getParam<std::vector<std::string> >("h")),
    _n_phase(_h_list.size()),
    _h_eta(_n_phase),
    _phase_base(getParam<std::vector<std::string> >("phase_base")),
    _phase_stress(_n_phase),
    _dphase_stress_dstrain(_n_phase),
    _base_name(isParamValid("base_name") ? getParam<std::string>("base_name") + "_" : "" ),
    _stress(declareProperty<RankTwoTensor>(_base_name + "stress")),
    _dstress_dstrain(declareProperty<ElasticityTensorR4>(_base_name + "Jacobian_mult"))
{
  // verify parameter length
  if (_n_phase != _phase_base.size())
    mooseError("h and phase_base input vectors need to have the same length in MultiPhaseStressMaterial " << name());

  for (unsigned int i = 0; i < _n_phase; ++i)
  {
    _h_eta[i] = &getMaterialProperty<Real>(_h_list[i]);
    _phase_stress[i] = &getMaterialProperty<RankTwoTensor>(_phase_base[i] + "_stress");
    _dphase_stress_dstrain[i] = &getMaterialProperty<ElasticityTensorR4>(_phase_base[i] + "_Jacobian_mult");
  }
}

void
MultiPhaseStressMaterial::computeQpProperties()
{
  _stress[_qp].zero();
  _dstress_dstrain[_qp].zero();

  for (unsigned int i = 0; i < _n_phase; ++i)
  {
    _stress[_qp] += (*_h_eta[i])[_qp] * (*_phase_stress[i])[_qp];
    _dstress_dstrain[_qp] += (*_h_eta[i])[_qp] * (*_dphase_stress_dstrain[i])[_qp];
  }
}
