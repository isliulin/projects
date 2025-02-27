// ==========================================================================
// Program DENSITY_L1 reads in a set of "squished" TDP and frusta
// metadata files generated by program CLEAN_L1.  It calculates an
// integrated illumination pattern for ALIRT within a constant
// Z-plane.  Assuming the illumination is exponentially
// attenuated through dense foliage, DENSITY_L1 renormalizes
// reflectivity probabilities so that they are approximately uniform
// in height.  It deletes any points with p-values less than some
// user-specified (or automatically calculated) reflectivity
// threshold.  DENSITY_L1 also performs a few iterations of simple
// density filtering to eliminate "lonely" voxel counts.  

// DENSITY_L1 outputs TDP and OSGA files for the aggregated and cleaned 
// point cloud.  It also generates rotated versions of these files
// which are aligned with the XY axes.  Finally, a rotated and z-flipped
// version of the data are written out to become input to program
// FISHING.

// cd to subdirectory containing squished.tdp files.  Then chant

//				density_L1

// ==========================================================================
// Last updated on 12/7/11; 12/8/11; 12/10/11
// ==========================================================================

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "image/drawfuncs.h"
#include "general/filefuncs.h"
#include "math/fourvector.h"
#include "math/prob_distribution.h"
#include "general/sysfuncs.h"
#include "osg/osg3D/tdpfuncs.h"
#include "video/texture_rectangle.h"
#include "image/TwoDarray.h"
#include "coincidence_processing/VolumetricCoincidenceProcessor.h"

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

   string bpfs_subdir=filefunc::get_pwd();
   cout << "bpfs_subdir = " << bpfs_subdir << endl;

// Constants:

   const double voxel_binsize=0.25;	// meter

// Repeated variable declarations:

   string banner,unix_cmd="";
   bool perturb_voxels_flag;

/*
// Import pure noise density-rate information:

   vector<double> noise_density_rate_mu,noise_density_rate_sigma;

   string noise_density_rate_text_filename=bpfs_subdir+
      "noise_density_rate.txt";
   filefunc::ReadInfile(noise_density_rate_text_filename);
   for (int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> column_strings=
         stringfunc::decompose_string_into_substrings(filefunc::text_line[i]);
      noise_density_rate_mu.push_back(stringfunc::string_to_number(
         column_strings[1]));
      noise_density_rate_sigma.push_back(stringfunc::string_to_number(
         column_strings[2]));
   }

   double weighted_noise_density_rate_mu=mathfunc::weighted_mean(
      noise_density_rate_mu,noise_density_rate_sigma);
   double weighted_noise_density_rate_sigma=mathfunc::weighted_std_dev(
      noise_density_rate_mu,noise_density_rate_sigma);

   cout << "noise density-rate = "
        << weighted_noise_density_rate_mu << " +/- "
        << weighted_noise_density_rate_sigma << endl;

   cout << "Expected noise counts per voxel = " 
        << weighted_noise_density_rate_mu*voxel_volume*3000*10
        << endl;
*/

// Parse input TDP files containing ALIRT images squished by program
// CLEAN_L1:

   vector<string> squished_tdp_filenames=
      filefunc::files_in_subdir_matching_substring(
         bpfs_subdir,"squished.tdp");

// First compute extremal XYZ values over all squished tdp files:

   banner="Computing extremal XYZ bounds";
   outputfunc::write_big_banner(banner);

   threevector XYZ_min(1.0E15,1.0E15,1.0E15);
   threevector XYZ_max(-1.0E15,-1.0E15,-1.0E15);
   for (unsigned int f=0; f<squished_tdp_filenames.size(); f++)
   {
      cout << "f = " << f
           << " squished filename = " << squished_tdp_filenames[f] << endl;
      tdpfunc::compute_extremal_XYZ_points_in_tdpfile(
         squished_tdp_filenames[f],XYZ_min,XYZ_max);
   }
   cout << "XYZ_min = " << XYZ_min << " XYZ_max = " << XYZ_max << endl;

