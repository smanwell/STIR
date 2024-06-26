//
//
/*!

  \file
  \ingroup symmetries

  \brief Declaration of class stir::RelatedDensels

  \author Sanida Mustafovic
  \author Kris Thielemans
  \author PARAPET project

*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- 2009, Hammersmith Imanet Ltd
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0 AND License-ref-PARAPET-license

    See STIR/LICENSE.txt for details
*/
#ifndef __RelatedDensels_H__
#define __RelatedDensels_H__

#include "stir/shared_ptr.h"
#include "stir/Densel.h"
#include <vector>
#include <iterator>

START_NAMESPACE_STIR

class DataSymmetriesForDensels;
/*!
  \ingroup symmetries
  \brief This class contains all information about a set of densels related
  by symmetry.
*/
class RelatedDensels
{
public:
  //! typedefs for iterator support

  typedef std::random_access_iterator_tag iterator_category;
  typedef Densel value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::ptrdiff_t difference_type;
  typedef std::size_t size_type;

  //! typedefs to make it partly comply with STL requirements
  typedef std::vector<Densel>::iterator iterator;
  typedef std::vector<Densel>::const_iterator const_iterator;
  //! Default constructor: creates no densels, no symmetries
  inline RelatedDensels();

  //! get the number of related densels
  inline int get_num_related_densels() const;

  //! get 'basic' densel coordinates
  inline Densel get_basic_densel() const;

  // get the pointer to a ProjDataInfo class
  // inline const ProjDataInfo * get_proj_data_info_sptr() const;

  //! return the symmetries used
  inline const DataSymmetriesForDensels* get_symmetries_ptr() const;

  //! get an empty copy
  RelatedDensels get_empty_copy() const;

  // basic iterator support

  //! use to initialise an iterator to the first element of the vector
  inline iterator begin();
  //! iterator 'past' the last element of the vector
  inline iterator end();
  //! use to initialise an iterator to the first element of the (const) vector
  inline const_iterator begin() const;
  //! iterator 'past' the last element of the (const) vector
  inline const_iterator end() const;

private:
  std::vector<Densel> related_densels;
  shared_ptr<DataSymmetriesForDensels> symmetries;
  //! a private constructor which sets the members
  inline RelatedDensels(const std::vector<Densel>& related_densels, const shared_ptr<DataSymmetriesForDensels>& symmetries_used);
};

END_NAMESPACE_STIR

#include "stir/recon_buildblock/RelatedDensels.inl"

#endif //__RelatedDensels_H__
