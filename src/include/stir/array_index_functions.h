//
// $Id$
//

#ifndef __stir_array_index_functions_h_
#define __stir_array_index_functions_h_

/*!
  \file 
  \ingroup buildblock
 
  \brief a variety of useful functions for indexing Array objects

  \author Kris Thielemans

  $Date$
  $Revision$
*/
/*
    Copyright (C) 2004- $Date$, Hammersmith Imanet Ltd
    See STIR/LICENSE.txt for details
*/


#include "stir/Array.h"
#include "stir/BasicCoordinate.h"

START_NAMESPACE_STIR

#if !defined(_MSC_VER) || _MSC_VER>1200
// VC 6.0 needs ugly work-arounds. We'll put these in the .inl file

/* \ingroup Array
   \name Functions for writing generic code with indexing of multi-dimensional arrays
   */
//@{ 

//! \name get(), an alternative for array indexing using BasicCoordinate objects
//{@

//! Case where the index has lower dimension than the array
template <int num_dimensions, int num_dimensions2, typename elemT>
inline
const Array<num_dimensions-num_dimensions2,elemT>&
get(const Array<num_dimensions,elemT>& a, const BasicCoordinate<num_dimensions2,int> &c);

//! Case where the index has the same dimension as the array
template <int num_dimensions, typename elemT>
inline
const elemT&
get(const Array<num_dimensions,elemT>& a, const BasicCoordinate<num_dimensions,int> &c);

//@}

//! Get the first multi-dimensional index of the array
template <int num_dimensions, typename T>
inline
BasicCoordinate<num_dimensions, int>
get_min_indices(const Array<num_dimensions, T>& a);


//! Given an index into an array, increment it to the next one
/*!
    \return \c true if the next index was still within the array,\x false otherwise

    This can be used to iterate through an array using code such as
    \code
    Array<num_dimensions2, T> array = ...;
    BasicCoordinate<num_dimensions, int> indices =
       get_min_indices(array);
    do 
    { something with indices }
    while (next(indices, array));
    \endcode
    \warning The above loop will fail for empty arrays
*/
template <int num_dimensions, typename T, int num_dimensions2>
inline
bool 
next(BasicCoordinate<num_dimensions, int>& indices, 
     const Array<num_dimensions2, T>& a);

//@}
#endif // end of VC 6.0 conditional


END_NAMESPACE_STIR

#include "stir/array_index_functions.inl"

#endif