// Import frusta metadata from text files exported by program CLEAN_L1:

   vector<double> frustum_weight;
   vector<threevector> sensor_posns;
   vector<threevector> frustum_ray[4];

   vector<string> frusta_filenames=
      filefunc::files_in_subdir_matching_substring(bpfs_subdir,"xform.frusta");

   for (unsigned int f=0; f<frusta_filenames.size(); f++)
   {
      cout << "f = " << f
           << " frustum filename = " << frusta_filenames[f] << endl;
      filefunc::ReadInfile(frusta_filenames[f]);

      int linenumber=0;
      int n_sensor_posns=filefunc::text_line.size()/5;
      for (int n=0; n<n_sensor_posns; n++)
      {
         vector<double> column_values=stringfunc::string_to_numbers(
            filefunc::text_line[linenumber++]);
         frustum_weight.push_back(column_values[1]);
         sensor_posns.push_back(threevector(
            column_values[2],column_values[3],column_values[4]));

         column_values.clear();
         column_values=stringfunc::string_to_numbers(
            filefunc::text_line[linenumber++]);
         frustum_ray[0].push_back(threevector(
            column_values[1],column_values[2],column_values[3]));

         column_values.clear();
         column_values=stringfunc::string_to_numbers(
            filefunc::text_line[linenumber++]);
         frustum_ray[1].push_back(threevector(
            column_values[1],column_values[2],column_values[3]));

         column_values.clear();
         column_values=stringfunc::string_to_numbers(
            filefunc::text_line[linenumber++]);
         frustum_ray[2].push_back(threevector(
            column_values[1],column_values[2],column_values[3]));

         column_values.clear();
         column_values=stringfunc::string_to_numbers(
            filefunc::text_line[linenumber++]);
         frustum_ray[3].push_back(threevector(
            column_values[1],column_values[2],column_values[3]));

/*
         cout << "n = " << n 
              << " sensor_posn = " 
              << sensor_posns.back().get(0) << " "
              << sensor_posns.back().get(1) << " "
              << sensor_posns.back().get(2) << endl;
         cout << "c = 0 " 
              << frustum_ray[0].back().get(0) << " "
              << frustum_ray[0].back().get(1) << " "
              << frustum_ray[0].back().get(2) << " "
              << endl;
         cout << "c = 1 " 
              << frustum_ray[1].back().get(0) << " "
              << frustum_ray[1].back().get(1) << " "
              << frustum_ray[1].back().get(2) << " "
              << endl;
         cout << "c = 2 " 
              << frustum_ray[2].back().get(0) << " "
              << frustum_ray[2].back().get(1) << " "
              << frustum_ray[2].back().get(2) << " "
              << endl;
         cout << "c = 3 " 
              << frustum_ray[3].back().get(0) << " "
              << frustum_ray[3].back().get(1) << " "
              << frustum_ray[3].back().get(2) << " "
              << endl;
         outputfunc::enter_continue_char();
*/
         
      } // loop over index n labeling frustum info for particular sensor posn
   } // loop over index f labeling frusta filenames

// As of 11/19/2011, we have empirically found that the frustum
// corners encoded via negative pixel numbers in raw BFP3 files
// are neither clockwise nor counter-clockwise ordered:

   vector<int> corner_index;
   for (int k=0; k<4; k++)
   {
      if (k==0) 
      {
         corner_index.push_back(0);
      }
      else if (k==1)
      {
         corner_index.push_back(2);
      }
      else if (k==2)
      {
         corner_index.push_back(3);
      }
      else if (k==3)
      {
         corner_index.push_back(1);
      }
   }

// Calculate illumination pattern within a constant Z plane first
// assuming no light attenuation through canopy:

   banner="Calculating illumination pattern in constant Z plane";
   outputfunc::write_big_banner(banner);

   int mdim=(XYZ_max.get(0)-XYZ_min.get(0))/voxel_binsize;
   int ndim=(XYZ_max.get(1)-XYZ_min.get(1))/voxel_binsize;
   twoDarray* illumpattern_twoDarray_ptr=new twoDarray(mdim,ndim);
   illumpattern_twoDarray_ptr->init_coord_system(
      XYZ_min.get(0),XYZ_max.get(0),XYZ_min.get(1),XYZ_max.get(1));
   illumpattern_twoDarray_ptr->clear_values();

//   double Z_min=XYZ_min.get(2);
   double Z_max=XYZ_max.get(2);

