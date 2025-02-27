// ==========================================================================
// Program FLATTEN reads in a cleaned TDP file generated
// by program DENSITY_L1.  It also reads in ground surface data
// generated by program FISHING.  FLATTEN subtracts the latter from
// the former and exports a new point cloud where the ground surface
// is (hopefully) reduced to a simple Z-plane.  FLATTEN also crops the
// ground surface so that it only contains Z points which lie over
// (rotated) ALIRT data.  Rotated and unrotated versions of the
// cropped ground surface are exported to TDP files.

//				flatten

// ==========================================================================
// Last updated on 12/10/11; 12/11/11; 12/12/11; 4/6/14
// ==========================================================================

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "image/compositefuncs.h"
#include "general/filefuncs.h"
#include "image/imagefuncs.h"
#include "math/prob_distribution.h"
#include "general/sysfuncs.h"
#include "osg/osg3D/tdpfuncs.h"
#include "image/TwoDarray.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

// Constant declarations:

   const double null_value=-1;

// Repeated variable declarations:

   string banner,unix_cmd="";

   string filtered_file_tdpname="filtered_points.tdp";
   cout << "filtered tdp filename = " << filtered_file_tdpname << endl;
   
   vector<double>* X_ptr=new vector<double>;
   vector<double>* Y_ptr=new vector<double>;
   vector<double>* Z_ptr=new vector<double>;
   vector<double>* P_ptr=new vector<double>;

   int npoints=tdpfunc::npoints_in_tdpfile(filtered_file_tdpname);
   X_ptr->reserve(npoints);
   Y_ptr->reserve(npoints);
   Z_ptr->reserve(npoints);
   P_ptr->reserve(npoints);
   tdpfunc::read_XYZP_points_from_tdpfile(
      filtered_file_tdpname,*X_ptr,*Y_ptr,*Z_ptr,*P_ptr);

// Read in ground surface information generated by program FISHING and
// store within twoDarray *ground_ztwoDarray_ptr:

   double delta_x=0.3; // meter
   double delta_y=0.3; // meter

   string ground_surface_tdp_filename="ground_surface.tdp";
   twoDarray* ground_ztwoDarray_ptr=tdpfunc::generate_ztwoDarray_from_tdpfile(
      ground_surface_tdp_filename,delta_x,delta_y);
   unsigned int mdim=ground_ztwoDarray_ptr->get_mdim();
   unsigned int ndim=ground_ztwoDarray_ptr->get_ndim();
   double xlo=ground_ztwoDarray_ptr->get_xlo();
   double xhi=ground_ztwoDarray_ptr->get_xhi();
   double ylo=ground_ztwoDarray_ptr->get_ylo();
   double yhi=ground_ztwoDarray_ptr->get_yhi();

// Instantiate cropped ground mask twoDarray

   twoDarray* cropped_ground_twoDarray_ptr=new twoDarray(mdim,ndim);
   cropped_ground_twoDarray_ptr->init_coord_system(xlo,xhi,ylo,yhi);
   cropped_ground_twoDarray_ptr->initialize_values(null_value);

   double zground_interp;
   for (unsigned int i=0; i<Z_ptr->size(); i++)
   {
      double x=X_ptr->at(i);
      double y=Y_ptr->at(i);
      double inside_flag=ground_ztwoDarray_ptr->point_to_interpolated_value(
         x,y,zground_interp);
      if (!inside_flag) continue;
      
      unsigned int m,n;
      if (cropped_ground_twoDarray_ptr->point_to_pixel(x,y,m,n))
      {
         cropped_ground_twoDarray_ptr->put(m,n,zground_interp);
      }
   } // loop over index i 

   unsigned int median_nsize=5;
   imagefunc::median_fill(
      median_nsize,cropped_ground_twoDarray_ptr,null_value);

// As of 12/10/11, we believe that aggressively low-pass filtering
// ground surface generated by program FISHING yields better bare
// earth results:
   
   twoDarray* lopass_cropped_ground_twoDarray_ptr=new twoDarray(
      cropped_ground_twoDarray_ptr);
   lopass_cropped_ground_twoDarray_ptr->initialize_values(null_value);
   double ground_correlation_distance=2;	// meters 
//   double ground_correlation_distance=10;	// meters  (Peter's CP)
   int avg_nsize=ground_correlation_distance/delta_x;
   if (is_even(avg_nsize)) avg_nsize++;
   cout << "avg_nsize = " << avg_nsize << endl;

   imagefunc::median_filter(
      avg_nsize,cropped_ground_twoDarray_ptr,
      lopass_cropped_ground_twoDarray_ptr);

