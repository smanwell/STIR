#include "stir/DICOM_defines.h"

/*
	Populate the set of DICOM tags using the common attributes from the projection data.
*/
//void 
//DCMDefTags::get_proj_data_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags)
//{
  //DCM_tags.clear(); // Clear the set of any existing elements.

  //DCM_tags.push_back(DCM_Specific_Character_Set);
  //DCM_tags.push_back(DCM_Image_Type);
  //DCM_tags.push_back(DCM_SOP_Class_UID);
  //DCM_tags.push_back(DCM_Study_Date);
  //DCM_tags.push_back(DCM_Acquisition_Date);
  //DCM_tags.push_back(DCM_Study_Time);
  //DCM_tags.push_back(DCM_Acquisition_Time);
  //DCM_tags.push_back(DCM_Modality);
  //DCM_tags.push_back(DCM_Manufacturer);
  //DCM_tags.push_back(DCM_Manufacturers_Model_Name);
  //DCM_tags.push_back(DCM_Institution_Name);
  //DCM_tags.push_back(DCM_Timezone_Offset_From_UTC);
  //DCM_tags.push_back(DCM_Station_Name);
  //DCM_tags.push_back(DCM_Study_Description);
  //DCM_tags.push_back(DCM_Counts_Accumulated);
  //DCM_tags.push_back(DCM_Acquisition_Termination_Condition);
  //DCM_tags.push_back(DCM_Software_Versions);
  //DCM_tags.push_back(DCM_Protocol_Name);
  //DCM_tags.push_back(DCM_Date_of_Last_Calibration);
  //DCM_tags.push_back(DCM_Time_of_Last_Calibration);
  //DCM_tags.push_back(DCM_Procedure_Code_Sequence);
  //DCM_tags.push_back(DCM_Anatomic_Region_Sequence);
  //DCM_tags.push_back(DCM_Patients_Name);
  //DCM_tags.push_back(DCM_Patient_ID);
  //DCM_tags.push_back(DCM_Issuer_of_Patient_ID);
  //DCM_tags.push_back(DCM_Issuer_of_Patient_ID_Qualifiers_Sequence);
  //DCM_tags.push_back(DCM_Patients_Sex);
  //DCM_tags.push_back(DCM_Other_Patient_IDs);
  //DCM_tags.push_back(DCM_Patients_Age);
  //DCM_tags.push_back(DCM_Intervention_Drug_Information_Sequence);
  //DCM_tags.push_back(DCM_Study_Instance_UID);
  //DCM_tags.push_back(DCM_Study_ID);
  //DCM_tags.push_back(DCM_Series_Number);
  //DCM_tags.push_back(DCM_Instance_Number);
  //DCM_tags.push_back(DCM_Frame_of_Reference_UID);
  //DCM_tags.push_back(DCM_Position_Reference_Indicator);
  //DCM_tags.push_back(DCM_Requesting_Physician);
  //DCM_tags.push_back(DCM_Requested_Procedure_Description);
  //DCM_tags.push_back(DCM_Requested_Procedure_Code_Sequence);
  //DCM_tags.push_back(DCM_Performed_Procedure_Step_Start_Date);
  //DCM_tags.push_back(DCM_Performed_Procedure_Step_Start_Time);
  //DCM_tags.push_back(DCM_Performed_Procedure_Step_ID);
  //DCM_tags.push_back(DCM_Performed_Procedure_Step_Description);
  //DCM_tags.push_back(DCM_Request_Attributes_Sequence);
  //DCM_tags.push_back(DCM_Requested_Procedure_Priority);
  //DCM_tags.push_back(DCM_Filler_Order_Number_Imaging_Service_Request);
  //DCM_tags.push_back(DCM_Radiopharmaceutical_Information_Sequence);
  //DCM_tags.push_back(DCM_Detector_Information_Sequence); // Copy only the first item
  //DCM_tags.push_back(DCM_Rotation_Vector);
  //DCM_tags.push_back(DCM_Number_of_Rotations);
  //DCM_tags.push_back(DCM_Rotation_Information_Sequence);
  //DCM_tags.push_back(DCM_Angular_View_Vector);
  //DCM_tags.push_back(DCM_Type_of_Detector_Motion);
  //DCM_tags.push_back(DCM_Patient_Orientation_Code_Sequence);
  //DCM_tags.push_back(DCM_Patient_Gantry_Relationship_Code_Sequence);
//}

