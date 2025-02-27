// ========================================================================
// Program LADARSYNTH performs a brute-force search over a
// 7-dimensional parameter space to find the scale factor, global
// translation and global rotation needed to georegister Noah
// Snavely's relatively registered photo set generated by his BUNDLER
// program.  We hardwire reasonable initial guesses for all these
// parameters within this program.  Looping over all the parameters,
// LADARSYNTH computes the squared projection error between 3D
// features manually selected in the NYC point cloud and 2D tiepoint
// counterparts also manually selected in some number of Noah's
// photos.  After multiple iterations of this looping are performed,
// LADARSYNTH returns a decent estimate for the 7 global parameter
// values.

// ladarsynth --region_filename ./bundler/HAFB/5-25-flight1/packages/peter_inputs.pkg

// ladarsynth --region_filename ./bundler/HAFB/5-25-flight1-CreditUnion/packages/peter_inputs.pkg

// Important note added in January 2010: Must respond "NO" when
// queried to convert input feature pass numbers

// ========================================================================
// Last updated on 6/20/11; 6/22/11; 6/23/11; 7/8/11
// ========================================================================

#include <iostream>
#include <string>
#include <vector>
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "video/camera.h"
#include "math/constant_vectors.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgFeatures/FeaturesGroup.h"
#include "general/filefuncs.h"
#include "osg/ModeController.h"
#include "templates/mytemplates.h"
#include "osg/osgOperations/Operations.h"
#include "optimum/optimizer.h"
#include "optimum/optimizer_funcs.h"
#include "numerical/param_range.h"
#include "passes/PassesGroup.h"
#include "video/photograph.h"
#include "video/photogroup.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "osg/osgWindow/ViewerManager.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ifstream;
using std::string;
using std::vector;

// ==========================================================================
int main( int argc, char** argv )
{

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   string cloud_filename="empty.xyzp";
   int cloudpass_ID=passes_group.generate_new_pass(cloud_filename);
   Pass* pass_ptr=passes_group.get_pass_ptr(cloudpass_ID);
   
   string image_list_filename=passes_group.get_image_list_filename();
   cout << " image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;

   string image_sizes_filename=passes_group.get_image_sizes_filename();
   cout << "image_sizes_filename = " << image_sizes_filename << endl;
   string bundle_filename=passes_group.get_bundle_filename();
   cout << "bundle_filename = " << bundle_filename << endl;
//   string covariances_filename=bundler_IO_subdir+"covariances.dat";
//   cout << "covariances_filename = " << covariances_filename << endl;

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("3D imagery");

// Create OSG root node:

   osg::Group* root = new osg::Group;

// Instantiate Operations object to handle mode, animation and image
// number control:

   Operations operations(ndims,window_mgr_ptr,passes_group);

   ModeController* ModeController_ptr=operations.get_ModeController_ptr();
   root->addChild(operations.get_OSGgroup_ptr());

// Add a custom manipulator to the event handler list:

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_ptr,window_mgr_ptr);
   window_mgr_ptr->set_CameraManipulator(CM_3D_ptr);

// Instantiate group to hold all decorations:
   
   Decorations decorations(window_mgr_ptr,ModeController_ptr,CM_3D_ptr);

// Read in Noah's original set of photos and reconstructed cameras:

   photogroup* bundler_photogroup_ptr=new photogroup();

   int n_photos_to_reconstruct=-1;
   bundler_photogroup_ptr->reconstruct_bundler_cameras(
      bundler_IO_subdir,image_list_filename,image_sizes_filename,
      bundle_filename,n_photos_to_reconstruct);

//   bundler_photogroup_ptr->read_photograph_covariance_traces(
//      covariances_filename);

// Copy original bundler photogroup onto working photogroup:

   photogroup* photogroup_ptr=new photogroup(*bundler_photogroup_ptr);
   int n_photos(photogroup_ptr->get_n_photos());
   cout << "n_photos = " << n_photos << endl;

// Initialize values for 7 global parameters needed to map bundler
// into world coordinates:

// HAFB 5-25-11 flight #1 around Credit Union parameters:

   double scale_0 = 530.658009474;
   double xtrans_0 = 313284.955587;
   double ytrans_0 = 4702895.89072;
   double ztrans_0 = 753.436368201;
   double az_0 = -102.049657005;
   double el_0 = -3.1104612636;
   double roll_0 = 11.0575532871;
// min_score = 0.368842967817