//   imagefunc::average_nonnull_values(
//      avg_nsize,avg_nsize,cropped_ground_twoDarray_ptr,
//      lopass_cropped_ground_twoDarray_ptr,null_value);

   delete cropped_ground_twoDarray_ptr;
   cropped_ground_twoDarray_ptr=lopass_cropped_ground_twoDarray_ptr;

   vector<double>* Xground_ptr=new vector<double>;
   vector<double>* Yground_ptr=new vector<double>;
   vector<double>* Zground_ptr=new vector<double>;

   Xground_ptr->reserve(npoints);
   Yground_ptr->reserve(npoints);
   Zground_ptr->reserve(npoints);
   
   for (unsigned int px=0; px<mdim; px++)
   {
      for (unsigned int py=0; py<ndim; py++)
      {
         double x,y;
         cropped_ground_twoDarray_ptr->pixel_to_point(px,py,x,y);
         double z=cropped_ground_twoDarray_ptr->get(px,py);

         if (z < 0) continue;

         Xground_ptr->push_back(x);
         Yground_ptr->push_back(y);
         Zground_ptr->push_back(z);
      } // loop over py
   } // loop over px

// Write out probability distribution for low-pass filtered and
// cropped Zground values:

   prob_distribution prob_Zground(*Zground_ptr,1000);
   cout << "Median Z ground = " << prob_Zground.median() << endl;
   cout << "Quartile width = " << prob_Zground.quartile_width() << endl;
   string prob_jpg_filename="prob_density.jpg";
   string prob_Zground_jpg_filename="prob_Zground_density.jpg";
   prob_Zground.writeprobdists(false);
    unix_cmd="mv "+prob_jpg_filename+" "+prob_Zground_jpg_filename;
   sysfunc::unix_command(unix_cmd);

// Write out new ground surface TDP file whose XY values are cropped
// to match filtered point cloud:

   string ground_cropped_tdp_filename="ground_cropped.tdp";
   tdpfunc::write_xyz_data(
      ground_cropped_tdp_filename,Xground_ptr,Yground_ptr,Zground_ptr);
   unix_cmd="lodtree "+ground_cropped_tdp_filename;
   sysfunc::unix_command(unix_cmd);

// Subtract ground from filtered ALIRT data to generate "flattened"
// version of point cloud:
   
   vector<double>* Xflattened_ptr=new vector<double>;
   vector<double>* Yflattened_ptr=new vector<double>;
   vector<double>* Zflattened_ptr=new vector<double>;
   vector<double>* Pflattened_ptr=new vector<double>;

   Xflattened_ptr->reserve(npoints);
   Yflattened_ptr->reserve(npoints);
   Zflattened_ptr->reserve(npoints);
   Pflattened_ptr->reserve(npoints);

   for (unsigned int i=0; i<Z_ptr->size(); i++)
   {
      double x=X_ptr->at(i);
      double y=Y_ptr->at(i);
      double p=P_ptr->at(i);
      double inside_flag=cropped_ground_twoDarray_ptr->
         point_to_interpolated_value(x,y,zground_interp);
      if (!inside_flag) continue;
      if (zground_interp < 0) continue;
      
      double z_flattened=Z_ptr->at(i)-zground_interp;
      Xflattened_ptr->push_back(x);
      Yflattened_ptr->push_back(y);
      Zflattened_ptr->push_back(z_flattened);
      Pflattened_ptr->push_back(p);
   } // loop over index i 

   delete X_ptr;
   delete Y_ptr;
   delete Z_ptr;
   delete P_ptr;

// Calculate probability distribution for flattened Zground
// values. Then threshold any very small or very large Z value
// outliers:

   prob_distribution prob_Zflattened(*Zflattened_ptr,1000);
   double z_0001=prob_Zflattened.find_x_corresponding_to_pcum(0.0001);
   double z_999=prob_Zflattened.find_x_corresponding_to_pcum(0.999);
   
   vector<double>* Xflattened_thresholded_ptr=new vector<double>;
   vector<double>* Yflattened_thresholded_ptr=new vector<double>;
   vector<double>* Zflattened_thresholded_ptr=new vector<double>;
   vector<double>* Pflattened_thresholded_ptr=new vector<double>;

   Xflattened_thresholded_ptr->reserve(npoints);
   Yflattened_thresholded_ptr->reserve(npoints);
   Zflattened_thresholded_ptr->reserve(npoints);
   Pflattened_thresholded_ptr->reserve(npoints);

   for (unsigned int i=0; i<Zflattened_ptr->size(); i++)
   {
      double curr_z=Zflattened_ptr->at(i);
      if (curr_z < z_0001 || curr_z > z_999) continue;
      Xflattened_thresholded_ptr->push_back(Xflattened_ptr->at(i));
      Yflattened_thresholded_ptr->push_back(Yflattened_ptr->at(i));
      Zflattened_thresholded_ptr->push_back(Zflattened_ptr->at(i));
      Pflattened_thresholded_ptr->push_back(Pflattened_ptr->at(i));
   }

   delete Xflattened_ptr;
   delete Yflattened_ptr;
   delete Zflattened_ptr;
   delete Pflattened_ptr;

   string flattened_tdp_filename="flattened_points.tdp";
   tdpfunc::write_xyzp_data(
      flattened_tdp_filename,Xflattened_thresholded_ptr,
      Yflattened_thresholded_ptr,Zflattened_thresholded_ptr,
      Pflattened_thresholded_ptr);
   unix_cmd="lodtree "+flattened_tdp_filename;
   sysfunc::unix_command(unix_cmd);
}