// Various empirical tests on 12/4/11 suggest setting Z_illumplane to
// Z_max rather than Z_min:

   double Z_illumplane=Z_max;

   vector<threevector> vertices,midcycle_vertices;
   int n_total_patterns=sensor_posns.size();
   cout << "n_total_patterns = " << n_total_patterns << endl;
   for (int j=0; j<n_total_patterns; j++)
   {
      outputfunc::update_progress_fraction(j,10000,n_total_patterns);
      threevector curr_sensor_posn = sensor_posns[j];
      double Z_sensor=curr_sensor_posn.get(2);

      vertices.clear();
      for (int k=0; k<4; k++)
      {
         double lambda=(Z_illumplane-Z_sensor)/
            frustum_ray[corner_index[k]].at(j).get(2);
         vertices.push_back(
            curr_sensor_posn+lambda*frustum_ray[corner_index[k]].at(j));
//         cout << "corner_index = " << corner_index[k] 
//              << " vertex.x = " << vertices.back().get(0)
//              << " vertex.y = " << vertices.back().get(1)
//              << " vertex.z = " << vertices.back().get(2) << endl;
      } // loop over index c labeling frustum corner

      polygon footprint(vertices);

// Amplify weights for footprints corresponding to oscillation extrema
// frusta locations:

      if (frustum_weight[j] < 0)	// oscillation mid-cycle
      {
         frustum_weight[j]=1;

         threevector vertex_avg(Zero_vector);
         for (unsigned int k=0; k<vertices.size(); k++)
         {
            vertex_avg += vertices[k];
         }
         vertex_avg /= vertices.size();
         midcycle_vertices.push_back(vertex_avg);
      }

      if (frustum_weight[j] > 2)	// oscillation extrema
      {
         frustum_weight[j]=10000;
//         frustum_weight[j]=POSITIVEINFINITY;
      }

// FAKE FAKE:  Thurs Dec 8, 2011
// We reset all frusta weight to uniform unity value:

      frustum_weight[j]=1;

      bool accumulate_flag=true;
      drawfunc::color_convex_quadrilateral_interior(
         footprint,frustum_weight[j],
         illumpattern_twoDarray_ptr,accumulate_flag);
   } // loop over index j labeling sensor position

// Fit line to midcycle vertices Zplane projections:

   threevector midcycle_vertices_COM(Zero_vector);
   for (unsigned int j=0; j<midcycle_vertices.size(); j++)
   {
      midcycle_vertices_COM += midcycle_vertices[j];
   } // loop over index j labeling midcycle vertices
   midcycle_vertices_COM /= midcycle_vertices.size();

   threevector avg_r_hat(Zero_vector);
   for (unsigned int j=0; j<midcycle_vertices.size(); j++)
   {
      threevector r_hat(midcycle_vertices[j]-midcycle_vertices_COM);
      r_hat=r_hat.unitvector();
      if (r_hat.get(0) < 0) r_hat *= -1;
      avg_r_hat += r_hat;
   } // loop over index j labeling midcycle vertices
   avg_r_hat /= midcycle_vertices.size();

   avg_r_hat.put(2,0);
   avg_r_hat=avg_r_hat.unitvector();
   cout << "avg r_hat = " << avg_r_hat << endl;

// Write out total illumination pattern to JPG file:

   int max_counts=0;
   vector<double> voxel_counts;
   for (int px=0; px<mdim; px++)
   {
      for (int py=0; py<ndim; py++)
      {
         int curr_counts=illumpattern_twoDarray_ptr->get(px,py);
         if (curr_counts==0) continue;
         max_counts=basic_math::max(max_counts,curr_counts);
         voxel_counts.push_back(curr_counts);
      }
   }
   cout << " max_counts = " << max_counts << endl;

   prob_distribution prob_counts(voxel_counts,1000);
   prob_counts.writeprobdists(false);

   twoDarray* tmp_pattern_twoDarray_ptr=new twoDarray(
      illumpattern_twoDarray_ptr);
   for (int px=0; px<mdim; px++)
   {
      for (int py=0; py<ndim; py++)
      {
         double curr_counts=illumpattern_twoDarray_ptr->get(px,py);
         tmp_pattern_twoDarray_ptr->put(px,py,curr_counts/max_counts);
      }
   }

   int n_channels=3;
   texture_rectangle* texture_rectangle_ptr=new texture_rectangle(
      mdim,ndim,1,n_channels,NULL);
   texture_rectangle_ptr->initialize_twoDarray_image(
      tmp_pattern_twoDarray_ptr);
   texture_rectangle_ptr->fill_twoDarray_image(
      tmp_pattern_twoDarray_ptr,n_channels);
   delete tmp_pattern_twoDarray_ptr;

   string output_filename="total_illumination_pattern.jpg";
   texture_rectangle_ptr->write_curr_frame(output_filename);
   delete texture_rectangle_ptr;

   banner="Wrote total illumination pattern to "+output_filename;
   outputfunc::write_big_banner(banner);

