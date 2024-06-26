//
//
/*!

  \file
  \ingroup projdata

  \brief Implementations of inline functions of class stir::Bin

  \author Nikos Efthimiou
  \author Sanida Mustafovic
  \author Kris Thielemans
  \author PARAPET project

*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- 2009, Hammersmith Imanet Ltd
    Copyright (C) 2016, University of Hull
    Copyright (C) 2023, University College London
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0 AND License-ref-PARAPET-license

    See STIR/LICENSE.txt for details
*/

START_NAMESPACE_STIR

Bin::Bin()
    : axial_pos(0),
      tangential_pos(0),
      bin_value(0.0f),
      time_frame(1)
{}

Bin::Bin(int segment_num, int view_num, int axial_pos_num, int tangential_pos_num, float bin_value)
    : ViewgramIndices(view_num, segment_num, /*timing_pos_num*/ 0),
      axial_pos(axial_pos_num),
      tangential_pos(tangential_pos_num),
      bin_value(bin_value),
      time_frame(1)
{}

Bin::Bin(int segment_num, int view_num, int axial_pos_num, int tangential_pos_num)
    : ViewgramIndices(view_num, segment_num, /*timing_pos_num*/ 0),
      axial_pos(axial_pos_num),
      tangential_pos(tangential_pos_num),
      bin_value(0.0f),
      time_frame(1)
{}

Bin::Bin(int segment_num, int view_num, int axial_pos_num, int tangential_pos_num, int timing_pos_num, float bin_value)
    : ViewgramIndices(view_num, segment_num, timing_pos_num),
      axial_pos(axial_pos_num),
      tangential_pos(tangential_pos_num),
      bin_value(bin_value),
      time_frame(1)
{}

Bin::Bin(int segment_num, int view_num, int axial_pos_num, int tangential_pos_num, int timing_pos_num)
    : ViewgramIndices(view_num, segment_num, timing_pos_num),
      axial_pos(axial_pos_num),
      tangential_pos(tangential_pos_num),
      bin_value(0.0f),
      time_frame(1)
{}

int
Bin::axial_pos_num() const
{
  return axial_pos;
}

int
Bin::tangential_pos_num() const
{
  return tangential_pos;
}

int
Bin::time_frame_num() const
{
  return time_frame;
}

int&
Bin::axial_pos_num()
{
  return axial_pos;
}

int&
Bin::tangential_pos_num()
{
  return tangential_pos;
}

int&
Bin::time_frame_num()
{
  return time_frame;
}

#if 0
const ProjDataInfo *
Bin::get_proj_data_info_sptr() const
{
  return proj_data_info_ptr.get();
}
#endif

Bin
Bin::get_empty_copy() const
{

  Bin copy(segment_num(), view_num(), axial_pos_num(), tangential_pos_num(), timing_pos_num(), 0.f);

  return copy;
}

float
Bin::get_bin_value() const
{
  return bin_value;
}

void
Bin::set_bin_value(float v)
{
  bin_value = v;
}

Bin&
Bin::operator+=(const float dx)
{
  bin_value += dx;
  return *this;
}

bool
Bin::operator==(const Bin& bin2) const
{
  return base_type::operator==(bin2) && axial_pos == bin2.axial_pos && tangential_pos == bin2.tangential_pos
         && time_frame == bin2.time_frame && bin_value == bin2.bin_value;
}

bool
Bin::operator!=(const Bin& bin2) const
{
  return !(*this == bin2);
}

Bin&
Bin::operator*=(const float dx)
{
  bin_value *= dx;
  return *this;
}

Bin&
Bin::operator/=(const float dx)
{
  if (dx == 0.f)
    bin_value = 0.0f;
  else
    bin_value /= dx;

  return *this;
}

END_NAMESPACE_STIR
