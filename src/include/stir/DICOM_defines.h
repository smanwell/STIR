#pragma once
#include <gdcmAttribute.h>
#include <gdcmVR.h>
#include <vector>

struct DCM_Tag
{
  gdcm::Tag tag;
  gdcm::VR vr;
  uint16_t group;
  uint16_t element;
  DCM_Tag(const gdcm::VR _vr, const uint16_t _group, const uint16_t _element) { 
	vr = _vr;
    group = _group;
    element = _element;
    tag = gdcm::Tag(group, element);
  }

  bool operator==(const DCM_Tag& _val) const { return tag == _val.tag; }
  bool operator!=(const DCM_Tag& _val) const { return tag != _val.tag; }
};

#define DCM_Specific_Character_Set gdcm::Attribute<0x0008, 0x0005, gdcm::VR::CS>
#define DCM_Image_Type gdcm::Attribute<0x0008, 0x0008, gdcm::VR::CS>
#define DCM_SOP_Class_UID gdcm::Attribute<0x0008, 0x0016, gdcm::VR::UI>
#define DCM_SOP_Instance_UID gdcm::Attribute<0x0008, 0x0018, gdcm::VR::UI>
#define DCM_Study_Date gdcm::Attribute<0x0008, 0x0020, gdcm::VR::DA>
#define DCM_Series_Date gdcm::Attribute<0x0008, 0x0021, gdcm::VR::DA>
#define DCM_Acquisition_Date gdcm::Attribute<0x0008, 0x0022, gdcm::VR::DA>
#define DCM_Content_Date gdcm::Attribute<0x0008, 0x0023, gdcm::VR::DA>
#define DCM_Study_Time gdcm::Attribute<0x0008, 0x0030, gdcm::VR::TM>
#define DCM_Series_Time gdcm::Attribute<0x0008, 0x0031, gdcm::VR::TM>
#define DCM_Acquisition_Time gdcm::Attribute<0x0008, 0x0032, gdcm::VR::TM>
#define DCM_Content_Time gdcm::Attribute<0x0008, 0x0033, gdcm::VR::TM>
#define DCM_Modality gdcm::Attribute<0x0008, 0x0060, gdcm::VR::CS>
#define DCM_Manufacturer gdcm::Attribute<0x0008, 0x0070, gdcm::VR::LO>
#define DCM_Manufacturer gdcm::Attribute<0x0008, 0x0070, gdcm::VR::LO>
#define DCM_Institution_Name gdcm::Attribute<0x0008, 0x0080, gdcm::VR::LO>
#define DCM_Code_Value gdcm::Attribute<0x0008, 0x0100, gdcm::VR::SH>
#define DCM_Coding_Scheme_Designator gdcm::Attribute<0x0008, 0x0102, gdcm::VR::SH>
#define DCM_Coding_Scheme_Version gdcm::Attribute<0x0008, 0x0103, gdcm::VR::SH>
#define DCM_Code_Meaning gdcm::Attribute<0x0008, 0x0104, gdcm::VR::LO>
#define DCM_Timezone_Offset_From_UTC gdcm::Attribute<0x0008, 0x0201, gdcm::VR::SH>
#define DCM_Station_Name gdcm::Attribute<0x0008, 0x1010, gdcm::VR::SH>
#define DCM_Study_Description gdcm::Attribute<0x0008, 0x1030, gdcm::VR::LO>
#define DCM_Procedure_Code_Sequence gdcm::Attribute<0x0008, 0x1032, gdcm::VR::SQ>
#define DCM_Series_Description gdcm::Attribute<0x0008, 0x103e, gdcm::VR::LO>
#define DCM_Manufacturers_Model_Name gdcm::Attribute<0x0008, 0x1090, gdcm::VR::LO>
#define DCM_Anatomic_Region_Sequence gdcm::Attribute<0x0008, 0x2218, gdcm::VR::SQ>
#define DCM_Patients_Name gdcm::Attribute<0x0010, 0x0010, gdcm::VR::PN>
#define DCM_Patient_ID gdcm::Attribute<0x0010, 0x0020, gdcm::VR::LO>
#define DCM_Issuer_of_Patient_ID gdcm::Attribute<0x0010, 0x0021, gdcm::VR::LO>
#define DCM_Issuer_of_Patient_ID_Qualifiers_Sequence gdcm::Attribute<0x0010, 0x0024, gdcm::VR::SQ>
#define DCM_Patients_Sex gdcm::Attribute<0x0010, 0x0040, gdcm::VR::CS>
#define DCM_Other_Patient_IDs gdcm::Attribute<0x0010, 0x1000, gdcm::VR::LO>
#define DCM_Patients_Age gdcm::Attribute<0x0010, 0x1010, gdcm::VR::AS>
#define DCM_Intervention_Drug_Information_Sequence gdcm::Attribute<0x0018, 0x0026, gdcm::VR::SQ>
#define DCM_Intervention_Drug_Dose gdcm::Attribute<0x0018, 0x0028, gdcm::VR::DS>
#define DCM_Radiopharmaceutical gdcm::Attribute<0x0018, 0x0031, gdcm::VR::LO, >
#define DCM_Intervention_Drug_Name gdcm::Attribute<0x0018, 0x0034, gdcm::VR::LO>
#define DCM_Intervention_Drug_Start_Time gdcm::Attribute<0x0018, 0x0035, gdcm::VR::TM>
#define DCM_Slice_Thickness gdcm::Attribute<0x0018, 0x0050, gdcm::VR::DS>
#define DCM_Counts_Accumulated gdcm::Attribute<0x0018, 0x0070, gdcm::VR::IS>
#define DCM_Acquisition_Termination_Condition gdcm::Attribute<0x0018, 0x0071, gdcm::VR::CS>
#define DCM_Spacing_Between_Slices gdcm::Attribute<0x0018, 0x0088, gdcm::VR::DS>
#define DCM_Software_Versions gdcm::Attribute<0x0018, 0x1020, gdcm::VR::LO>
#define DCM_Protocol_Name gdcm::Attribute<0x0018, 0x1030, gdcm::VR::LO>
#define DCM_Radionuclide_Total_Dose gdcm::Attribute<0x0018, 0x1074, gdcm::VR::DS, >
#define DCM_Table_Height gdcm::Attribute<0x0018, 0x1130, gdcm::VR::DS>
#define DCM_Table_Traverse gdcm::Attribute<0x0018, 0x1131, gdcm::VR::DS>
#define DCM_Rotation_Direction gdcm::Attribute<0x0018, 0x1140, gdcm::VR::CS>
#define DCM_Radial_Position gdcm::Attribute<0x0018, 0x1142, gdcm::VR::DS>
#define DCM_Scan_Arc gdcm::Attribute<0x0018, 0x1143, gdcm::VR::DS>
#define DCM_Angular_Step gdcm::Attribute<0x0018, 0x1144, gdcm::VR::DS>
#define DCM_Center_of_Rotation_Offset gdcm::Attribute<0x0018, 0x1145, gdcm::VR::DS>
#define DCM_Field_of_View_Shape gdcm::Attribute<0x0018, 0x1147, gdcm::VR::CS>
#define DCM_Field_of_View_Dimensions gdcm::Attribute<0x0018, 0x1149, gdcm::VR::IS>
#define DCM_Collimator_grid_Name gdcm::Attribute<0x0018, 0x1180, gdcm::VR::SH>
#define DCM_Collimator_Type gdcm::Attribute<0x0018, 0x1181, gdcm::VR::CS>
#define DCM_Focal_Distance gdcm::Attribute<0x0018, 0x1182, gdcm::VR::IS>
#define DCM_X_Focus_Center gdcm::Attribute<0x0018, 0x1183, gdcm::VR::DS>
#define DCM_Y_Focus_Center gdcm::Attribute<0x0018, 0x1184, gdcm::VR::DS>
#define DCM_Date_of_Last_Calibration gdcm::Attribute<0x0018, 0x1200, gdcm::VR::DA>
#define DCM_Time_of_Last_Calibration gdcm::Attribute<0x0018, 0x1201, gdcm::VR::TM>
#define DCM_Convolution_Kernel gdcm::Attribute<0x0018, 0x1210, gdcm::VR::SH>
#define DCM_Actual_Frame_Duration gdcm::Attribute<0x0018, 0x1242, gdcm::VR::IS>
#define DCM_Contributing_Equipment_Sequence gdcm::Attribute<0x0018, 0xa001, gdcm::VR::SQ>
#define DCM_Contribution_Date_Time gdcm::Attribute<0x0018, 0xa002, gdcm::VR::DT>
#define DCM_Contribution_Description gdcm::Attribute<0x0018, 0xa003, gdcm::VR::ST>
#define DCM_Study_Instance_UID gdcm::Attribute<0x0020, 0x000d, gdcm::VR::UI>
#define DCM_Series_Instance_UID gdcm::Attribute<0x0020, 0x000e, gdcm::VR::UI>
#define DCM_Study_ID gdcm::Attribute<0x0020, 0x0010, gdcm::VR::SH>
#define DCM_Series_Number gdcm::Attribute<0x0020, 0x0011, gdcm::VR::IS>
#define DCM_Instance_Number gdcm::Attribute<0x0020, 0x0013, gdcm::VR::IS>
#define DCM_Image_Position_Patient gdcm::Attribute<0x0020, 0x0032, gdcm::VR::DS>
#define DCM_Image_Orientation_Patient gdcm::Attribute<0x0020, 0x0037, gdcm::VR::DS>
#define DCM_Frame_of_Reference_UID gdcm::Attribute<0x0020, 0x0052, gdcm::VR::UI>
#define DCM_Position_Reference_Indicator gdcm::Attribute<0x0020, 0x1040, gdcm::VR::LO>
#define DCM_Samples_per_Pixel gdcm::Attribute<0x0028, 0x0002, gdcm::VR::US>
#define DCM_Photometric_Interpretation gdcm::Attribute<0x0028, 0x0004, gdcm::VR::CS>
#define DCM_Number_of_Frames gdcm::Attribute<0x0028, 0x0008, gdcm::VR::IS>
#define DCM_Frame_Increment_Pointer gdcm::Attribute<0x0028, 0x0009, gdcm::VR::AT>
#define DCM_Rows gdcm::Attribute<0x0028, 0x0010, gdcm::VR::US>
#define DCM_Columns gdcm::Attribute<0x0028, 0x0011, gdcm::VR::US>
#define DCM_Pixel_Spacing gdcm::Attribute<0x0028, 0x0030, gdcm::VR::DS>
#define DCM_Zoom_Factor gdcm::Attribute<0x0028, 0x0031, gdcm::VR::DS>
#define DCM_Corrected_Image gdcm::Attribute<0x0028, 0x0051, gdcm::VR::CS>
#define DCM_Bits_Allocated gdcm::Attribute<0x0028, 0x0100, gdcm::VR::US>
#define DCM_Bits_Stored gdcm::Attribute<0x0028, 0x0101, gdcm::VR::US>
#define DCM_High_Bit gdcm::Attribute<0x0028, 0x0102, gdcm::VR::US>
#define DCM_Pixel_Representation gdcm::Attribute<0x0028, 0x0103, gdcm::VR::US>
#define DCM_Smallest_Image_Pixel_Value gdcm::Attribute<0x0028, 0x0106, gdcm::VR::US>
#define DCM_Largest_Image_Pixel_Value gdcm::Attribute<0x0028, 0x0107, gdcm::VR::US>
#define DCM_Window_Center gdcm::Attribute<0x0028, 0x1050, gdcm::VR::DS>
#define DCM_Window_Width gdcm::Attribute<0x0028, 0x1051, gdcm::VR::DS>
#define DCM_Requesting_Physician gdcm::Attribute<0x0032, 0x1032, gdcm::VR::PN>
#define DCM_Requesting_Service gdcm::Attribute<0x0032, 0x1033, gdcm::VR::LO>
#define DCM_Requesting_Service_Code_Sequence gdcm::Attribute<0x0032, 0x1034, gdcm::VR::SQ>
#define DCM_Requested_Procedure_Description gdcm::Attribute<0x0032, 0x1060, gdcm::VR::LO>
#define DCM_Requested_Procedure_Code_Sequence gdcm::Attribute<0x0032, 0x1064, gdcm::VR::SQ>
#define DCM_Scheduled_Procedure_Step_Description gdcm::Attribute<0x0040, 0x0007, gdcm::VR::LO>
#define DCM_Scheduled_Protocol_Code_Sequence gdcm::Attribute<0x0040, 0x0008, gdcm::VR::SQ>
#define DCM_Scheduled_Procedure_Step_ID gdcm::Attribute<0x0040, 0x0009, gdcm::VR::SH>
#define DCM_Identifier_Type_Code gdcm::Attribute<0x0040, 0x0035, gdcm::VR::CS>
#define DCM_Performed_Procedure_Step_Start_Date gdcm::Attribute<0x0040, 0x0244, gdcm::VR::DA>
#define DCM_Performed_Procedure_Step_Start_Time gdcm::Attribute<0x0040, 0x0245, gdcm::VR::TM>
#define DCM_Performed_Procedure_Step_ID gdcm::Attribute<0x0040, 0x0253, gdcm::VR::SH>
#define DCM_Performed_Procedure_Step_Description gdcm::Attribute<0x0040, 0x0254, gdcm::VR::LO>
#define DCM_Request_Attributes_Sequence gdcm::Attribute<0x0040, 0x0275, gdcm::VR::SQ>
#define DCM_Requested_Procedure_ID gdcm::Attribute<0x0040, 0x1001, gdcm::VR::SH>
#define DCM_Requested_Procedure_Priority gdcm::Attribute<0x0040, 0x1003, gdcm::VR::SH>
#define DCM_Filler_Order_Number_Imaging_Service_Request gdcm::Attribute<0x0040, 0x2017, gdcm::VR::LO>
#define DCM_Purpose_of_Reference_Code_Sequence gdcm::Attribute<0x0040, 0xa170, gdcm::VR::SQ>
#define DCM_Number_of_Energy_Windows gdcm::Attribute<0x0054, 0x0011, gdcm::VR::US>
#define DCM_Energy_Window_Information_Sequence gdcm::Attribute<0x0054, 0x0012, gdcm::VR::SQ>
#define DCM_Energy_Window_Range_Sequence gdcm::Attribute<0x0054, 0x0013, gdcm::VR::SQ>
#define DCM_Energy_Window_Lower_Limit gdcm::Attribute<0x0054, 0x0014, gdcm::VR::DS>
#define DCM_Energy_Window_Upper_Limit gdcm::Attribute<0x0054, 0x0015, gdcm::VR::DS>
#define DCM_Radiopharmaceutical_Information_Sequence gdcm::Attribute<0x0054, 0x0016, gdcm::VR::SQ>
#define DCM_Energy_Window_Name gdcm::Attribute<0x0054, 0x0018, gdcm::VR::SH>
#define DCM_Number_of_Detectors gdcm::Attribute<0x0054, 0x0021, gdcm::VR::US>
#define DCM_Detector_Information_Sequence gdcm::Attribute<0x0054, 0x0022, gdcm::VR::SQ>
#define DCM_Rotation_Vector gdcm::Attribute<0x0054, 0x0050, gdcm::VR::US>
#define DCM_Number_of_Rotations gdcm::Attribute<0x0054, 0x0051, gdcm::VR::US>
#define DCM_Rotation_Information_Sequence gdcm::Attribute<0x0054, 0x0052, gdcm::VR::SQ>
#define DCM_Number_of_Frames_in_Rotation gdcm::Attribute<0x0054, 0x0053, gdcm::VR::US>
#define DCM_Slice_Vector gdcm::Attribute<0x0054, 0x0080, gdcm::VR::US>
#define DCM_Number_of_Slices gdcm::Attribute<0x0054, 0x0081, gdcm::VR::US>
#define DCM_Angular_View_Vector gdcm::Attribute<0x0054, 0x0090, gdcm::VR::US>
#define DCM_Start_Angle gdcm::Attribute<0x0054, 0x0200, gdcm::VR::DS>
#define DCM_Type_of_Detector_Motion gdcm::Attribute<0x0054, 0x0202, gdcm::VR::CS>
#define DCM_Radionuclide_Code_Sequence gdcm::Attribute<0x0054, 0x0300, gdcm::VR::SQ, >
#define DCM_Image_ID gdcm::Attribute<0x0054, 0x0400, gdcm::VR::SH>
#define DCM_Patient_Orientation_Code_Sequence gdcm::Attribute<0x0054, 0x0410, gdcm::VR::SQ>
#define DCM_Patient_Orientation_Modifier_Code_Sequence gdcm::Attribute<0x0054, 0x0412, gdcm::VR::SQ>
#define DCM_Patient_Gantry_Relationship_Code_Sequence gdcm::Attribute<0x0054, 0x0414, gdcm::VR::SQ>
#define DCM_Pixel_Data gdcm::Attribute<0x7fe0, 0x0010, gdcm::VR::OW>

//namespace DCMDefTags
//{
//void get_proj_data_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags);
//void get_image_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags);
//void get_recon_par_DCM_tag_set(std::vector<DCM_Tag>& DCM_tags);
//}