// Instantiate Volumetric Coincidence Processor:

   VolumetricCoincidenceProcessor* vcp_ptr=new VolumetricCoincidenceProcessor;
   vcp_ptr->initialize_coord_system(XYZ_min,XYZ_max,voxel_binsize);
   
   banner="Accumulating points within VCP";
   outputfunc::write_big_banner(banner);

   for (unsigned int f=0; f<squished_tdp_filenames.size(); f++)
   {
      cout << "f = " << f
           << " squished filename = " << squished_tdp_filenames[f] << endl;

      vector<double>* X_ptr=new vector<double>;
      vector<double>* Y_ptr=new vector<double>;
      vector<double>* Z_ptr=new vector<double>;
      vector<double>* P_ptr=new vector<double>;

      int npoints=tdpfunc::npoints_in_tdpfile(squished_tdp_filenames[f]);
      X_ptr->reserve(npoints);
      Y_ptr->reserve(npoints);
      Z_ptr->reserve(npoints);
      P_ptr->reserve(npoints);
      tdpfunc::read_XYZP_points_from_tdpfile(
         squished_tdp_filenames[f],*X_ptr,*Y_ptr,*Z_ptr,*P_ptr);

      for (unsigned int i=0; i<X_ptr->size(); i++)
      {
         double curr_x=X_ptr->at(i);
         double curr_y=Y_ptr->at(i);
         double curr_z=Z_ptr->at(i);
         int counts=P_ptr->at(i);
//         cout << "i = " << i << " input P = " << P_ptr->at(i) 
//              << " counts = " << counts << endl;
         
         vcp_ptr->accumulate_points_and_counts(curr_x,curr_y,curr_z,counts);
      }
      cout << "Aggregated number of points = " << vcp_ptr->size() << endl;

      delete X_ptr;
      delete Y_ptr;
      delete Z_ptr;
      delete P_ptr;
   } // loop over index f labeling squished filenames

   int n_aggregated_points=vcp_ptr->size();
   banner="Total number of aggregated points = "+stringfunc::number_to_string(
      n_aggregated_points);
   outputfunc::write_big_banner(banner);

// After looking at several scatter plots of P vs Z on 11/21/11, we estimated
// P obeys an exponential attenuation function of Z with the following
// decay constant:

//   const double e_folding_distance=0;	// meters
   const double e_folding_distance=25;	// meters

   int min_illum_counts=20;
   double p_threshold=vcp_ptr->find_signal_reflectivity_threshold(
      illumpattern_twoDarray_ptr,min_illum_counts,e_folding_distance);

   vector<double>* X_ptr=new vector<double>;
   vector<double>* Y_ptr=new vector<double>;
   vector<double>* Z_ptr=new vector<double>;
   vector<double>* P_ptr=new vector<double>;

   X_ptr->reserve(n_aggregated_points);
   Y_ptr->reserve(n_aggregated_points);
   Z_ptr->reserve(n_aggregated_points);
   P_ptr->reserve(n_aggregated_points);

   double min_prob_threshold=0.0;
   cout << "Enter minimum probability threshold:" << endl;
   cin >> min_prob_threshold;
//   double min_prob_threshold=p_threshold;
   perturb_voxels_flag=true;
//   perturb_voxels_flag=false;

/*
   vcp_ptr->retrieve_XYZP_points(
      X_ptr,Y_ptr,Z_ptr,P_ptr,min_prob_threshold,perturb_voxels_flag);
   cout << "aggregated_points.size() = " << X_ptr->size() << endl;

   string aggregated_tdp_filename="aggregated_points.tdp";
   tdpfunc::write_xyzp_data(
      aggregated_tdp_filename,X_ptr,Y_ptr,Z_ptr,P_ptr);
   unix_cmd="lodtree "+aggregated_tdp_filename;
   sysfunc::unix_command(unix_cmd);
*/

   banner="Density filtering aggregated points:";
   outputfunc::write_big_banner(banner);

   int delta_vox=1;
//   int min_neighbors=1;
   int min_neighbors=3;