//   double scale_0 = 530.896912372;
//   double xtrans_0 = 313286.279887;
//   double ytrans_0 = 4702896.49331;
//   double ztrans_0 = 753.537362631;
//   double az_0 = -102.061447189;
//   double el_0 = -3.00984383159;
//   double roll_0 = 11.0070825303;
// min_score = 0.418530019853

//   double scale_0 = 530.74075111;
//   double xtrans_0 = 313283.511746;
//   double ytrans_0 = 4702894.03245;
//   double ztrans_0 = 754.333190531;
//   double az_0 = -102.330884521;
//   double el_0 = -2.78803068408;
//   double roll_0 = 10.9638755517;
// min_score = 0.608540340356


// HAFB 5-25-11 flight #1 around Credit Union parameters derived from
// automatically aligning GPS flight path via Horn's fit with Bundler's
// reconstructed camera path:

//   double scale_0 = 528.1880478;
//   double xtrans_0 = 313262.0577;
//   double ytrans_0 = 4702878.068;
//   double ztrans_0 = 778.2265064;
//   double az_0 = -102.2874481;
//   double el_0 = -3.774566935;
//   double roll_0 = 11.22987512;
   
   threevector bundler_rotation_origin( 
      0.8407217026, -4.402640027, 0.9562195599 ); 

/*
// HAFB 5-25-11 flight #1 parameters derived from automatically
// aligning GPS flight path via Horn's fit with Bundler's
// reconstructed camera path:

   double scale_0 = 607.6087759;
   double xtrans_0 = 312624.789;
   double ytrans_0 = 4702642.943;
   double ztrans_0 = 777.2634642;
   double az_0 = 78.22852189;
   double el_0 = -3.969422217;
   double roll_0 = 18.1096131;
   
   threevector bundler_rotation_origin( 
      -1.456946901, -1.981181625, 0.5975318973 ); 
*/


/*
   double scale_0=11.3511650263346;
   double xtrans_0=328140.559711755;
   double ytrans_0=4692066.68078271;
   double ztrans_0=18.2412664446772;
   double az_0=-159.533317681255;
   double el_0=0.637843771630157;
   double roll_0=-15.7861034431828;
   threevector bundler_rotation_origin(
      328212.210605263,
      4692025.66431579,
      36.1552629968421);
*/
   
   photogroup_ptr->set_bundler_to_world_scalefactor(scale_0);
   photogroup_ptr->set_bundler_to_world_az(az_0*PI/180);
   photogroup_ptr->set_bundler_to_world_el(el_0*PI/180);
   photogroup_ptr->set_bundler_to_world_roll(roll_0*PI/180);
   photogroup_ptr->set_bundler_to_world_translation(
      threevector(xtrans_0,ytrans_0,ztrans_0));
 
   optimizer* optimizer_ptr=new optimizer(photogroup_ptr);
   optimizer_ptr->set_bundler_photogroup_ptr(bundler_photogroup_ptr);

   for (int n=0; n<n_photos; n++)
   {
//      cout << "n = " << n << endl;
      photograph* photograph_ptr=photogroup_ptr->get_photograph_ptr(n);
      camera* camera_ptr=photograph_ptr->get_camera_ptr();
      camera_ptr->construct_internal_parameter_K_matrix();
   }

// As of 4/20/09, we hardwire the pass numbers for the photos for
// which we manually identified 2D tiepoint counterparts to 3D
// features within some ladar point cloud:

   vector<int> manually_selected_photo_numbers;

// HAFB 5-25-11 FLIR flight #1 over HAFB Credit Union with 4.7 deg FOV

   manually_selected_photo_numbers.push_back(125-125);
   manually_selected_photo_numbers.push_back(140-125);
   manually_selected_photo_numbers.push_back(180-125);
   manually_selected_photo_numbers.push_back(260-125);
   manually_selected_photo_numbers.push_back(295-125);
   manually_selected_photo_numbers.push_back(310-125);

// HAFB 5-25-11 FLIR flight #1 over HAFB with 28 deg FOV

//   manually_selected_photo_numbers.push_back(170-100);
//   manually_selected_photo_numbers.push_back(250-100);
//   manually_selected_photo_numbers.push_back(325-100);
//   manually_selected_photo_numbers.push_back(400-100);

