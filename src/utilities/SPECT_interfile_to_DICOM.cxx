/*
  Copyright (C) 2018, 2023, University College London
  Copyright (C) 2019-2023, National Physical Laboratory
  This file is part of STIR.

  SPDX-License-Identifier: Apache-2.0

  See STIR/LICENSE.txt for details
*/

/*!
  \file
  \ingroup utilities
  \brief Convert reconstructed SPECT images from Interfile to DICOM.

  Read files based on https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.8.4.html,
  https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.8.4.8.html etc

  \author Spencer Manwell
*/
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <stdio.h>
#include <time.h>

#include <boost/format.hpp>

#include <gdcmDataSet.h>
#include <gdcmAttribute.h>
#include <gdcmImageWriter.h>
#include <gdcmReader.h>
#include <gdcmImageReader.h>
#include <gdcmUIDGenerator.h>
//#include <gdcmCSComp>

// For error handling and messaging.
#include "stir/info.h"
#include "stir/error.h"
#include "stir/warning.h"
#include "stir/Succeeded.h"

// For reading SPECT (Interfile) images
#include "stir/utilities.h"
#include "stir/IO/Interfile.h"
#include "stir/IO/InterfileHeader.h"
#include "stir/VoxelsOnCartesianGrid.h"
#include "stir/PatientPosition.h"

// For reading the recon parameter information
#include "stir/OSMAPOSL/OSMAPOSLReconstruction.h"
#include "stir/DiscretisedDensity.h"
#include "stir/recon_buildblock/PoissonLogLikelihoodWithLinearModelForMeanAndProjData.h"
#include "stir/recon_buildblock/ProjectorByBinPairUsingProjMatrixByBin.h"
#include "stir/recon_buildblock/ProjMatrixByBinSPECTUB.h"

// For DICOM I/O
#include "stir/DICOM_defines.h"

// Interfile Keyword Defines
// that are represented in the DICOM output
#define def_psf_type "psf type"                         // 2D // 3D // Geometrical
#define def_atteunation_type "attenuation type"         // Simple // Full // No
#define def_scatter_type "scatter type"                 // None // TEW // DEW // SIM - DOES NOT CURENLTY EXIST IN THE STIR INTERFILE.
#define def_prior_type "prior type "                    // None // <others>
#define def_post_filter "post-filter type"              // Options: gaussian // median // metz // convolution - we'll probably only use gaussian.
#define def_x_filter_FWHM "x-dir filter FWHM (in mm)"   // integer value as string
#define def_y_filter_FWHM "y-dir filter FWHM (in mm)"   // integer value as string
#define def_z_filter_FWHM "z-dir filter FWHM (in mm)"   // integer value as string
#define def_num_subsets "number of subsets"             // integer value as string
#define def_num_subiterations "number of subiterations" // integer value as string
#define def_image_description "image description"       // None // <free-form <= 64 (LO)>
#define def_image_id "image identifier"                 // None // <free-form, <= 16 char (SH)>

template <class TArray>
char*
convert_vector_array_to_char_array( uint32_t& buffer_length, const std::vector<TArray> data )
{
  buffer_length = uint32_t(data.size() * sizeof(TArray));
  char* buffer = new char[buffer_length];
  char* p = buffer;
  for (auto _data : data)
    {
      memcpy(p, &_data, sizeof(TArray));
      p += sizeof(TArray);
    }
  return buffer;
}

