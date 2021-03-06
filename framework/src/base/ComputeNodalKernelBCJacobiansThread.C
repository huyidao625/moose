/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

#include "ComputeNodalKernelBCJacobiansThread.h"
#include "AuxiliarySystem.h"
#include "FEProblem.h"
#include "NodalKernel.h"
#include "NodalKernelWarehouse.h"

// libmesh includes
#include "libmesh/threads.h"

ComputeNodalKernelBCJacobiansThread::ComputeNodalKernelBCJacobiansThread(FEProblem & fe_problem,
                                                                         AuxiliarySystem & sys,
                                                                         std::vector<NodalKernelWarehouse> & nodal_kernels,
                                                                         SparseMatrix<Number> & jacobian) :
    ThreadedNodeLoop<ConstBndNodeRange, ConstBndNodeRange::const_iterator>(fe_problem),
    _sys(sys),
    _nodal_kernels(nodal_kernels),
    _jacobian(jacobian),
    _num_cached(0)
{
}

// Splitting Constructor
ComputeNodalKernelBCJacobiansThread::ComputeNodalKernelBCJacobiansThread(ComputeNodalKernelBCJacobiansThread & x, Threads::split split) :
    ThreadedNodeLoop<ConstBndNodeRange, ConstBndNodeRange::const_iterator>(x, split),
    _sys(x._sys),
    _nodal_kernels(x._nodal_kernels),
    _jacobian(x._jacobian),
    _num_cached(0)
{
}

void
ComputeNodalKernelBCJacobiansThread::pre()
{
  _num_cached = 0;
}

void
ComputeNodalKernelBCJacobiansThread::onNode(ConstBndNodeRange::const_iterator & node_it)
{
  const BndNode * bnode = *node_it;

  BoundaryID boundary_id = bnode->_bnd_id;

  std::vector<std::pair<MooseVariable *, MooseVariable *> > & ce = _fe_problem.couplingEntries(_tid);
  for (std::vector<std::pair<MooseVariable *, MooseVariable *> >::iterator it = ce.begin(); it != ce.end(); ++it)
  {
    MooseVariable & ivariable = *(*it).first;
    MooseVariable & jvariable = *(*it).second;

    unsigned int ivar = ivariable.number();
    unsigned int jvar = jvariable.number();

    // The NodalKernels that are active and are coupled to the jvar in question
    std::vector<MooseSharedPointer<NodalKernel> > active_involved_kernels;

    if (_nodal_kernels[_tid].activeBoundaryNodalKernels(boundary_id).size() > 0)
    {
      // Loop over each NodalKernel to see if it's involved with the jvar
      for (std::vector<MooseSharedPointer<NodalKernel> >::iterator nodal_kernel_it = _nodal_kernels[_tid].activeBoundaryNodalKernels(boundary_id).begin();
          nodal_kernel_it != _nodal_kernels[_tid].activeBoundaryNodalKernels(boundary_id).end();
          ++nodal_kernel_it)
      {
        MooseSharedPointer<NodalKernel> & nodal_kernel = *nodal_kernel_it;

        // If this NodalKernel isn't operating on this ivar... skip it
        if (nodal_kernel->variable().number() != ivar)
          break;

        // If this NodalKernel is acting on the jvar add it to the list and short-circuit the loop
        if (nodal_kernel->variable().number() == jvar)
        {
          active_involved_kernels.push_back(nodal_kernel);
          continue;
        }

        // See if this NodalKernel is coupled to the jvar
        const std::vector<MooseVariable *> & coupled_vars = (*nodal_kernel_it)->getCoupledMooseVars();
        for (std::vector<MooseVariable *>::iterator var_it; var_it != coupled_vars.end(); ++var_it)
        {
          if ( (*var_it)->number() == jvar )
          {
            active_involved_kernels.push_back(nodal_kernel);
            break; // It only takes one
          }
        }
      }
    }

    // Did we find any NodalKernels coupled to this jvar?
    if (!active_involved_kernels.empty())
    {
      // prepare variables
      for (std::map<std::string, MooseVariable *>::iterator it = _sys._nodal_vars[_tid].begin(); it != _sys._nodal_vars[_tid].end(); ++it)
      {
        MooseVariable * var = it->second;
        var->prepareAux();
      }

      if (_nodal_kernels[_tid].activeBoundaryNodalKernels(boundary_id).size() > 0)
      {
        Node * node = bnode->_node;
        if (node->processor_id() == _fe_problem.processor_id())
        {
          _fe_problem.reinitNodeFace(node, boundary_id,  _tid);

          for (std::vector<MooseSharedPointer<NodalKernel> >::iterator nodal_kernel_it = active_involved_kernels.begin();
               nodal_kernel_it != active_involved_kernels.end();
               ++nodal_kernel_it)
            (*nodal_kernel_it)->computeOffDiagJacobian(jvar);

          _num_cached++;
        }
      }

      if (_num_cached == 20) //cache 20 nodes worth before adding into the jacobian
      {
        _num_cached = 0;
        Threads::spin_mutex::scoped_lock lock(Threads::spin_mtx);
        _fe_problem.assembly(_tid).addCachedJacobianContributions(_jacobian);
      }
    }
  }
}

void
ComputeNodalKernelBCJacobiansThread::join(const ComputeNodalKernelBCJacobiansThread & /*y*/)
{
}