/*
// MIT 2317

   manually_selected_photo_numbers.push_back(32);
   manually_selected_photo_numbers.push_back(1150);
   manually_selected_photo_numbers.push_back(1218);
   manually_selected_photo_numbers.push_back(1385);
   manually_selected_photo_numbers.push_back(1416);
   manually_selected_photo_numbers.push_back(1432);
   manually_selected_photo_numbers.push_back(1437);
   manually_selected_photo_numbers.push_back(1662);
   manually_selected_photo_numbers.push_back(1817);
   manually_selected_photo_numbers.push_back(2003);
*/

//   cout << "manually_selected_photo_numbers = " << endl;
//   templatefunc::printVector(manually_selected_photo_numbers);

// Instantiate FeaturesGroup to hold small number of manually
// extracted ladar and corresponding Bundler stills features:

   FeaturesGroup* manual_FeaturesGroup_ptr=new FeaturesGroup(
      ndims,pass_ptr,CM_3D_ptr);

   string manual_features_filename=
      bundler_IO_subdir+"features_CU_manual_combined.dat";
//      bundler_IO_subdir+"features_HAFB_manual_combined.dat";
//      bundler_IO_subdir+"features_manual_combined_10passes.dat";
   string banner="Do NOT convert manually extracted feature pass numbers!";
   outputfunc::write_big_banner(banner);

   manual_FeaturesGroup_ptr->read_feature_info_from_file(
      manual_features_filename);
//   manual_FeaturesGroup_ptr->write_feature_html_file(5+2);

   optimizer_ptr->extract_manual_feature_info(manual_FeaturesGroup_ptr);

   param_range scale(0.9*scale_0,1.1*scale_0, 5);
   param_range x_trans(xtrans_0 - 100, xtrans_0 + 100, 3);
   param_range y_trans(ytrans_0 - 100, ytrans_0 + 100, 3);
   param_range z_trans(ztrans_0 - 30, ztrans_0+30, 3);
   param_range az((az_0-5)*PI/180, (az_0+5)*PI/180,5);
   param_range el((el_0-3)*PI/180, (el_0+3)*PI/180,5);
   param_range roll((roll_0-2)*PI/180, (roll_0+2)*PI/180,5);

   double min_score=POSITIVEINFINITY;
  