//   int min_neighbors=4;
   cout << "Enter min_neighbors:" << endl;
   cin >> min_neighbors;
   
   int min_avg_counts=3;
   const int n_iters=3;
   for (int iter=0; iter<n_iters; iter++)
   {
      vcp_ptr->delete_isolated_voxels(delta_vox,min_neighbors,min_avg_counts);
   }

//   vcp_ptr->generate_p_vs_z_profiles();

   X_ptr->clear();
   Y_ptr->clear();
   Z_ptr->clear();
   P_ptr->clear();

   vcp_ptr->retrieve_XYZP_points(
      X_ptr,Y_ptr,Z_ptr,P_ptr,min_prob_threshold,perturb_voxels_flag);
   cout << "filtered_points.size() = " << X_ptr->size() << endl;

   string filtered_tdp_filename="filtered_points.tdp";
   tdpfunc::write_xyzp_data(
      filtered_tdp_filename,X_ptr,Y_ptr,Z_ptr,P_ptr);
   unix_cmd="lodtree "+filtered_tdp_filename;
   sysfunc::unix_command(unix_cmd);

// Export "flipped-Z" version of filtered point cloud for purposes of
// ground finding via fishnet:

// Invert all Z's so that Zmin <--> Zmax:

   double Zmin=mathfunc::minimal_value(*Z_ptr);
   double Zmax=mathfunc::maximal_value(*Z_ptr);
   cout << "Zmin = " << Zmin << " Zmax = " << Zmax << endl;

   vector<double>* Zflipped_ptr=new vector<double>;
   for (unsigned int i=0; i<Z_ptr->size(); i++)
   {
      double curr_Z=Z_ptr->at(i);
      Zflipped_ptr->push_back(Zmin+Zmax-curr_Z);
   }
   
   string flipped_filtered_tdp_filename="flipped_filtered_points.tdp";
   tdpfunc::write_xyzp_data(
      flipped_filtered_tdp_filename,X_ptr,Y_ptr,Zflipped_ptr,P_ptr);
   unix_cmd="lodtree "+flipped_filtered_tdp_filename;
   sysfunc::unix_command(unix_cmd);

/*   
// Calculate and export angle theta needed to align filtered point
// cloud with XY axes:

   double theta=atan2(avg_r_hat.get(1),avg_r_hat.get(0));
   cout << "theta = " << theta*180/PI << endl;
   double cos_theta=cos(theta);
   double sin_theta=sin(theta);
   
   double X_mean=mathfunc::mean(*X_ptr);
   double Y_mean=mathfunc::mean(*Y_ptr);

   string theta_filename="rotation_params.dat";
   ofstream theta_stream;
   theta_stream.precision(10);
   filefunc::openfile(theta_filename,theta_stream);
   theta_stream 
      << "# Angle theta (in radians) needed to align filtered point cloud with XY axes" 
      << endl;
   theta_stream << theta << endl;
   theta_stream << "# theta = " << theta*180/PI << " degs" << endl;
   theta_stream << "# 2D rotation center" << endl;
   theta_stream << X_mean << "  " << Y_mean << endl;
   filefunc::closefile(theta_filename,theta_stream);


// Generate rotated version of filtered points which align with XY axes:   

   for (int i=0; i<Z_ptr->size(); i++)
   {
      double x=X_ptr->at(i)-X_mean;
      double y=Y_ptr->at(i)-Y_mean;
      double x_rot=cos_theta*x+sin_theta*y;
      double y_rot=-sin_theta*x+cos_theta*y;
      X_ptr->at(i)=x_rot+X_mean;
      Y_ptr->at(i)=y_rot+Y_mean;
   }
   
   string rotated_filtered_tdp_filename="rotated_filtered_points.tdp";
   tdpfunc::write_xyzp_data(
      rotated_filtered_tdp_filename,X_ptr,Y_ptr,Z_ptr,P_ptr);
   unix_cmd="lodtree "+rotated_filtered_tdp_filename;
   sysfunc::unix_command(unix_cmd);
   
   string flipped_rotated_filtered_tdp_filename=
      "flipped_rotated_filtered_points.tdp";
   tdpfunc::write_xyzp_data(
      flipped_rotated_filtered_tdp_filename,X_ptr,Y_ptr,Zflipped_ptr,P_ptr);
   unix_cmd="lodtree "+flipped_rotated_filtered_tdp_filename;
   sysfunc::unix_command(unix_cmd);
*/
   delete X_ptr;
   delete Y_ptr;
   delete Z_ptr;
   delete Zflipped_ptr;
   delete P_ptr;
}
