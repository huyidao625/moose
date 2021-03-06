#include "DefaultMatPropConsumerKernel.h"

template<>
InputParameters validParams<DefaultMatPropConsumerKernel>()
{
  InputParameters params = validParams<Kernel>();
  params.addParam<std::string>("mat_prop", "prop", "Material property name to fetch");
  return params;
}

DefaultMatPropConsumerKernel::DefaultMatPropConsumerKernel(const InputParameters & parameters) :
    DerivativeMaterialInterface<Kernel>(parameters),
    _prop_name(getParam<std::string>("mat_prop")),
    _prop(getDefaultMaterialProperty<Real>(_prop_name))
{
}