//    int counter=0;
//   int n_iters=5;
//   int n_iters=10;
   int n_iters=15;
   for (int iter=0; iter<n_iters; iter++)
   {
      cout << "iter = " << iter << " of " << n_iters << endl;

// ========================================================================
// Begin while loops over camera parameters here
// ========================================================================

      while (scale.prepare_next_value())
      {
         cout << scale.get_counter() << " " << flush;

         while (x_trans.prepare_next_value())
         {
            while (y_trans.prepare_next_value())
            {
               while (z_trans.prepare_next_value())
               {
                  threevector curr_translation(
                     x_trans.get_value(),y_trans.get_value(),
                     z_trans.get_value());
                  
                  while (az.prepare_next_value())
                  {
                     while (el.prepare_next_value())
                     {
                        while (roll.prepare_next_value())
                        {

                           double score=0;
                           for (unsigned int k=0; 
                                k<manually_selected_photo_numbers.size(); k++)
                           {
                              int n=manually_selected_photo_numbers[k];
//                              cout << "n = " << n << endl;
                              photograph* photograph_ptr=
                                 photogroup_ptr->get_photograph_ptr(n);
                              camera* camera_ptr=photograph_ptr->
                                 get_camera_ptr();

                              photograph* bundler_photograph_ptr=
                                 bundler_photogroup_ptr->
                                 get_photograph_ptr(n);

                              *camera_ptr=*(bundler_photograph_ptr->
                                            get_camera_ptr());

// Post-sailplane reconstruction (Dec 2010) method for converting bundler
// to world coordinates:

                              camera_ptr->convert_bundler_to_world_coords(
                                 bundler_rotation_origin,
                                 az.get_value(),el.get_value(),
                                 roll.get_value(),
                                 scale.get_value(),curr_translation);

/*
// Deprecated pre-sailplane reconstruction (Dec 2010) method for
// converting bundler to world coordinates:

                              camera_ptr->convert_bundler_to_world_coords(
                                 x_trans.get_value(),y_trans.get_value(),
                                 z_trans.get_value(),
			         bundler_rotation_origin,
				 az.get_value(),
                                 el.get_value(),roll.get_value(),
                                 scale.get_value());
*/

                              camera_ptr->
                                 construct_projection_matrix_for_fixed_K();
//                              cout << " *P_ptr = " 
//                                   << *(camera_ptr->get_P_ptr()) << endl;
                           
                              double avg_residual=optimizer_ptr->
                                 projection_error(n);
//                              cout << "avg_residual = " << avg_residual 
//                                   << endl;
                              if (finite(avg_residual) != 0)
                              {
                                 score += 1000*avg_residual;
                              }
                              else
                              {
                                 score += POSITIVEINFINITY;
                                 break;
                              }

                           } // loop over index k labeling manually selected
			     //   photos

                           if (score < min_score)
                           {
                              min_score=score;
                              scale.set_best_value();
                              x_trans.set_best_value();
                              y_trans.set_best_value();
                              z_trans.set_best_value();
                              az.set_best_value();
                              el.set_best_value();
                              roll.set_best_value();
                           }

//                           cout << "score = " << score 
//                                << " min_score = " << min_score 
//                                << " s_count = " << scale.get_counter()
//                                << " counter = " << counter++ << endl;

                           
                        } // roll while loop
                     }  // el while loop
                  } // az while loop
               } // z_trans while loop
            } // y_trans while loop
         } // x_trans while loop
      } // scale while loop
      cout << endl;

// ========================================================================
// End while loop over camera parameters
// ========================================================================

      double frac=0.45;
      scale.shrink_search_interval(scale.get_best_value(),frac);
      x_trans.shrink_search_interval(x_trans.get_best_value(),frac);
      y_trans.shrink_search_interval(y_trans.get_best_value(),frac);
      z_trans.shrink_search_interval(z_trans.get_best_value(),frac);
      az.shrink_search_interval(az.get_best_value(),frac);
      el.shrink_search_interval(el.get_best_value(),frac);
      roll.shrink_search_interval(roll.get_best_value(),frac);

      cout.precision(12);
      cout << "Best scale value = " << scale.get_best_value() << endl;
      cout << "Best x_trans value = " << x_trans.get_best_value() << endl;
      cout << "Best y_trans value = " << y_trans.get_best_value() << endl;
      cout << "Best z_trans value = " << z_trans.get_best_value() << endl;
      cout << "Best az value = " << az.get_best_value()*180/PI << endl;
      cout << "Best el value = " << el.get_best_value()*180/PI << endl;
      cout << "Best roll value = " << roll.get_best_value()*180/PI << endl;
      cout << "min_score = " << min_score << endl << endl;

//      outputfunc::enter_continue_char();

   } // loop over iter index

   cout.precision(12);
   cout << "double scale_0 = " << scale.get_best_value() << ";" << endl;
   cout << "double xtrans_0 = " << x_trans.get_best_value() << ";" << endl;
   cout << "double ytrans_0 = " << y_trans.get_best_value() << ";" << endl;
   cout << "double ztrans_0 = " << z_trans.get_best_value() << ";" << endl;
   cout << "double az_0 = " << az.get_best_value()*180/PI << ";" << endl;
   cout << "double el_0 = " << el.get_best_value()*180/PI << ";" << endl;
   cout << "double roll_0 = " << roll.get_best_value()*180/PI << ";" << endl;
   cout << "// min_score = " << min_score << endl << endl;

   string peter_inputs_filename=bundler_IO_subdir+
      "packages/peter_inputs.pkg";

   cout << "======================================================" << endl;
   cout << "Replace scaling, translation and rotation parameter values" 
        << endl;
   cout << "within "+peter_inputs_filename << endl;
   cout << "with the following georegistered values:" << endl;
   cout << "======================================================" << endl
        << endl;

   cout << "--fitted_world_to_bundler_distance_ratio " 
        << scale.get_best_value() << endl;
   cout << "--bundler_translation_X " << x_trans.get_best_value() << endl;
   cout << "--bundler_translation_Y " << y_trans.get_best_value() << endl;
   cout << "--bundler_translation_Z " << z_trans.get_best_value() << endl;
   cout << "--global_az " << az.get_best_value()*180/PI << endl;
   cout << "--global_el " << el.get_best_value()*180/PI << endl;
   cout << "--global_roll " << roll.get_best_value()*180/PI << endl;
   cout << "--bundler_rotation_origin_X " 
        << bundler_rotation_origin.get(0) << endl;
   cout << "--bundler_rotation_origin_Y " 
        << bundler_rotation_origin.get(1) << endl;
   cout << "--bundler_rotation_origin_Z " 
        << bundler_rotation_origin.get(2) << endl;

   delete optimizer_ptr;
   delete window_mgr_ptr;
}