/*
    Populate the set of DICOM tags using the attributes specific to the SPECT image data.
    Note that some of these fields overlap with those from the projection data.
    In these cases the values from the projection data need to be overwritten.
*/
//void 
//DCMDefTags::get_image_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags)
//{
  //DCM_tags.clear(); // Clear the set of any existing elements.

  //DCM_tags.push_back(DCM_SOP_Instance_UID);
  //DCM_tags.push_back(DCM_Series_Date);
  //DCM_tags.push_back(DCM_Content_Date);
  //DCM_tags.push_back(DCM_Series_Time);
  //DCM_tags.push_back(DCM_Content_Time);
  //DCM_tags.push_back(DCM_Series_Description);
  //DCM_tags.push_back(DCM_Series_Instance_UID);

  //DCM_tags.push_back(DCM_Slice_Thickness);
  //DCM_tags.push_back(DCM_Samples_per_Pixel);
  //DCM_tags.push_back(DCM_Photometric_Interpretation);
  //DCM_tags.push_back(DCM_Number_of_Frames);
  //DCM_tags.push_back(DCM_Number_of_Slices);
  //DCM_tags.push_back(DCM_Frame_Increment_Pointer);
  //DCM_tags.push_back(DCM_Slice_Vector);
  //DCM_tags.push_back(DCM_Rows);
  //DCM_tags.push_back(DCM_Columns);
  //DCM_tags.push_back(DCM_Pixel_Spacing);
  //DCM_tags.push_back(DCM_Bits_Allocated);
  //DCM_tags.push_back(DCM_Bits_Stored);
  //DCM_tags.push_back(DCM_High_Bit);
  //DCM_tags.push_back(DCM_Pixel_Representation);
  //// Order here is important, we must read the pixel data prior to
  //// handling the <>_Image_Pixel_Value and Window_<> fields.
  //DCM_tags.push_back(DCM_Pixel_Data);
  //DCM_tags.push_back(DCM_Smallest_Image_Pixel_Value);
  //DCM_tags.push_back(DCM_Largest_Image_Pixel_Value);
  //DCM_tags.push_back(DCM_Window_Center);
  //DCM_tags.push_back(DCM_Window_Width);
  //DCM_tags.push_back(DCM_Number_of_Detectors);       // Just 1, even for multi-head SPECT cameras
  //DCM_tags.push_back(DCM_Image_Position_Patient);    // Belongs to the Detector Information Sequence
  //DCM_tags.push_back(DCM_Image_Orientation_Patient); // Belongs to the Detector Information Sequence

  //DCM_tags.push_back(DCM_Number_of_Energy_Windows);           // Should only have 1 item corresponding to the photopeak
  //DCM_tags.push_back(DCM_Energy_Window_Information_Sequence); // Which contains the following 4 tags per item
  //DCM_tags.push_back(DCM_Energy_Window_Range_Sequence);       // Contains the following 2 tags per item.
  //DCM_tags.push_back(DCM_Energy_Window_Lower_Limit);
  //DCM_tags.push_back(DCM_Energy_Window_Upper_Limit);
  //DCM_tags.push_back(DCM_Energy_Window_Name);
//}

/*
    Populate the set of DICOM tags using the attributes specific to the reconstruction parameters.
*/
//void 
//DCMDefTags::get_recon_par_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags)
//{
  //DCM_tags.clear(); // Clear the set of any existing elements.

  //DCM_tags.push_back(DCM_Series_Description);
  
// DCM_tags.push_back(DCM_Corrected_Image);
// DCM_tags.push_back(DCM_Image_ID);

// Belongs to the Detector Information Sequence
// Optional only, made redundant by specifying the image spacing. 
// DCM_tags.push_back(DCM_Zoom_Factor); 

// DCM_tags.push_back(DCM_Convolution_Kernel); // Specifies the recon method and corrections it includes
  //// The following sequence will refer to the reconstruction software
  //// Its values will need to be set manually, i.e. we can't simply set the sequence directly.
  //// The values for the single item as the four that follow.
  //DCM_tags.push_back(DCM_Contributing_Equipment_Sequence); // NOTE that the item for the sequence will need to be created manually
  //DCM_tags.push_back(DCM_Manufacturer);
  //DCM_tags.push_back(DCM_Manufacturers_Model_Name);
  //DCM_tags.push_back(DCM_Contribution_Date_Time);
  //DCM_tags.push_back(DCM_Contribution_Description);

//}