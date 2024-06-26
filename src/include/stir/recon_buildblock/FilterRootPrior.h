//
//
/*
    Copyright (C) 2000- 2007, Hammersmith Imanet Ltd
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0

    See STIR/LICENSE.txt for details
*/
/*!
  \file
  \ingroup priors
  \brief Declaration of class stir::FilterRootPriot

  \author Kris Thielemans
  \author Sanida Mustafovic

*/

#ifndef __stir_recon_buildblock_FilterRootPrior_H__
#define __stir_recon_buildblock_FilterRootPrior_H__

#include "stir/RegisteredParsingObject.h"
#include "stir/recon_buildblock/GeneralisedPrior.h"
#include "stir/shared_ptr.h"

START_NAMESPACE_STIR

template <typename DataT>
class DataProcessor;

/*!
  \ingroup priors
  \brief
  A class in the GeneralisedPrior hierarchy. This implements 'generalised'
  priors a la the Median Root Prior (which was invented by
  Sakari Alenius).

  This class takes an DataProcessor object (i.e. a filter), and computes
  the prior 'gradient' as
  \f[ G_v = \beta ( {\lambda_v \over F_v} - 1) \f]
  where \f$ \lambda\f$ is the data where to compute the gradient, and
  \f$F\f$ is the data obtained by filtering \f$\lambda\f$.

  However, we need to avoid division by 0, as it might cause a NaN or an
  'infinity'. So, we replace the quotient above by<br>
  if \f$|\lambda_v| < M*|F_v| \f$
  then \f${\lambda_v \over F_v}\f$
  else \f$M*\rm{sign}(F_v)*\rm{sign}(lambda_v)\f$,<br>
  where \f$M\f$ is an arbitrary threshold on the quotient (fixed to 1000 when
  I wrote this documentation, but check FilterRootPrior.cxx if you want to be sure).

  Note that for nearly all filters, this is not a real prior, as this
  'gradient' is \e not the gradient of a function. This can be checked
  by computing the 'Hessian' (i.e. the partial derivatives of the components
  of the gradient). For most (interesting) filters, the Hessian will no be
  symmetric.

  The Median Root Prior is obtained by using a MedianImageFilter3D as
  DataProcessor.
*/
template <typename DataT>
class FilterRootPrior : public RegisteredParsingObject<FilterRootPrior<DataT>, GeneralisedPrior<DataT>, GeneralisedPrior<DataT>>
{
private:
  typedef RegisteredParsingObject<FilterRootPrior<DataT>, GeneralisedPrior<DataT>, GeneralisedPrior<DataT>> base_type;

public:
  //! Name which will be used when parsing a GeneralisedPrior object
  static const char* const registered_name;

  //! Default constructor (no filter)
  FilterRootPrior();

  //! Constructs it explicitly
  FilterRootPrior(shared_ptr<DataProcessor<DataT>> const&, const float penalization_factor);

  bool is_convex() const override;

  //! compute the value of the function
  /*! \warning Generally there is no function associated to this prior,
    so we just return 0 and write a warning the first time it's called.
   */
  double compute_value(const DataT& current_estimate) override;

  //! compute gradient by applying the filter
  void compute_gradient(DataT& prior_gradient, const DataT& current_estimate) override;

  //! Has to be called before using this object
  Succeeded set_up(shared_ptr<const DataT> const& target_sptr) override;

protected:
  //! Check that the prior is ready to be used
  void check(DataT const& current_image_estimate) const override;

private:
  shared_ptr<DataProcessor<DataT>> filter_ptr;
  void set_defaults() override;
  void initialise_keymap() override;
};

END_NAMESPACE_STIR

#endif
