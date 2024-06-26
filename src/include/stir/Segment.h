/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000-2012 Hammersmith Imanet Ltd
    Copyright (C) 2023, University College London
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0 AND License-ref-PARAPET-license

    See STIR/LICENSE.txt for details
*/
/*!

  \file
  \ingroup projdata
  \brief Declaration of class stir::Segment

  \author Kris Thielemans
  \author PARAPET project
*/
#ifndef __Segment_H__
#define __Segment_H__

#include "stir/ProjDataInfo.h"
#include "stir/SegmentIndices.h"
#include "stir/SinogramIndices.h"
#include "stir/ViewgramIndices.h"
#include "stir/shared_ptr.h"

START_NAMESPACE_STIR
template <typename elemT>
class Sinogram;
template <typename elemT>
class Viewgram;

/*!
  \brief An (abstract base) class for storing 3d projection data
  \ingroup projdata

  This stores a subset of the data accessible via a ProjData object,
  where the SegmentIndices are fixed.

  At the moment, 2 'storage modes' are supported (and implemented as
  derived classes).

  The template argument \c elemT is used to specify the data-type of the
  elements of the 3d object.
 */

template <typename elemT>
class Segment
{
#ifdef SWIG
  // need to make typedef public for swig
 public:
#endif
  typedef Segment<elemT> self_type;

public:
  enum StorageOrder
  {
    StorageByView,
    StorageBySino
  };

  virtual ~Segment()
  {}
  //! Get shared pointer to proj data info
  inline shared_ptr<const ProjDataInfo> get_proj_data_info_sptr() const;

  virtual StorageOrder get_storage_order() const = 0;
  inline SegmentIndices get_segment_indices() const;
  //! Get the segment number
  inline int get_segment_num() const;
  //! Get the timing position index
  inline int get_timing_pos_num() const;
  virtual int get_min_axial_pos_num() const = 0;
  virtual int get_max_axial_pos_num() const = 0;
  virtual int get_min_view_num() const = 0;
  virtual int get_max_view_num() const = 0;
  virtual int get_min_tangential_pos_num() const = 0;
  virtual int get_max_tangential_pos_num() const = 0;
  virtual int get_num_axial_poss() const = 0;

  virtual int get_num_views() const = 0;
  virtual int get_num_tangential_poss() const = 0;

  //! return a new sinogram, with data set as in the segment
  virtual Sinogram<elemT> get_sinogram(int axial_pos_num) const = 0;
  //! return a new viewgram, with data set as in the segment
  virtual Viewgram<elemT> get_viewgram(int view_num) const = 0;

  //! return a new sinogram, with data set as in the segment
  inline Sinogram<elemT> get_sinogram(const SinogramIndices& s) const;
  //! return a new viewgram, with data set as in the segment
  inline Viewgram<elemT> get_viewgram(const ViewgramIndices&) const;

  //! set data in segment according to sinogram \c s
  virtual void set_sinogram(const Sinogram<elemT>& s) = 0;
  //! set sinogram at a different axial_pos_num
  virtual void set_sinogram(const Sinogram<elemT>& s, int axial_pos_num) = 0;
  //! set data in segment according to viewgram \c v
  virtual void set_viewgram(const Viewgram<elemT>& v) = 0;

  //! \name Equality
  //@{
  //! Checks if the 2 objects have the proj_data_info, segment_num etc.
  /*! If they do \c not have the same characteristics, the string \a explanation
      explains why.
  */
  bool has_same_characteristics(self_type const&, std::string& explanation) const;

  //! Checks if the 2 objects have the proj_data_info, segment_num etc.
  /*! Use this version if you do not need to know why they do not match.
   */
  bool has_same_characteristics(self_type const&) const;

  //! check equality (data has to be identical)
  /*! Uses has_same_characteristics() and Array::operator==.
      \warning This function uses \c ==, which might not be what you
      need to check when \c elemT has data with float or double numbers.
  */
  virtual bool operator==(const self_type&) const = 0;

  //! negation of operator==
  bool operator!=(const self_type&) const;
  //@}

protected:
  shared_ptr<const ProjDataInfo> proj_data_info_sptr;
  SegmentIndices _indices;

  inline Segment(const shared_ptr<const ProjDataInfo>& proj_data_info_sptr_v, const SegmentIndices&);
};

END_NAMESPACE_STIR

#include "stir/Segment.inl"

#endif
