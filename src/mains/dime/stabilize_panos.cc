// =======================================================================
// Program STABILIZE_PANOS first imports a set of subsampled, 
// "UV-corrected" WISP panoramas separated in time generated by program
// UVCORRECT_PANOS.  It extracts SIFT and ASIFT features via calls to
// Lowe's SIFT binary and the affine SIFT library.  Consolidated SIFT
// & ASIFT interest points and descriptors are exported to key files
// following Lowe's conventions. STABILIZE_PANOS performs tiepoint
// matching via homography estimation and RANSAC.

// Each tiepoint pair between a 0th and later WISP frame establishes a
// correspondence between some "X,Y" and counterpart "U,V"
// coordinates.  STABILIZE_PANOS simply computes the average of
// Delta=UV-XY over all input panoramas. The Delta twovectors are
// exported to an output text file as functions of panorama frame
// numbers.

// STABILIZE_PANOS exports a set of full-resolution and subsampled
// WISP panoramas which should be stabilized relative to the 0th
// frame's UV-corrected panorama.  It then chains the subsampled panos
// together into an AVI movie at 20 fps.

// =======================================================================
// Last updated on 7/8/13; 7/15/13; 7/17/13; 9/3/13
// =======================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "video/camerafuncs.h"
#include "general/filefuncs.h"
#include "video/image_matcher.h"
#include "math/mathfuncs.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::pair;
using std::string;

