// $Id$
#ifndef __stir_read_data_H__
#define __stir_read_data_H__

/*!
  \file 
  \ingroup Array 
  \brief declarations of read_data() functions for reading Arrays from file

  \author Kris Thielemans

  $Date$
  $Revision$
*/
/*
    Copyright (C) 2004- $Date$, Hammersmith Imanet Ltd
    See STIR/LICENSE.txt for details
*/

#include "stir/ByteOrder.h"
#include <stdio.h>
#include <iostream>
START_NAMESPACE_STIR

class Succeeded;
class NumericType;
template <class T> class NumericInfo;
template <int num_dimensions, class elemT> class Array;

/*! \ingroup Array
  \brief Read the data of an Array from file.

  Only the data will be written, not the dimensions, start indices, nor byte-order.
  Hence, this should only used for low-level IO.

  \a IStreamT is supposed to be stream or file type (see implementations for 
  read_data_1d()).

  \warning When an error occurs, the function immediately returns. 
  However, the data might have been partially read from \a s.
*/
template <class IStreamT, int num_dimensions, class elemT>
inline Succeeded 
read_data(IStreamT& s, Array<num_dimensions,elemT>& data, 
	  const ByteOrder byte_order=ByteOrder::native);

/*! \ingroup Array
  \brief Read the data of an Array from file as a different type.

  This function essentially first calls convert_data() to construct
  an array with elements of type \a InputType, and then calls 
  read_data(IStreamT&, const Array<num_dimensions,elemT>&, 
	   const ByteOrder, const bool).
  \see read_data(IStreamT&, const Array<num_dimensions,elemT>&, 
	   const ByteOrder, const bool)

  \see find_scale_factor() for the meaning of \a scale_factor.
*/
template <class IStreamT, int num_dimensions, class elemT, class InputType, class ScaleT>
inline Succeeded 
read_data(IStreamT& s, Array<num_dimensions,elemT>& data, 
	  NumericInfo<InputType> input_type, 
	  ScaleT& scale_factor,
	  const ByteOrder byte_order=ByteOrder::native);

/*! \ingroup Array
  \brief Read the data of an Array from file as a different type.

  \see read_data(IStreamT&, const Array<num_dimensions,elemT>&, 
	   NumericInfo<InputType>, 
	   ScaleT&,
	   const ByteOrder,
	   const bool)
  The only difference is that the input type is now specified using NumericType.
*/
template <class IStreamT, int num_dimensions, class elemT, class ScaleT>
inline Succeeded 
read_data(IStreamT& s, 
	   Array<num_dimensions,elemT>& data, 
	   NumericType type, ScaleT& scale,
	   const ByteOrder byte_order=ByteOrder::native);


/* \ingroup Array
  \brief  This is the (internal) function that does the actual reading from std::istream.
  \internal
 */
template <class elemT>
inline Succeeded
read_data_1d(std::istream& s, Array<1, elemT>& data,
	     const ByteOrder byte_order);


/* \ingroup Array
  \brief  This is the (internal) function that does the actual reading from a FILE*.
  \internal
 */
template <class elemT>
inline Succeeded
read_data_1d(FILE*& , Array<1, elemT>& data,
	     const ByteOrder byte_order);

END_NAMESPACE_STIR

#include "stir/read_data.inl"

#endif