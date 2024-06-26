//
//
/*!
  \file
  \ingroup buildblock
  \brief This file initialises ByteOrder::native_order.

  \author Kris Thielemans
  \author Alexey Zverovich
  \author PARAPET project

*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- 2009, Hammersmith Imanet Ltd
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0 AND License-ref-PARAPET-license

    See STIR/LICENSE.txt for details
*/
/*
  Modification history:
  -first version(s) Kris Thielemans
  -KT&AZ 08/12/99 avoid using ntohs()
*/

#include "stir/ByteOrder.h"

START_NAMESPACE_STIR

/* A somewhat complicated way to determine the byteorder.
   The advantage is that it doesn't need ntohs (and so any
   compiler specific definitions, libraries or whatever).
   Philosophy: initialise a long number, look at its first byte
   by casting its address as a char *.
   The reinterpret_cash is to make it typesafe for C++.

   First we do a (paranoid) check :
      sizeof(unsigned long) - sizeof(unsigned char) > 0
   This is done via a 'compile-time assertion', i.e. it breaks
   at compile time when the assertion is false. The line below
   relies on the fact that you cannot have an array with
   zero (or less) elements.
 */

typedef char assert_unsigned_long_size[sizeof(unsigned long) - sizeof(unsigned char)];

static const unsigned long magic = 1;

const ByteOrder::Order ByteOrder::native_order
    = *(reinterpret_cast<const unsigned char*>(&magic)) == 1 ? little_endian : big_endian;

END_NAMESPACE_STIR