int main( int argc, char** argv ) 
{
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   PassesGroup passes_group(&arguments);
//   int videopass_ID=passes_group.get_videopass_ID();
//   cout << "videopass_ID = " << videopass_ID << endl;
//   string image_list_filename=passes_group.get_image_list_filename();
//   cout << "image_list_filename = " << image_list_filename << endl;
//   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
//   string bundler_IO_subdir="./bundler/DIME/";
//   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;

   string date_string="05202013";
   cout << "Enter date string (e.g. 05202013 or 05222013):" << endl;
   cin >> date_string;
   filefunc::add_trailing_dir_slash(date_string);

   string bundler_subdir="./bundler/DIME/";
   string MayFieldtest_subdir=bundler_subdir+"May2013_Fieldtest/";
//   string FSFdate_subdir=MayFieldtest_subdir+"05202013/";
   string FSFdate_subdir=MayFieldtest_subdir+date_string;
   cout << "FSFdate_subdir = " << FSFdate_subdir << endl;

//   int scene_ID=18;
   int scene_ID=19;
   cout << "Enter scene ID:" << endl;
   cin >> scene_ID;
   string scene_ID_str=stringfunc::integer_to_string(scene_ID,2);
   string bundler_IO_subdir=FSFdate_subdir+"Scene"+scene_ID_str+"/";
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;


   string stable_frames_subdir=bundler_IO_subdir+"stable_frames/";
   filefunc::dircreate(stable_frames_subdir);
   string subsampled_stable_frames_subdir=
      stable_frames_subdir+"subsampled/";
   filefunc::dircreate(subsampled_stable_frames_subdir);

   string delta_UV_filename=stable_frames_subdir+"delta_UV.dat";
   ofstream delta_stream;
   
   filefunc::openfile(delta_UV_filename,delta_stream);
   delta_stream << "Frame	delta_U		delta_V" << endl << endl;

// Import sub-sampled WISP panos:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group);
   int n_images=photogroup_ptr->get_n_photos();
   cout << "n_images = " << n_images << endl;

// We explicitly confirmed on 1/30/13 that the FLANN library yields
// noticeably better feature matching results than the older ANN
// library:

   bool FLANN_flag=true;
   image_matcher SIFT(photogroup_ptr,FLANN_flag);
   SIFT.set_sampson_error_flag(true);

// Note added on 2/10/13: "root-SIFT" matching appears to yield
// inferior results for Affine-SIFT features than conventional "SIFT"
// matching !

   SIFT.set_root_sift_matching_flag(false);
//   SIFT.set_root_sift_matching_flag(true);

   string features_subdir=bundler_IO_subdir+"features/";
   filefunc::dircreate(features_subdir);

   timefunc::initialize_timeofday_clock();

// --------------------------------------------------------------------------
// Feature extraction starts here:

// Extract conventional SIFT features from each input image via
// Lowe's binary:

   string sift_keys_subdir=bundler_IO_subdir+"sift_keys/";
   bool delete_pgm_file_flag=true;
//   bool delete_pgm_file_flag=false;
   SIFT.extract_SIFT_features(sift_keys_subdir,delete_pgm_file_flag);
//   SIFT.set_num_threads(6);
//   SIFT.parallel_extract_SIFT_features(sift_keys_subdir,delete_pgm_file_flag);
   outputfunc::print_elapsed_time();

   string asift_keys_subdir=bundler_IO_subdir+"asift_keys/";
   SIFT.extract_ASIFT_features(asift_keys_subdir);
   outputfunc::print_elapsed_time();

// Export consolidated sets of SIFT & ASIFT features to output keyfiles:

   string all_keys_subdir=bundler_IO_subdir+"all_keys/";
   filefunc::dircreate(all_keys_subdir);

   int istart=0;
   int istop=n_images-1;
   for (int i=istart; i<=istop; i++)
   {
      double progress_frac=double(i-istart)/double(istop-istart);
      outputfunc::print_elapsed_and_remaining_time(progress_frac);

      photograph* photo_ptr=photogroup_ptr->get_photograph_ptr(i);

      string basename=filefunc::getbasename(photo_ptr->get_filename());
      string prefix=stringfunc::prefix(basename);
      string all_keys_filename=all_keys_subdir+prefix+".key";

      cout << "all_keys_filename = " << all_keys_filename << endl;
      if (filefunc::fileexist(all_keys_filename))
      {
         cout << "all_keys_filename = " << all_keys_filename
              << " already exists in " << all_keys_subdir << endl;
      }
      else
      {
         SIFT.export_features_to_Lowe_keyfile(
            photo_ptr->get_ydim(),all_keys_filename,
            SIFT.get_image_feature_info(i));
         string banner="Exported "+all_keys_filename;
         outputfunc::write_banner(banner);
      }
   } // loop over index i labeling images

// --------------------------------------------------------------------------
// Feature matching between subsampled WISP panos starts here:

//   double max_Lowe_ratio=0.5;    	// DIME
   double max_Lowe_ratio=0.6;    	// DIME
//   double max_Lowe_ratio=0.7;    	// OK for GEO
//   cout << "Enter max Lowe ratio:" << endl;
//   cin >> max_Lowe_ratio;

   double sqrd_max_Lowe_ratio=sqr(max_Lowe_ratio);
   double worst_frac_to_reject=0;
//         double max_scalar_product=0.001;
//         double max_scalar_product=0.000333;
//   double max_scalar_product=1E-4;

   double sqrt_max_sqrd_delta=0.015;
//   cout << "Enter sqrt(max_sqrd_delta):" << endl;
//   cin >> sqrt_max_sqrd_delta;
   double max_sqrd_delta=sqr(sqrt_max_sqrd_delta);

//   int max_n_good_RANSAC_iters=250;
   int max_n_good_RANSAC_iters=500;

   photograph* zeroth_photo_ptr=photogroup_ptr->get_photograph_ptr(0);      
//   double Umax=zeroth_photo_ptr->get_maxU();

// Instantiate *input_texture_rectangle_ptr and
// *output_texture_rectangle_ptr to hold full-resolution (40Kx2.2K)
// input panoramas to be stabilized and output stabilized panoramas:

   string blank_image_filename="./blank_40Kx2.2K.jpg";
   texture_rectangle* input_texture_rectangle_ptr=
      new texture_rectangle(blank_image_filename,NULL);
   texture_rectangle* output_texture_rectangle_ptr=
      new texture_rectangle(blank_image_filename,NULL);

   int i=0;      
   for (int j=1; j<n_images; j++)
   {
      outputfunc::print_elapsed_time();
      cout << " j = " << j << endl;

// Match SIFT & ASIFT features across subsampled image pairs:

      SIFT.homography_match_image_pair_features(
         i,j,max_n_good_RANSAC_iters,sqrd_max_Lowe_ratio,
         worst_frac_to_reject,max_sqrd_delta,features_subdir);

      int n_inliers=SIFT.get_inlier_XY().size();
      vector<double> Delta_U,Delta_V;
      for (int r=0; r<n_inliers; r++)
      {
         twovector curr_XY=SIFT.get_inlier_XY().at(r); // 0th subsampled pano
         twovector curr_UV=SIFT.get_inlier_UV().at(r); // later subsampled pano
         Delta_U.push_back(curr_UV.get(0)-curr_XY.get(0));
         Delta_V.push_back(curr_UV.get(1)-curr_XY.get(1));
      }
      double mu_Delta_U=mathfunc::mean(Delta_U);
      double sigma_Delta_U=mathfunc::std_dev(Delta_U);
      cout << "Delta_U = " << mu_Delta_U << " +/- " << sigma_Delta_U << endl;
      double mu_Delta_V=mathfunc::mean(Delta_V);
      double sigma_Delta_V=mathfunc::std_dev(Delta_V);
      cout << "Delta_V = " << mu_Delta_V << " +/- " << sigma_Delta_V << endl;

      delta_stream << j 
                   << "        " << mu_Delta_U 
                   << "        " << mu_Delta_V << endl;

// Import full-resolution 40Kx2.2K WISP panorama into
// *input_texture_rectangle:

      photograph* photo_ptr=photogroup_ptr->get_photograph_ptr(j);      
      string input_image_filename=photo_ptr->get_filename();
//      cout << "input_image_filename = " << input_image_filename << endl;

      string horizons_subdir=filefunc::getdirname(input_image_filename);
//      cout << "horizons_subdir = " << horizons_subdir << endl;
      string image_basename=filefunc::getbasename(input_image_filename);
      string uvcorrected_image_basename=
         image_basename.substr(11,image_basename.size()-11);

      string uvcorrected_image_filename=horizons_subdir+
         uvcorrected_image_basename;
      cout << endl;
      cout << "Input uvcorrected_image_filename = "
           << uvcorrected_image_filename << endl << endl;
      input_texture_rectangle_ptr->import_photo_from_file(
         uvcorrected_image_filename);

      string stable_image_basename="stable_"+uvcorrected_image_basename;
      string stable_image_filename=stable_frames_subdir+stable_image_basename;
      cout << "Output stable_image_filename = "
           << stable_image_filename << endl << endl;
//      camerafunc::ucorrect_WISP_image(
//         input_texture_rectangle_ptr,output_texture_rectangle_ptr,
//         mu_Delta_U,0,0,0);
      camerafunc::uv_translate_WISP_image(
         input_texture_rectangle_ptr,output_texture_rectangle_ptr,
         mu_Delta_U,mu_Delta_V);
      output_texture_rectangle_ptr->write_curr_frame(stable_image_filename);

// Export subsampled version of corrected WISP image to subsampled
// subdirectory:

      string subsampled_filename=subsampled_stable_frames_subdir
         +"subsampled_"+stable_image_basename;
      string unix_cmd="convert -resize 9.9% "+stable_image_filename+" "+
         subsampled_filename;
      sysfunc::unix_command(unix_cmd);
   } // loop over index j labeling input images

   filefunc::closefile(delta_UV_filename,delta_stream);

// Generate AVI movie from subsampled stabilized panoramas:

   string AVI_filename=subsampled_stable_frames_subdir+
      "subsampled_stable_uv_20fps.avi";
   string unix_cmd="cd "+subsampled_stable_frames_subdir+";";
   unix_cmd += "mkmpeg4 -v -f 20 -b 24000000 -o "+
      AVI_filename+" *.jpg";
   sysfunc::unix_command(unix_cmd);
   string banner="Exported subsampled AVI movie to "+AVI_filename;
   outputfunc::write_big_banner(banner);

   delete input_texture_rectangle_ptr;
   delete output_texture_rectangle_ptr;

   banner="Finished running program STABILIZE_PANOS";
   outputfunc::write_banner(banner);
   outputfunc::print_elapsed_time();
}