template <typename... Args>
std::string
string_format(const std::string& format, Args... args)
{
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
  if (size_s <= 0)
    {
      throw std::runtime_error("Error during formatting.");
    }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template <class TArray>
std::string*
convert_vector_to_string_array(const std::vector<TArray> data, const std::string& format)
{
  std::string string_array[data.size()];
  int idx = 0;
  for (auto& _data : data)
    {
      string_array[idx] = string_format(format, _data);
    }

  return &string_array;
}

/*
    Assert that an element exists for the given tag in the input file.
    The file MUST have been read before calling this function.
*/
stir::Succeeded
check_tag_exists(gdcm::Tag tag, gdcm::Reader& reader)
{
  gdcm::DataElement element = reader.GetFile().GetDataSet().GetDataElement(tag);
  if (element.GetByteValue() == NULL)
    {
      // It seems like we'll get here for sequences too. So we need another check for these cases.
      const gdcm::SequenceOfItems* sqi = element.GetValueAsSQ();
      if ((sqi != NULL && sqi->GetNumberOfItems() == 0) || sqi == NULL)
        {
          // It either does not exist or its an empty sequence.
          //std::string tag_str = tag.PrintAsContinuousString();
          //stir::warning(
          //    boost::format("Dataset does not contain tag %1%")
          //    % tag_str);
          return stir::Succeeded::no;
        }
    }
  return stir::Succeeded::yes;
}

// TODO Test this function on a sequence of items which contains multiple items.
stir::Succeeded
replace_sequennce_item_data_element(gdcm::DataSet& ds, const gdcm::Tag sq_tag, const gdcm::DataElement de, int item_id )
{ 
  stir::Succeeded status = stir::Succeeded::no;

  // Assert that we can find the sequence in the DataSet
  if (ds.FindDataElement(sq_tag) == false)
    {
      return status;
    }

  const gdcm::DataElement& sq_de = ds.GetDataElement(sq_tag);
  gdcm::SmartPointer<gdcm::SequenceOfItems> sq = sq_de.GetValueAsSQ();
  if (sq->GetNumberOfItems())
    {
      // Check that the requested item_id is in bounds for the given sequence.
      if (item_id <= 0 || item_id > sq->GetNumberOfItems())
        {
          return status;
        }

      // Create a Sequence and insert the one item
      gdcm::SmartPointer<gdcm::SequenceOfItems> modified_sq = new gdcm::SequenceOfItems();
      modified_sq->SetLengthToUndefined();

      for (auto idx = 1; idx <= sq->GetNumberOfItems(); ++idx)
        {
          gdcm::Item item_idx = sq->GetItem(idx);
          if (idx == item_id)
          {
            gdcm::DataSet& item_ds = item_idx.GetNestedDataSet();
            item_ds.Replace(de); // Note that this call does not verify the insertion.
          }
          modified_sq->AddItem(item_idx);
      }

      // Define sequence attribute and initialize it with the sequence of items.
      gdcm::DataElement modified_sq_de(sq_tag);
      modified_sq_de.SetVR(gdcm::VR::SQ);
      modified_sq_de.SetValue(*modified_sq);
      modified_sq_de.SetVLToUndefined();

      // Replace sequence in data set (in case there was an existing entry).
      ds.Replace(modified_sq_de);

      status = stir::Succeeded::yes;
    }

  return status;
}

std::string
get_patient_position_as_string(gdcm::DataSet const ds)
{
  /*
   * Patient Orientation Modifier COde Sequence 99SDM Codes Values (Meaning)
   * F-10310 (prone)
   * F-10340 (supine)
   * F-10317 (right lateral decubitus)
   * F-10319 (left lateral decubitus)
   *
   * Patient Orientation COde Sequence 99SDM Code Values (Meaning)
   * F-10440 (erect)
   * F-10450 (recumbent)
   * F-10460 (semi-erect)
   *
   * Patient Gantry Relationship Code Sequence 99SDM Code Values (Meaning)
   * R-10516 (oblique)
   * F-10470 (headfirst)
   * F-10480 (feet-first)
   * R-10515 (transverse)
   *
   */
  // Determine the patient-gantry relationship.  
  // Support types are headfirst and feet-first.
  DCM_Code_Value code_value;
  DCM_Code_Meaning code_meaning;

  DCM_Patient_Gantry_Relationship_Code_Sequence patient_gantry_relationship_sequence; // By definition, contains only 1 item.
  stir::PatientPosition::OrientationValue patient_gantry_orientation
      = stir::PatientPosition::OrientationValue::unknown_orientation;
  const gdcm::SmartPointer<gdcm::SequenceOfItems> pgr_seq = ds.GetDataElement(patient_gantry_relationship_sequence.GetTag()).GetValueAsSQ();
  if (pgr_seq->GetNumberOfItems()>0)
    {
      gdcm::Item item = pgr_seq->GetItem(1);
      gdcm::DataSet& item_ds = item.GetNestedDataSet();
      code_value.SetFromDataElement(item_ds.GetDataElement(code_value.GetTag()));
      code_meaning.SetFromDataElement(item_ds.GetDataElement(code_meaning.GetTag()));
      if (code_value.GetValue() == "F-10470" && code_meaning.GetValue() == "headfirst")
      {
          patient_gantry_orientation = stir::PatientPosition::OrientationValue::head_in;
      }
      else if (code_value.GetValue() == "F-10480" && code_meaning.GetValue() == "feet-first")
      {
          patient_gantry_orientation = stir::PatientPosition::OrientationValue::feet_in;
      }
    }

  // Determine the patient-gravity relationship
  // Retrieve the Patient Orientation Code Sequence
  // Assert that the patient is recumbent, prior rot checking their pose on the bed.
  DCM_Patient_Orientation_Code_Sequence patient_orientation_sequence; // By definition, contains only 1 item.
  stir::PatientPosition::RotationValue patient_rotation
      = stir::PatientPosition::RotationValue::unknown_rotation;
  const gdcm::SmartPointer<gdcm::SequenceOfItems> po_seq
      = ds.GetDataElement(patient_orientation_sequence.GetTag()).GetValueAsSQ();
  if (po_seq->GetNumberOfItems() > 0)
    {
      gdcm::Item item = po_seq->GetItem(1);
      gdcm::DataSet& item_ds = item.GetNestedDataSet();
      code_value.SetFromDataElement(item_ds.GetDataElement(code_value.GetTag()));
      code_meaning.SetFromDataElement(item_ds.GetDataElement(code_meaning.GetTag()));
      if (code_value.GetValue() == "F-10450" && code_meaning.GetValue() == "recumbent")
      {
          // Patient was recumbent for the acquisition.
          // Determine the orientation modifier.
          DCM_Patient_Orientation_Modifier_Code_Sequence patient_orientation_modifier_sequence;
          const gdcm::SmartPointer<gdcm::SequenceOfItems> pom_seq
              = ds.GetDataElement(patient_orientation_sequence.GetTag()).GetValueAsSQ();
          if (pom_seq->GetNumberOfItems() > 0)
          {
            gdcm::Item item = po_seq->GetItem(1);
            gdcm::DataSet& item_ds = item.GetNestedDataSet();
            code_value.SetFromDataElement(item_ds.GetDataElement(code_value.GetTag()));
            code_meaning.SetFromDataElement(item_ds.GetDataElement(code_meaning.GetTag()));
            if (code_value.GetValue() == "F-10310" && code_meaning.GetValue() == "prone")
              {
                patient_rotation = stir::PatientPosition::RotationValue::prone;
              }
            else if (code_value.GetValue() == "F-10340" && code_meaning.GetValue() == "supine")
              {
                patient_rotation = stir::PatientPosition::RotationValue::supine;
              }
            else if (code_value.GetValue() == "F-10317" && code_meaning.GetValue() == "right lateral decubitus")
              {
                patient_rotation = stir::PatientPosition::RotationValue::right;
              }
            else if (code_value.GetValue() == "F-10319" && code_meaning.GetValue() == "left lateral decubitus")
              {
                patient_rotation = stir::PatientPosition::RotationValue::left;
              }
          }
      }
    }

  stir::PatientPosition patient_position;
  patient_position.set_orientation(patient_gantry_orientation);
  patient_position.set_rotation(patient_rotation);

  return patient_position.get_position_as_string();
}

/*
    Copy all the necessary DICOM tags from the input file to the destination DataSet object.
    We currently print warning for missing attributes.
    See DICOM_defines.h for a list of the "necessary" tags.
*/
stir::Succeeded
insert_projection_attributes(gdcm::DataSet& dest_ds, const std::string& filename)
{

  gdcm::Reader reader;
  reader.SetFileName(filename.c_str());
  // Assert that we can open the file
  if (!reader.CanRead())
    {
      stir::error(boost::format("Cannot read projection data file %1%") % filename);
      return stir::Succeeded::no;
    }
  // Read the file
  reader.Read();

  // Attempt to copy all the required projection data attributes.
  // Special handling in specific cases.
  // This block is horribly ugly and repetitive, but I couldn't think of a way
  // to loop over the templated gdcm::Attribute<> class type since their args
  // make them all different class types and thus can't exist in container all together.
  // TODO Throw error for missing attributes of type 1 - Required.

  gdcm::DataElement de;
  gdcm::DataSet ds = reader.GetFile().GetDataSet();
  //    DCM_Specific_Character_Set
  DCM_Specific_Character_Set specific_character_set;
  if (check_tag_exists(specific_character_set.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(specific_character_set.GetTag());
      dest_ds.Insert(de);
    }

  //    DCM_Image_Type
  // Need to replace the type value TOMO with RECON TOMO
  DCM_Image_Type image_type;
  if (check_tag_exists(image_type.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(image_type.GetTag());
      image_type.SetFromDataElement(de);
      // If the type value "TOMO" exists, replace it with "RECON TOMO".
      // Otherwise, append "RECON TOMO".
      int value_id = image_type.GetNumberOfValues();
      for (auto idx = 0; idx < image_type.GetNumberOfValues(); ++idx)
      {
          std::string type_idx = std::string(image_type.GetValue(idx));
          if ( type_idx.compare( "TOMO" ) == 0) {
            value_id = idx;
            break;
          }
      }

      // Create a new attribute for this field that includes all the original values
      // as well as RECON TOMO.
      int num_values = (value_id != image_type.GetNumberOfValues()) ? image_type.GetNumberOfValues() : (image_type.GetNumberOfValues() + 1);
      DCM_Image_Type modified_image_type;
      modified_image_type.SetNumberOfValues(num_values);
      for (auto idx = 0; idx < num_values; ++idx)
      {
          if (idx == value_id)
          {
            modified_image_type.SetValue(idx, "RECON TOMO");
          }
          else
          {
            modified_image_type.SetValue(idx, image_type.GetValue(idx));
          }
      }

      dest_ds.Replace(modified_image_type.GetAsDataElement());
    }

  //    DCM_SOP_Class_UID
  DCM_SOP_Class_UID sop_class_uid;
  if (check_tag_exists(sop_class_uid.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(sop_class_uid.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Study_Date
  DCM_Study_Date study_date;
  if (check_tag_exists(study_date.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(study_date.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Acquisition_Date
  DCM_Acquisition_Date acquisition_date;
  if (check_tag_exists(acquisition_date.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(acquisition_date.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Series_Date
  DCM_Series_Date series_date;
  if (check_tag_exists(series_date.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(series_date.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Study_Time
  DCM_Study_Time study_time;
  if (check_tag_exists(study_time.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(study_time.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Acquisition_Time
  DCM_Acquisition_Time acquisition_time;
  if (check_tag_exists(acquisition_time.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(acquisition_time.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Series_Time
  DCM_Series_Time series_time;
  if (check_tag_exists(series_time.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(series_time.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Modality
  DCM_Modality modality;
  if (check_tag_exists(modality.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(modality.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Manufacturer
  DCM_Manufacturer manufacturer;
  if (check_tag_exists(manufacturer.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(manufacturer.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Manufacturers_Model_Name
  DCM_Manufacturers_Model_Name manufacturers_model_name;
  if (check_tag_exists(manufacturers_model_name.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(manufacturers_model_name.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Institution_Name
  DCM_Institution_Name institution_name;
  if (check_tag_exists(institution_name.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(institution_name.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Timezone_Offset_From_UTC
  DCM_Timezone_Offset_From_UTC timezone_offset_from_utc;
  if (check_tag_exists(timezone_offset_from_utc.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(timezone_offset_from_utc.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Station_Name
  DCM_Station_Name station_name;
  if (check_tag_exists(station_name.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(station_name.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Study_Description
  DCM_Study_Description study_description;
  if (check_tag_exists(study_description.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(study_description.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Counts_Accumulated
  DCM_Counts_Accumulated counts_accumulated;
  if (check_tag_exists(counts_accumulated.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(counts_accumulated.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Acquisition_Termination_Condition
  DCM_Acquisition_Termination_Condition acquisition_termination_condtion;
  if (check_tag_exists(acquisition_termination_condtion.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(acquisition_termination_condtion.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Software_Versions
  DCM_Software_Versions software_versions;
  if (check_tag_exists(software_versions.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(software_versions.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Protocol_Name
  DCM_Protocol_Name protocol_name;
  if (check_tag_exists(protocol_name.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(protocol_name.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Date_of_Last_Calibration
  DCM_Date_of_Last_Calibration date_of_last_calibration;
  if (check_tag_exists(date_of_last_calibration.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(date_of_last_calibration.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Time_of_Last_Calibration
  DCM_Time_of_Last_Calibration time_of_last_calibration;
  if (check_tag_exists(time_of_last_calibration.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(time_of_last_calibration.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Procedure_Code_Sequence
  DCM_Procedure_Code_Sequence procedure_code_sequence;
  if (check_tag_exists(procedure_code_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(procedure_code_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Anatomic_Region_Sequence
  DCM_Anatomic_Region_Sequence anatomic_region_sequence;
  if (check_tag_exists(anatomic_region_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(anatomic_region_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patients_Name
  DCM_Patients_Name patient_name;
  if (check_tag_exists(patient_name.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_name.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patient_ID
  DCM_Patient_ID patient_id;
  if (check_tag_exists(patient_id.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_id.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Issuer_of_Patient_ID
  DCM_Issuer_of_Patient_ID issuer_of_patient_id;
  if (check_tag_exists(issuer_of_patient_id.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(issuer_of_patient_id.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Issuer_of_Patient_ID_Qualifiers_Sequence
  DCM_Issuer_of_Patient_ID_Qualifiers_Sequence issuer_of_patient_id_qualifiers_sequence;
  if (check_tag_exists(issuer_of_patient_id_qualifiers_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(issuer_of_patient_id_qualifiers_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patients_Sex
  DCM_Patients_Sex patient_sex;
  if (check_tag_exists(patient_sex.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_sex.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Other_Patient_IDs
  DCM_Other_Patient_IDs other_patient_ids;
  if (check_tag_exists(other_patient_ids.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(other_patient_ids.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patients_Age
  DCM_Patients_Age patient_age;
  if (check_tag_exists(patient_age.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_age.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Intervention_Drug_Information_Sequence
  DCM_Intervention_Drug_Information_Sequence drug_information_sequence;
  if (check_tag_exists(drug_information_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(drug_information_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Study_Instance_UID
  DCM_Study_Instance_UID study_instance_uid;
  if (check_tag_exists(study_instance_uid.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(study_instance_uid.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Study_ID
  DCM_Study_ID study_id;
  if (check_tag_exists(study_id.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(study_id.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Series_Number
  DCM_Series_Number series_number;
  if (check_tag_exists(series_number.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(series_number.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Instance_Number
  DCM_Instance_Number instance_number;
  if (check_tag_exists(instance_number.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(instance_number.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Frame_of_Reference_UID
  DCM_Frame_of_Reference_UID frame_of_reference_uid;
  if (check_tag_exists(frame_of_reference_uid.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(frame_of_reference_uid.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Position_Reference_Indicator
  DCM_Position_Reference_Indicator position_reference_indicator;
  if (check_tag_exists(position_reference_indicator.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(position_reference_indicator.GetTag());
      dest_ds.Insert(de);
    }
  // DCM_Corrected_Image
  // Note that this field will be modified later on, but is required here as an initialization.
  DCM_Corrected_Image corrected_image;
  if (check_tag_exists(corrected_image.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(corrected_image.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Requesting_Physician
  DCM_Requesting_Physician requesting_physician;
  if (check_tag_exists(requesting_physician.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(requesting_physician.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Requested_Procedure_Description
  DCM_Requested_Procedure_Description requested_procedure_code_description;
  if (check_tag_exists(requested_procedure_code_description.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(requested_procedure_code_description.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Requested_Procedure_Code_Sequence
  DCM_Requested_Procedure_Code_Sequence requested_procedure_code_sequence;
  if (check_tag_exists(requested_procedure_code_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(requested_procedure_code_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Performed_Procedure_Step_Start_Date
  DCM_Performed_Procedure_Step_Start_Date performed_procedure_step_start_date;
  if (check_tag_exists(performed_procedure_step_start_date.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(performed_procedure_step_start_date.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Performed_Procedure_Step_Start_Time
  DCM_Performed_Procedure_Step_Start_Time performed_procedure_step_start_time;
  if (check_tag_exists(performed_procedure_step_start_time.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(performed_procedure_step_start_time.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Performed_Procedure_Step_ID
  DCM_Performed_Procedure_Step_ID performed_procedure_step_id;
  if (check_tag_exists(performed_procedure_step_id.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(performed_procedure_step_id.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Performed_Procedure_Step_Description
  DCM_Performed_Procedure_Step_Description performed_procedure_step_description;
  if (check_tag_exists(performed_procedure_step_description.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(performed_procedure_step_id.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Request_Attributes_Sequence
  DCM_Request_Attributes_Sequence request_attributes_sequence;
  if (check_tag_exists(request_attributes_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(request_attributes_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Requested_Procedure_Priority
  DCM_Requested_Procedure_Priority requested_procedure_priority;
  if (check_tag_exists(requested_procedure_priority.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(requested_procedure_priority.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Filler_Order_Number_Imaging_Service_Request
  DCM_Filler_Order_Number_Imaging_Service_Request filler_order_number_imaging_service_request;
  if (check_tag_exists(filler_order_number_imaging_service_request.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(filler_order_number_imaging_service_request.GetTag());
      dest_ds.Insert(de);
    }
  // FOR TESTING  DCM_Energy_Window_Information_Sequence
  DCM_Energy_Window_Information_Sequence energy_window_information_sequence;
  if (check_tag_exists(energy_window_information_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(energy_window_information_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Radiopharmaceutical_Information_Sequence
  DCM_Radiopharmaceutical_Information_Sequence radiopharmaceutical_information_sequence;
  if (check_tag_exists(radiopharmaceutical_information_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(radiopharmaceutical_information_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Detector_Information_Sequence // Copy only the first item
  // Copy only the first item in this sequence.
  // Achieved by removing all items with ids greater than 1.
  DCM_Detector_Information_Sequence detector_information_sequence;
  if (check_tag_exists(detector_information_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(detector_information_sequence.GetTag());
      gdcm::SmartPointer<gdcm::SequenceOfItems> sqi = de.GetValueAsSQ();
      if (sqi->GetNumberOfItems() > 1)
        {
          for (int sequence_idx = sqi->GetNumberOfItems(); sequence_idx > 1; sequence_idx--)
            {
              sqi->RemoveItemByIndex(sequence_idx);
            }
        }
      // Now copy the residual sequence.
      dest_ds.Insert(de);
    }

  //    DCM_Rotation_Vector
  DCM_Rotation_Vector rotation_vector;
  if (check_tag_exists(rotation_vector.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(rotation_vector.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Number_of_Rotations
  DCM_Number_of_Rotations number_of_rotations;
  if (check_tag_exists(number_of_rotations.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(number_of_rotations.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Rotation_Information_Sequence
  DCM_Rotation_Information_Sequence rotation_information_sequence;
  if (check_tag_exists(rotation_information_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(rotation_information_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Type_of_Detector_Motion
  DCM_Type_of_Detector_Motion type_of_detector_motion;
  if (check_tag_exists(type_of_detector_motion.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(type_of_detector_motion.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patient_Orientation_Code_Sequence
  DCM_Patient_Orientation_Code_Sequence patient_orientation_code_sequence;
  if (check_tag_exists(patient_orientation_code_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_orientation_code_sequence.GetTag());
      dest_ds.Insert(de);
    }
  //    DCM_Patient_Gantry_Relationship_Code_Sequence
  DCM_Patient_Gantry_Relationship_Code_Sequence patient_gantry_relationship_code_sequence;
  if (check_tag_exists(patient_gantry_relationship_code_sequence.GetTag(), reader) == stir::Succeeded::yes)
    {
      de = ds.GetDataElement(patient_gantry_relationship_code_sequence.GetTag());
      dest_ds.Insert(de);
    }

  if (dest_ds.IsEmpty())
    {
      return stir::Succeeded::no;
    }
  return stir::Succeeded::yes;
}

void
insert_image_attributes(gdcm::DataSet& dest_ds, const std::string& filename)
{
    // TODO have this function return a stir::Succeeded value and implement some error handling.
    
    // Read the file into memory
    stir::VoxelsOnCartesianGrid<float>* image_ptr = stir::read_interfile_image(filename); // For read static images - not dynamic/parametric.
    // A bit ugly but we need to read the image' file's header again to get the image scaling factors from the header.
    stir::InterfileImageHeader hdr; // IS THIS BEING USED?
    hdr.parse(filename, false);

    // Update the gdcm::DataSet with the image's attributes

    // DCM_Pixel_Spacing
    // Need the in-plane spacing values: "<width>\<height>"
    stir::BasicCoordinate<3, float> spacing = image_ptr->get_grid_spacing(); // 1-based indices.
       
    DCM_Pixel_Spacing pixel_spacing;
    pixel_spacing.SetValue(spacing[1], 0);
    pixel_spacing.SetValue(spacing[2], 1);
    dest_ds.Insert(pixel_spacing.GetAsDataElement());

    DCM_Slice_Thickness slice_thickness;
    slice_thickness.SetValue(std::fabs(spacing[3]));
    dest_ds.Insert(slice_thickness.GetAsDataElement());

    // DCM_Rows
    DCM_Rows rows;
    rows.SetValue(image_ptr->get_x_size());
    dest_ds.Insert(rows.GetAsDataElement());

    // DCM_Columns
    DCM_Columns columns;
    columns.SetValue(image_ptr->get_y_size());
    dest_ds.Insert(columns.GetAsDataElement());

    // DCM_Number_of_Frames
    DCM_Number_of_Frames num_frames;
    num_frames.SetValue(image_ptr->get_z_size());
    dest_ds.Insert(num_frames.GetAsDataElement());

    // DCM_Samples_per_Pixel
    DCM_Samples_per_Pixel samples_per_pixel;
    samples_per_pixel.SetValue(1);
    dest_ds.Insert(samples_per_pixel.GetAsDataElement());

    // DCM_Photometric_Interpretation
    DCM_Photometric_Interpretation photometric_interpretation;
    photometric_interpretation.SetValue("MONOCHROME2");
    dest_ds.Insert(photometric_interpretation.GetAsDataElement());

    // DCM_Pixel_Representation
    // Store as unsigned.
    DCM_Pixel_Representation pixel_representation;
    pixel_representation.SetValue(0);
    dest_ds.Insert(pixel_representation.GetAsDataElement());

    // DCM_Bits_Allocated
    DCM_Bits_Allocated bits_allocated;
    bits_allocated.SetValue(8 * sizeof(uint16_t));
    dest_ds.Insert(bits_allocated.GetAsDataElement());
          
    // DCM_Bits_Stored
    DCM_Bits_Stored bits_stored;
    bits_stored.SetValue(8 * sizeof(uint16_t));
    dest_ds.Insert(bits_stored.GetAsDataElement());

    // DCM_High_Bit
    DCM_High_Bit high_bit;
    high_bit.SetValue(8 * sizeof(uint16_t) - 1);
    dest_ds.Insert(high_bit.GetAsDataElement());

    // DCM_Pixel_Data
    // Generate an image buffer
    // Note that on load this buffer will have been rescaled to the contain the real pixel values.
    // TODO We may need to scale once again, for storage with 16-bit pixel depth.
    int number_of_voxels = image_ptr->get_x_size() * image_ptr->get_y_size() * image_ptr->get_z_size();
    char* buffer = new char[number_of_voxels * sizeof(uint16_t)];
    char* p = buffer;
            
    for (auto& itr = image_ptr->begin_all(); itr != image_ptr->end_all(); itr++)
        {
        uint16_t val = uint16_t(*itr);
        void* val_ptr = (void*)&val;
        memcpy(p, val_ptr, sizeof(uint16_t));
        p += sizeof(uint16_t);
        }
    //DCM_Pixel_Data pixel_data; // This is failing a gdcm assertion for some reason...
    //pixel_data.SetNumberOfValues(number_of_voxels);
    gdcm::Tag pixel_data_tag(0x7fe0, 0x0010);
    gdcm::DataElement pixel_data(pixel_data_tag);
    pixel_data.SetByteValue(buffer, (uint32_t)(number_of_voxels * sizeof(uint16_t)));
    delete[] buffer;

    // Pass the image buffer to the gdcm::Image object.
    //DICOM_image_data.SetDataElement(pixel_data.GetAsDataElement());
    dest_ds.Insert(pixel_data);

    // Content Date/Time
    // TODO the recon should write the time of the file creation into the image file (interfile).
    // IF that's the case we should be setting this value in insert_image_data().
    // For now we'll just get the current date and time.
    time_t now = time(0);
    struct tm tstruct;
    char date_buf[100], time_buf[100];
    tstruct = *localtime(&now);
    strftime(date_buf, sizeof(date_buf), "%Y%m%d", &tstruct); // YYYYMMDD, e.g. "20240515" for May 15, 2024.
    strftime(time_buf, sizeof(time_buf), "%H%M%S", &tstruct); // HHMMSS, (24-hour representation) e.g. "142034" for 2:20:34

    DCM_Content_Date content_date = { date_buf };
    dest_ds.Insert(content_date.GetAsDataElement());

    DCM_Content_Time content_time = { time_buf };
    dest_ds.Insert(content_time.GetAsDataElement());

    // DCM_Number_of_Slices
    // Required if the SLice_Vector is indicated by the Frame Increment Pointer (as we have).
    DCM_Number_of_Slices number_of_slices;
    number_of_slices.SetValue(image_ptr->get_z_size());
    dest_ds.Insert(number_of_slices.GetAsDataElement());

    // DCM_Slice_Vector
    // An array of values indicating the slice number for each frame.
    // The vector length is equal to the number of frames in the image.
    std::vector<uint16_t> slice_vector_data(image_ptr->get_z_size());
    std::iota(slice_vector_data.begin(), slice_vector_data.end(), 1);
    DCM_Slice_Vector slice_vector;
    slice_vector.SetNumberOfValues(slice_vector_data.size());
    for (auto idx = 0; idx < slice_vector_data.size(); idx++)
        {
        slice_vector.SetValue(idx, slice_vector_data.at(idx));
        }
    dest_ds.Insert(slice_vector.GetAsDataElement());

    // DCM_Frame_Increment_Pointer
    // Indication of the tag(s) that describes the intended interpretation of each slice.
    // For non-gated/non-dynamic images, the Slice Vector alone would indicate the spatial slice
    // number of each frame.
    DCM_Frame_Increment_Pointer frame_increment_pointer;
    frame_increment_pointer.SetNumberOfValues(1);
    frame_increment_pointer.SetValue(slice_vector.GetTag()); // DCM_Slice_Vector
    dest_ds.Insert(frame_increment_pointer.GetAsDataElement());

    // DCM_Smallest_Image_Pixel_Value
    DCM_Smallest_Image_Pixel_Value smallest_pixel_value;

    float smallest_pixel_value_float = image_ptr->find_min();
    smallest_pixel_value.SetNumberOfValues(1);
    smallest_pixel_value.SetValue(uint16_t(std::round(smallest_pixel_value_float)));
    dest_ds.Insert(smallest_pixel_value.GetAsDataElement());

    // DCM_Largest_Image_Pixel_Value
    DCM_Largest_Image_Pixel_Value largest_pixel_value;
    float largest_pixel_value_float = image_ptr->find_max();
    largest_pixel_value.SetNumberOfValues(1);
    largest_pixel_value.SetValue(uint16_t(std::round(largest_pixel_value_float)));
    dest_ds.Insert(largest_pixel_value.GetAsDataElement());

    // DCM_Window_Center
    float window_center_float = 0.5 * (largest_pixel_value_float + smallest_pixel_value_float);          
    DCM_Window_Center window_center;
    window_center.SetNumberOfValues(1);
    window_center.SetValue(window_center_float);
    dest_ds.Insert(window_center.GetAsDataElement());

    // DCM_Window_Width
    float window_width_float = (largest_pixel_value_float - smallest_pixel_value_float);
    DCM_Window_Width window_width;
    window_width.SetNumberOfValues(1);
    window_width.SetValue(window_width_float);
    dest_ds.Insert(window_width.GetAsDataElement());
             
    // DCM_Number_of_Detectors
    DCM_Number_of_Detectors number_of_detectors;
    number_of_detectors.SetValue(1);
    dest_ds.Insert(number_of_detectors.GetAsDataElement());

    // Not sure we can get it from the projection data or not.
    // Should easy if the projection data only has 1 window.
    // But if it's multiple (which is common) it may be difficult to determine whcih item in the sequence
    // correspond to the photopeak window.
    // Alternatively, the interfile format has attributes for this as well and it should be
    // accessible from the iamge_ptr's ExamInfo object, but these values don't seem to be 
    // included in the output images atm. 
    
    // DCM_Number_of_Energy_Windows
    DCM_Number_of_Energy_Windows number_of_energy_windows;
    number_of_energy_windows.SetValue(1); // Always 1.
    dest_ds.Insert(number_of_energy_windows.GetAsDataElement());

    // DCM_Energy_Window_Information_Sequence;
    // Note that following structure for this its sequence item:
    // ITEM 1
    // > Energy WIndow Range Sequence
    // >> ITEM 1
    // >> Energy Window Lower Limit
    // >> Energy Window Upper Limit
    // >> End ITEM 1
    // > Energy Window Name
    // End ITEM 1
    // Of note is the nested sequence.
 
    // Energy Window Range Sequence
    DCM_Energy_Window_Lower_Limit energy_window_lower_limit;
    energy_window_lower_limit.SetValue(image_ptr->get_exam_info().get_low_energy_thres());
    DCM_Energy_Window_Upper_Limit energy_window_upper_limit;
    energy_window_upper_limit.SetValue(image_ptr->get_exam_info().get_high_energy_thres());

    // Create an item for the Energy Window Range Sequence and insert the above data elements.
    gdcm::Item it_ewrs;
    it_ewrs.SetVLToUndefined();
    gdcm::DataSet& nds_ewrs = it_ewrs.GetNestedDataSet();
    nds_ewrs.Insert(energy_window_lower_limit.GetAsDataElement());
    nds_ewrs.Insert(energy_window_upper_limit.GetAsDataElement());

    // Create a sequence of items for the Energy Window Range Sequence and insert its item
    gdcm::SmartPointer<gdcm::SequenceOfItems> sq_ewrs = new gdcm::SequenceOfItems();
    sq_ewrs->SetLengthToUndefined();
    sq_ewrs->AddItem(it_ewrs);

    DCM_Energy_Window_Range_Sequence energy_window_range_sequence;
    gdcm::DataElement de_sq_ewrs(energy_window_range_sequence.GetTag());
    de_sq_ewrs.SetVR(gdcm::VR::SQ);
    de_sq_ewrs.SetValue(*sq_ewrs);
    de_sq_ewrs.SetVLToUndefined();

    // Create and item for the Energy_Window_Information_Sequence and insert its elements
    // Energy Window Name
    std::string radionuclide_name = image_ptr->get_exam_info().get_radionuclide().get_name();
    DCM_Energy_Window_Name energy_window_name = { radionuclide_name };

    gdcm::Item it_ewis;
    it_ewis.SetVLToUndefined();
    gdcm::DataSet& nds_ewis = it_ewis.GetNestedDataSet();
    nds_ewis.Insert(de_sq_ewrs);
    nds_ewis.Insert(energy_window_name.GetAsDataElement());

    // Create a Sequence of items for the Energy Window Information Sequence and insert its item
    gdcm::SmartPointer<gdcm::SequenceOfItems> sq_ewis = new gdcm::SequenceOfItems();
    sq_ewis->SetLengthToUndefined();
    sq_ewis->AddItem(it_ewis);

    // Define sequence attribute and initialize it with the sequence of items.
    DCM_Energy_Window_Information_Sequence energy_window_information_sequence;
    gdcm::DataElement de_sq_ewis(energy_window_information_sequence.GetTag());
    de_sq_ewis.SetVR(gdcm::VR::SQ);
    de_sq_ewis.SetValue(*sq_ewis);
    de_sq_ewis.SetVLToUndefined();

    // Replace sequence in data set (in case there was an existing entry).
    dest_ds.Replace(de_sq_ewis);

    // TODO Determine why the energy window information doesn't survive after using exe's like manip_projdata or sitr_math
    // when subtracting the scatter data from the photopeak data.

    // The following two attributes are nested in the detector information sequence for the NM IOD.
    // We will try to insert these attributes in that sequence, if possible.
    // Otherwise we insert them at the root level.
    // We'll instantiate the gdcm::Attribute object for this sequence to access its tag.
    DCM_Detector_Information_Sequence detector_information_sequence; // We only need this for the tag - it has/needs no data.

    // DCM_Image_Position_Patient
    // TODO The origin alone seem like the center of the volume (0,0,0), probably need to offset this to the 
    // position of the first voxel.
    DCM_Image_Position_Patient image_position_patient;
    image_position_patient.SetValue(image_ptr->get_origin().x(), 0); // left-right coord
    image_position_patient.SetValue(image_ptr->get_origin().y(), 1); // anterior-posterior coord
    image_position_patient.SetValue(image_ptr->get_origin().z(), 2); // inferior-superior  coord.
    
    if (replace_sequennce_item_data_element(dest_ds, detector_information_sequence.GetTag(), image_position_patient.GetAsDataElement(), 1) == stir::Succeeded::no)
        {
        dest_ds.Insert(image_position_patient.GetAsDataElement());
        }

    // Patient-Image Orientation Dependent Attributes
    // For all of the following, we assume that the patient is Recumbent.
    // Determine the patient-gantry orientation
    stir::PatientPosition::OrientationValue patient_gantry_orientation = stir::PatientPosition::OrientationValue::head_in; // Fixed for now.

    // THIS RELATIONSHIP DOESN'T SEEM TO BE CORRECT
    // Headfirst != Negative PSacing Between Slices
    // Feet-first != Positive Spacing Between Slices.
    DCM_Spacing_Between_Slices spacing_between_slices; // Required, empty if unknown.
    if (patient_gantry_orientation == stir::PatientPosition::OrientationValue::head_in)
        {
        spacing_between_slices.SetValue(-spacing[3]);
        dest_ds.Insert(spacing_between_slices.GetAsDataElement());
        }
    else if (patient_gantry_orientation == stir::PatientPosition::OrientationValue::feet_in)
        {
        spacing_between_slices.SetValue(spacing[3]);
        dest_ds.Insert(spacing_between_slices.GetAsDataElement());
        }
    
    // Determine the patient 
    // DCM_Image_Orientation_Patient
    // TODO Not sure how to generate this from the stir image...
    // Perhaps from the exam info's patient position field, which seems to contains "orientation" and "rotation"
    // Although it doesn't seem to be set for my current test case.
    std::string patient_position = "";
    patient_position = image_ptr->get_exam_info().patient_position.get_position_as_string();
    // Try to get it from the Patient Position attribute.
    // If that fails then from the Patient Orientation Code (Supine, prone, Left/Right Decubitus) Sequence and Patient Gantry Relationship Code Sequence (Head/Feet First)
    if (patient_position.compare("unknown") == 0)
        {
            patient_position = get_patient_position_as_string(dest_ds);
        }

    float iop[6] = { 1., 0., 0., 0., 1., 0. }; // Initialize as recumbent/supine.
    if (patient_position.compare("HFS") == 0)
        {
        }
    else if (patient_position.compare("HFP") == 0)
        {
        }
    else if (patient_position.compare("HFDR") == 0)
        {
        }
    else if (patient_position.compare("HFDL") == 0)
        {
        }
    else if (patient_position.compare("FFS") == 0)
        {
        }
    else if (patient_position.compare("FFP") == 0)
        {
        }
    else if (patient_position.compare("FFDR") == 0)
        {
        }
    else if (patient_position.compare("FFDL") == 0)
        {
        }
    else
        {
        // Not defined/Not supported
        }
    
    DCM_Image_Orientation_Patient image_orientation_patient;
    // THESE VALUES ARE MADE UP FOR TESTING - Transverse supine
    for (int idx = 0; idx < 6; idx++)
        {
            image_orientation_patient.SetValue(iop[idx], idx);
        }

    if (replace_sequennce_item_data_element( dest_ds, detector_information_sequence.GetTag(), image_orientation_patient.GetAsDataElement(), 1) == stir::Succeeded::no)
        {
        dest_ds.Insert(image_position_patient.GetAsDataElement());
        }
 
}

std::string
trim_parameter_value(std::string str)
{
    std::string delimiter = ":=";

    // Trim the line to include only the rhs of the delimiter.
    size_t trim_pos = str.find(delimiter);
    if (std::string::npos != trim_pos)
        {
        trim_pos += 2;
        str = str.substr(trim_pos, str.length() - trim_pos);
        }

    // Trim any preceding white spacing
    trim_pos = str.find_first_not_of(' ');
    if (std::string::npos != trim_pos)
        {
        str = str.substr(trim_pos, str.length() - trim_pos);
        }

    // Trim any trailing white space
    trim_pos = str.find_last_not_of(' ');
    if (std::string::npos != trim_pos)
        {
        trim_pos += 1;
        str = str.substr(0, trim_pos);
        }

    return str;
   
}

// A helper function to read the recon parameter file and extract specific values that are
// important for the header of the DICOM output.
void
parse_reconstruction_parameters(std::unordered_map<std::string, std::string>& parameter_map, const std::string& filename)
{
    std::ifstream if_stream;
    if_stream.open(filename);

    // Generate a list of the keywords
    std::set<std::string> keys;
    keys.insert(def_psf_type);
    keys.insert(def_atteunation_type);
    keys.insert(def_scatter_type);
    keys.insert(def_prior_type);
    keys.insert(def_post_filter);
    keys.insert(def_x_filter_FWHM);
    keys.insert(def_y_filter_FWHM);
    keys.insert(def_z_filter_FWHM);
    keys.insert(def_num_subiterations);
    keys.insert(def_num_subsets);
    keys.insert(def_image_description);
    keys.insert(def_image_id);

    // Loop over every line in the file.
    // For each search for one of the keywords we care about.
    // If a keyword is detected, store the key-value pair in the parameter_map
    // and then remove that key from the original set so that we don't continue to search for it.
    std::string value;
    for (std::string line; std::getline(if_stream, line);)
        {
        // Loop over all keywords
        for (auto& _key : keys)
            {
              if (line.find(_key) != line.npos)
                {
                  value = trim_parameter_value(line);
                  parameter_map.insert(std::make_pair(_key, value));
                  // Remove this key from the set of keywords
                  // to shorted our search for subsequent lines.
                  keys.erase(_key);
                  break;
                }
            }
        }

    // For all remaining keys, store a value of "None" in the map.
    for (auto& _key : keys)
        {
        parameter_map.insert(std::make_pair(_key, "None"));
        }
}

void
insert_reconstruction_attributes(gdcm::DataSet& dest_ds, const std::string& filename)
{
    std::unordered_map<std::string, std::string> parameter_map;
    parse_reconstruction_parameters(parameter_map, filename);
   
    int num_iterations = atoi(parameter_map[def_num_subiterations].c_str());
    int num_subsets = atoi(parameter_map[def_num_subsets].c_str());
    
    // Alternative - extract the recon param info using STIR key parsers.
    // stir::OSMAPOSLReconstruction<stir::DiscretisedDensity<3, float>> reconstruction_object(filename);
    // int num_iterations = reconstruction_object.get_num_subiterations();
    // int num_subsets = reconstruction_object.get_num_subsets();
    // Extract the psf_type and attenuatuon_type parameters from the ProjMatrixByBinSPECTUB class.
    //stir::ProjMatrixByBinSPECTUB const* proj_matrix
    //    = reconstruction_object.get_objective_function_test().get_projector_pair_test().get_proj_matrix_test_ptr();

    std::string psf_type = parameter_map[def_psf_type];
    std::string attenuation_type = parameter_map[def_atteunation_type];
    std::string prior_type = parameter_map[def_prior_type];
    std::string scatter_type = parameter_map[def_scatter_type];
    std::string post_filter_type = parameter_map[def_post_filter];
    
    // Convolution_Kernel - Specifies the recon method and corrections it includes
    // E.g. OSEM 16i8s AC PSF \\ Separable Gaussian 10mm,10mm,10mm
    std::string convolution_kernel_recon_str = "";
    if (prior_type == "None")
        {
        convolution_kernel_recon_str += std::string("OSEM");
        }
    else
        {
        convolution_kernel_recon_str += std::string("OSMAPOSL");
        }
    std::string iteration_str = string_format(" %di%ds", num_iterations, num_subsets);
    convolution_kernel_recon_str += iteration_str;

    if (attenuation_type != "No")
        {
        convolution_kernel_recon_str += std::string(" AC");
        }

    if (psf_type != "Geometrical") // TODO check if we should expect lower case here.
        {
        convolution_kernel_recon_str += std::string(" PSF");
        }

    std::string convolution_kernel_filter_str = "";
    if (post_filter_type != "None")
        {
        if (post_filter_type.find("Gaussian") != std::string::npos)
            {
              convolution_kernel_filter_str = "Gauss 3D ";
              convolution_kernel_filter_str += (parameter_map[def_x_filter_FWHM] + std::string(", "));
              convolution_kernel_filter_str += (parameter_map[def_y_filter_FWHM] + std::string(", "));
              convolution_kernel_filter_str += (parameter_map[def_z_filter_FWHM] + std::string(" "));
              convolution_kernel_filter_str += std::string("mm");
            }
        }

    DCM_Convolution_Kernel convolution_kernel;
    convolution_kernel.SetNumberOfValues(2);
    convolution_kernel.SetValue(0,convolution_kernel_recon_str.c_str());
    convolution_kernel.SetValue(1, convolution_kernel_filter_str.c_str());
    dest_ds.Insert(convolution_kernel.GetAsDataElement());

    // Corrected Image
    // Check what already included in the dataset from the projection data, e.g. UNIF, COR, ATTN, SCAT, etc.
    // TODO The recon param file should be modified to indicate if the projection data used as input have been
    // corrected for scatter. When that is in place we can retrieve that information from the reconstruction object
    // directly.
    // We'll hard code the value for now.
    DCM_Corrected_Image corrected_image;
    gdcm::DataElement corrected_image_de = dest_ds.GetDataElement(corrected_image.GetTag());
    const gdcm::ByteValue* bv = corrected_image_de.GetByteValue();
    if (bv != nullptr)
        {
        corrected_image.SetFromDataElement(dest_ds.GetDataElement(corrected_image.GetTag()));
        }
    // Story existing values in a vector
    int value_multiplicity = corrected_image.GetNumberOfValues();
    std::vector<gdcm::CSComp> corrected_image_str_vector;
    for (int idx = 0; idx < value_multiplicity; idx++)
        {
        corrected_image_str_vector.push_back(corrected_image.GetValue(idx));
        }

    // Prepend the strings that we need.
    if (attenuation_type != "No" && attenuation_type != "None")
        {
        corrected_image_str_vector.insert(corrected_image_str_vector.begin(), "ATTN");
        }
    if (scatter_type != "No" && scatter_type != "None")
        {
        corrected_image_str_vector.insert(corrected_image_str_vector.begin(), "SCAT");
        }

    value_multiplicity = (int)corrected_image_str_vector.size();
    corrected_image.SetNumberOfValues(value_multiplicity);
    gdcm::CSComp* corrected_image_array = new gdcm::CSComp[value_multiplicity];
    int idx = 0;
    for (auto& _str : corrected_image_str_vector)
        {
        corrected_image_array[idx] = _str;
        idx++;
        }
    corrected_image.SetValues(corrected_image_array, value_multiplicity);
    // Since this field was already initialized in the dest DataSet we need to replace, rather than insert this element.
    dest_ds.Replace(corrected_image.GetAsDataElement());
    
    // Generate our own UIDs using the Convergent ROOT
    gdcm::UIDGenerator uid_generator;
    uid_generator.SetRoot("1.2.840.114202"); // SITE ROOT USED BY CONVERGENT
    // SOP Instance UID
    DCM_SOP_Instance_UID sop_instance_uid;
    const char* sopi_uid = uid_generator.Generate();
    sop_instance_uid.SetValue(sopi_uid);
    dest_ds.Insert(sop_instance_uid.GetAsDataElement());

    // Series Instance UID
    DCM_Series_Instance_UID series_instance_uid;
    const char* si_uid = uid_generator.Generate();
    series_instance_uid.SetValue(si_uid);
    dest_ds.Insert(series_instance_uid.GetAsDataElement());

    // Series Description
    // TODO We'll need to make change to the recon parameter file to support such a field. 
    std::string series_description_str = "SPECT Recon";
    if (parameter_map[def_image_description] != "None")
        {
        series_description_str = parameter_map[def_image_description];
        }
    DCM_Series_Description series_description = { series_description_str.c_str() };
    dest_ds.Insert(series_description.GetAsDataElement());

    // Image ID
    // For the NM IOD, this attribute is nested in the Detector Information Sequence.
    // We'll insert the value there if possible, otherwise we'll write it to the root data set.
    // TODO the recon param file should be modified to include a parameter for this attribute.
    if (parameter_map[def_image_id] != "None")
        {
        std::string image_id_str = parameter_map[def_image_id];
        DCM_Image_ID image_id = { image_id_str.c_str() };

        DCM_Detector_Information_Sequence detector_information_sequence; // We only need this for the tag - it has/needs no data.
        if (replace_sequennce_item_data_element(dest_ds, detector_information_sequence.GetTag(), image_id.GetAsDataElement(), 1)
            == stir::Succeeded::no)
            {
              dest_ds.Insert(image_id.GetAsDataElement());
            }
        }    

    // Contributing_Equipment_Sequence
    // Note on the structure of the item in this sequence:
    // ITEM 1
    // > Manufacturer
    // > Manufacturer Model Name
    // > Contribution Date Time
    // > Software Versions
    // > Contribution Description
    // > Purpose of Reference Code Sequence
    // >> ITEM 1
    // >> DCM_Code_Meaning 
    // >> DCM_Code_Value 
    // >> DCM_Coding_Scheme_Designator 
    // >> DCM_Coding_Scheme_Version
    // > End ITEM 1
    // End ITEM 1
    DCM_Manufacturer contributing_manufacturer = { "Convergent Imaging Solutions" };
    DCM_Manufacturers_Model_Name manufacturer_model_name = { "UniSyn MI - Reveal" };
    DCM_Contribution_Description contribution_description = { "Tomographic Reconstruction" };
    DCM_Software_Versions software_versions;
    software_versions.SetNumberOfValues(1);
    software_versions.SetValue(0, "1.0");

    DCM_Contribution_Date_Time contirbuting_date_time;
    time_t now = time(0);
    struct tm tstruct;
    char datetime_buf[100];
    tstruct = *localtime(&now);
    strftime(datetime_buf, sizeof(datetime_buf), "%Y%m%d%H%M%S", &tstruct); // YYYYMMDDHHMMSS, e.g. "20240515142034" for May 15, 2024 at 2:20:34 pm
    contirbuting_date_time.SetValue(datetime_buf);
    
    DCM_Purpose_of_Reference_Code_Sequence purpose_of_reference_code_sequence;
    /*
    * Populate this sequence as per:
    * Coding Scheme Designator "DCM" Coding Scheme Version "01"
    * 113961 "Reconstruction Algorithm" - Description of the algorithm used when reconstructing the image from the data acquired during the acquisition process.
    */
    DCM_Code_Meaning code_meaning = { "Reconstruction Algorithm" };
    DCM_Code_Value code_value = { "113961" };
    DCM_Coding_Scheme_Designator code_scheme_designator = { "DCM" };
    DCM_Coding_Scheme_Version code_scheme_version = { "1.0" };

    // Create the single item for the Purpose of Reference Code Sequence and insert its element.
    gdcm::Item it_prcs;
    it_prcs.SetVLToUndefined();
    gdcm::DataSet& nds_prcs = it_prcs.GetNestedDataSet();
    nds_prcs.Insert(code_meaning.GetAsDataElement());
    nds_prcs.Insert(code_value.GetAsDataElement());
    nds_prcs.Insert(code_scheme_designator.GetAsDataElement());
    nds_prcs.Insert(code_scheme_version.GetAsDataElement());

    // Create DataElement contains a sequence of items for the Purpose of Reference Code Sequence
    gdcm::SmartPointer<gdcm::SequenceOfItems> sq_prcs = new gdcm::SequenceOfItems();
    sq_prcs->SetLengthToUndefined();
    sq_prcs->AddItem(it_prcs);

    gdcm::DataElement de_sq_prcs(purpose_of_reference_code_sequence.GetTag());
    de_sq_prcs.SetVR(gdcm::VR::SQ);
    de_sq_prcs.SetValue(*sq_prcs);
    de_sq_prcs.SetVLToUndefined();

    // Create the single item for the ontributing_Equipment_Sequence and insert its elements.
    gdcm::Item it_ces;
    it_ces.SetVLToUndefined();
    gdcm::DataSet& nds_ces = it_ces.GetNestedDataSet();
    nds_ces.Insert(contributing_manufacturer.GetAsDataElement());
    nds_ces.Insert(manufacturer_model_name.GetAsDataElement());
    nds_ces.Insert(software_versions.GetAsDataElement());
    nds_ces.Insert(contribution_description.GetAsDataElement());
    nds_ces.Insert(contirbuting_date_time.GetAsDataElement());
    nds_ces.Insert(de_sq_prcs);

    // Create a Sequence and insert the one item
    gdcm::SmartPointer<gdcm::SequenceOfItems> sq_ces = new gdcm::SequenceOfItems();
    sq_ces->SetLengthToUndefined();
    sq_ces->AddItem(it_ces);

    // Define sequence attribute and initialize it with the sequence of items.
    DCM_Contributing_Equipment_Sequence contributing_equipment_sequence;
    gdcm::DataElement de_sq_ces(contributing_equipment_sequence.GetTag());
    de_sq_ces.SetVR(gdcm::VR::SQ);
    de_sq_ces.SetValue(*sq_ces);
    de_sq_ces.SetVLToUndefined();

    // Replace sequence in data set (in case there was an existing entry).
    dest_ds.Replace(de_sq_ces);

}

int
main(int argc, char* argv[])
{
    /*
    We expect the input arguments to be:
    1) File path to a reference DICOM SPECT projection data set.
    2) File path to an interfile SPECT image reconstructed from the projection data pointed to by (1).
    3) File path to a parameter file used to reconstruct the image at (2).
    4) Output file path.
    */

    if (argc != 5)
      {
        std::cerr << "Usage: " << argv[0] << " <SPECT_sinogram(DICOM)> <SPECT_image(interfile)> <Recon_Par_File> <output_file_path)>\n";
        exit(EXIT_FAILURE);
    }

    const std::string projdata_DICOM_filename(argv[1]);
    const std::string imagedata_interfile_filename(argv[2]);
    const std::string recon_par_filename(argv[3]);
    const std::string output_DICOM_filename(argv[4]);
    
    // Instantiate a GDCM DataSet object to which we will assign all the necessary DICOM information.
    gdcm::DataSet data_set;
    
    // Read the DICOM projection data header and copy it to the destination DataSet.
    if (insert_projection_attributes(data_set, projdata_DICOM_filename) == stir::Succeeded::no)
    {
        exit(EXIT_FAILURE);
    }

    // Read the SPECT image (header and data)
    //  Extract the header data describing the image buffer and the spacing/size/positioning/orientation
    insert_image_attributes(data_set, imagedata_interfile_filename);

    // Insert information related to the reconstruction.
    // Some element will be accessible from the reconstruction parameter file,
    // others will be hard-coded.
    insert_reconstruction_attributes(data_set, recon_par_filename);
    
    // Create and open the output DICOM file
    gdcm::SmartPointer<gdcm::File> file = new gdcm::File; // gdcm::Writer requires a smart pointer to its File.
    // Pass the DICOM data that we've collected to the new file.
    gdcm::ImageReader reader;
    reader.SetFileName(projdata_DICOM_filename.c_str());
    if (reader.Read())
    {
        // Initialize the File object use the header (meta data) from the DICOM projection data
        gdcm::FileMetaInformation header = reader.GetFile().GetHeader();
        // Overwrite the Transfer Syntax UID to always be Implicit VR Little Endian.
        header.SetDataSetTransferSyntax(gdcm::TransferSyntax::ImplicitVRLittleEndian);
        file->SetHeader(header);
        // Insert the DataSet that we've constructed
        file->SetDataSet(data_set);
    }
    else
    {
        std::cerr << "Failed to open file: " << projdata_DICOM_filename << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Create a writer, pass the file info to it, then write.
    gdcm::Writer writer;
    writer.CheckFileMetaInformationOn();
    writer.SetFileName(output_DICOM_filename.c_str());
    writer.SetFile(*file);

    if (!writer.Write())
    {
        std::cerr << "Could not write: " << output_DICOM_filename.c_str() << std::endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
    
}
