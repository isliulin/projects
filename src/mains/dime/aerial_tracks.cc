// =======================================================================
// Program AERIAL_TRACKS imports aerial track information extracted
// by Mike Watson from various May 2013 Panama City fieldtest sources.
// It instantiates and fills track objects with aircraft timing and
// UTM position information.  AERIAL_TRACKS exports aerial tracks
// in 3D UTM geocoordinates to text files named like

//			tracks_18:34:04.10-18:55:02.58.txt

// and projected imageplane UV coordinate to text files name like

//		projected_tracks_18:34:04.10-18:55:02.58.txt


//				./aerial_tracks

// =======================================================================
// Last updated on 7/23/13; 8/8/13; 12/23/13
// =======================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "astro_geo/Clock.h"
#include "general/filefuncs.h"
#include "astro_geo/geofuncs.h"
#include "astro_geo/geopoint.h"
#include "osg/osgOperations/Operations.h"
#include "general/outputfuncs.h"
#include "passes/PassesGroup.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"
#include "track/tracks_group.h"
#include "osg/osgWindow/ViewerManager.h"

int main( int argc, char** argv ) 
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::map;
   using std::ofstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);

// Initialize constants and parameters read in from command line as
// well as ascii text file:

   const int ndims=2;
   PassesGroup passes_group(&arguments);

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("2D imagery");

// Instantiate Operations object to handle mode, animation and image
// number control:

   bool display_movie_state=true;
   bool display_movie_number=true;
   bool display_movie_world_time=true;
   Operations operations(
      ndims,window_mgr_ptr,passes_group,display_movie_state,
      display_movie_number,display_movie_world_time);

// Specify start, stop and step times for master game clock:

   operations.set_master_world_start_UTC(
      passes_group.get_world_start_UTC_string());
   operations.set_master_world_stop_UTC(
      passes_group.get_world_stop_UTC_string());
   operations.set_delta_master_world_time_step_per_master_frame(
      passes_group.get_world_time_step());

   string date_string="05202013";
   cout << "Enter date string (e.g. 05202013 or 05222013):" << endl;
   cin >> date_string;
   filefunc::add_trailing_dir_slash(date_string);

   string bundler_subdir="./bundler/DIME/";
   string MayFieldtest_subdir=bundler_subdir+"May2013_Fieldtest/";
   string FSFdate_subdir=MayFieldtest_subdir+date_string;
//   string FSFdate_subdir=MayFieldtest_subdir+"05202013/";
   cout << "FSFdate_subdir = " << FSFdate_subdir << endl;

   int scene_ID;
   cout << "Enter scene ID:" << endl;
   cin >> scene_ID;
   string scene_ID_str=stringfunc::integer_to_string(scene_ID,2);
   string bundler_IO_subdir=FSFdate_subdir+"Scene"+scene_ID_str+"/";
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string stable_frames_subdir=bundler_IO_subdir+"stable_frames/";
   string pano_times_filename=stable_frames_subdir+"pano_times.txt";
   string panos_az_el_filename=stable_frames_subdir+"Panos_Az_El.txt";
   string FSF_track_filename=stable_frames_subdir+"FSF_UTM_track.dat";

   string GPSINS_subdir="/data/DIME/GPSINS/May2013_Fieldtest/20130520/";
   string aerial_tracks_subdir=bundler_IO_subdir+"aerial_tracks/";
   filefunc::dircreate(aerial_tracks_subdir);

   timefunc::initialize_timeofday_clock();

//   const double min_range=0*1000;
//   const double max_range=200*1000;	// meters
   const double min_Z=10;		// meters

   bool northern_hemisphere_flag=true;
   int specified_UTM_zonenumber=16;	// Panama City, FL
   Clock clock;
   tracks_group ship_tracks_group,aerial_tracks_group;

// Import scene-specific track for WISP sensor generated by program
// GEOREG_FSF_PANOS:

   int FSF_track_ID=0;
   track* FSF_track_ptr=ship_tracks_group.generate_new_track(FSF_track_ID);

   filefunc::ReadInfile(FSF_track_filename);
   vector<threevector> wisp_XYZ;
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<double> column_values=stringfunc::string_to_numbers(
         filefunc::text_line[i]);
      double epoch_time=column_values[1];
      double easting=column_values[2];
      double northing=column_values[3];
      double altitude=column_values[4];
      threevector curr_WISP_posn(easting,northing,altitude);
      FSF_track_ptr->set_XYZ_coords(epoch_time,curr_WISP_posn);
   }

// Initialize WISP image plane parameters:

   int pano_width=40000;
   int pano_height=2200;
//   double Umin=0;
   double Umax=double(pano_width)/double(pano_height);
//   double Vmin=0;
   double Vmax=1;

// In April 2013, Gary Long discovered that WISP pixels are not square!

//   double IFOV_u=360*PI/180 / Umax;
   double IFOV_v=30*PI/180 / Vmax;
//   cout << "IFOV_u = " << IFOV_u << endl;
//   cout << "IFOV_v = " << IFOV_v << endl;
//   cout << "IFOV_v/IFOV_u = " << IFOV_v/IFOV_u << endl;

// Import WISP image times as functions of panorama IDs:

   typedef map<int,double> PANO_TIME_MAP;
   
// independent int var = pano_ID
// dependent double var = epoch time

   PANO_TIME_MAP pano_time_map;
   PANO_TIME_MAP::iterator pano_time_iter;

   filefunc::ReadInfile(pano_times_filename);
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i]);
      int pano_ID=stringfunc::string_to_number(substrings[0]);
      double epoch_time=stringfunc::string_to_number(substrings[1]);
      pano_time_map[pano_ID]=epoch_time;
   }

// Import azimuths corresponding to U=0 and elevations corresponding
// to V=0 within each stabilized WISP panorama.  Store results within
// an STL map:

   typedef map<int,twovector> U0_AZ_V0_EL_MAP;

// independent int var = pano_ID
// dependent twovector var = azimuth corresponding to U=0 (degs) and
// elevation corresponding to V=0 (degs)

   U0_AZ_V0_EL_MAP u0_az_v0_el_map;
   U0_AZ_V0_EL_MAP::iterator u0_az_v0_el_iter;
   
   filefunc::ReadInfile(panos_az_el_filename);
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<double> column_values=stringfunc::string_to_numbers(
         filefunc::text_line[i]);

      int pano_ID=column_values[0];
      double uzero_az=column_values[3];
      double vzero_el=column_values[4];
      u0_az_v0_el_map[pano_ID]=twovector(uzero_az,vzero_el);
   }

// Import aerial tracks from CSV files generated by Mike Watson:

   string aerial_tracks_filename=
      GPSINS_subdir+"Scene"+scene_ID_str+"_TargetData_processed.csv";
   bool file_read_flag=filefunc::ReadInfile(aerial_tracks_filename);

   string separator_chars=",";
   vector<int> track_labels;
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i],separator_chars);
      double elapsed_secs=stringfunc::string_to_number(substrings[0]);

      int track_label=-1;
      string platform_label=substrings[1].substr(0,3);
      if (platform_label=="qf4")
      {
         track_label=10;
      }
      else if (platform_label=="bqm")
      {
         track_label=11;
      }
      else if (substrings[1]=="F15")
      {
         track_label=12;
      }
      else if (substrings[1]=="F15_21")
      {
         track_label=13;
      }
      else if (substrings[1]=="HE")
      {
         track_label=14;
      }
      else if (substrings[1]=="F16_5")
      {
         track_label=20;
      }
      else if (substrings[1]=="F16_23")
      {
         track_label=21;
      }
      else
      {
         cout << "No platform label found in processed TargetData CSV file"
              << endl;
         cout << " at line number " << i << endl;
         continue;
      }
      
      if (i%100==0)
      {
         cout << "i = " << i 
              << " track_label = " << track_label
              << endl;
      }

      double latitude=stringfunc::string_to_number(substrings[2]);
      double longitude=stringfunc::string_to_number(substrings[3]);
      double altitude=stringfunc::string_to_number(substrings[4]);
      geopoint curr_geopoint(
         longitude,latitude,altitude,specified_UTM_zonenumber);

      track* curr_track_ptr=
         aerial_tracks_group.get_track_given_label(track_label);
      
      if (curr_track_ptr==NULL)
      {
         curr_track_ptr=aerial_tracks_group.generate_new_track();
         curr_track_ptr->set_label_ID(track_label);
         aerial_tracks_group.associate_track_labelID_and_ID(
            curr_track_ptr);
         track_labels.push_back(track_label);
      }

      curr_track_ptr->set_XYZ_coords(
         elapsed_secs,curr_geopoint.get_UTM_posn());

//      cout << "track_label = " << track_label
//           << " track_ID = " << curr_track_ptr->get_ID()
//           << " curr_track_ptr = " << curr_track_ptr << endl;
//      outputfunc::enter_continue_char();

   } // loop over index i labeling lines in AERIAL_tracks_filename

   int n_tracks=aerial_tracks_group.get_n_tracks();
   cout << "n_tracks = " << n_tracks << endl;
   cout << "track_labels.size() = " << track_labels.size() << endl;

   double start_time=operations.get_master_world_start_time();
   double stop_time=operations.get_master_world_stop_time();
   double dt=operations.get_delta_master_world_time_step_per_master_frame();

   clock.convert_elapsed_secs_to_date(start_time);
   string start_stamp=clock.YYYY_MM_DD_H_M_S();
   vector<string> substrings=stringfunc::decompose_string_into_substrings(
      start_stamp);
   string start_HHMMSS=substrings[1];
   cout << "Starting track time = " << start_stamp << endl;
   cout << "Start HHMMSS = " << start_HHMMSS << endl;

   clock.convert_elapsed_secs_to_date(stop_time);
   string stop_stamp=clock.YYYY_MM_DD_H_M_S();
   substrings=stringfunc::decompose_string_into_substrings(stop_stamp);
   string stop_HHMMSS=substrings[1];
   cout << "Stopping track time = " << stop_stamp << endl;
   cout << "Stop HHMMSS = " << stop_HHMMSS << endl;

   threevector interp_aerial_posn,interp_FSF_posn;

// First export 3D AERIAL track positions in UTM geocoordinates 
// as function of track ID:

   string tracks_filename=aerial_tracks_subdir+"tracks_"+start_HHMMSS+"-"+
      stop_HHMMSS+".txt";
   ofstream tracks_stream;
   tracks_stream.precision(12);
   filefunc::openfile(tracks_filename,tracks_stream);

   tracks_stream << 
      "# Track_ID 1970_Epoch   Easting     Northing       Altitude"
                 << endl << endl;

   for (int track_counter=0; track_counter<n_tracks; track_counter++)
   {
      int track_label=track_labels[track_counter];
      cout << track_label << " " << flush;

      bool track_points_written_flag=false;
      track* aerial_track_ptr=aerial_tracks_group.get_track_ptr(track_counter);
      
      double curr_t=start_time;
      while (curr_t < stop_time)
      {
         if (aerial_track_ptr->get_interpolated_posn(
            curr_t,interp_aerial_posn) && FSF_track_ptr->get_interpolated_posn(
               curr_t,interp_FSF_posn))
         {

// Modify aerial track point's altitude to compensate for earth's
// curvature:

            geopoint aerial_track_point(
               northern_hemisphere_flag,specified_UTM_zonenumber,
               interp_aerial_posn.get(0),interp_aerial_posn.get(1),
               interp_aerial_posn.get(2));
            geopoint wisp_tangent_point(
               northern_hemisphere_flag,specified_UTM_zonenumber,
               interp_FSF_posn.get(0),interp_FSF_posn.get(1),
               interp_FSF_posn.get(2));
            double eff_track_Z=geofunc::altitude_relative_to_tangent_plane(
               wisp_tangent_point,aerial_track_point);
            interp_aerial_posn.put(2,eff_track_Z);

            threevector rel_XYZ=(interp_aerial_posn-interp_FSF_posn);
//            double range=rel_XYZ.magnitude();
            threevector r_hat=rel_XYZ.unitvector();
//            cout << "range = " << range*0.001 << " kms" << endl;
            
            if (interp_aerial_posn.get(2) > min_Z)
            {
               tracks_stream << track_label << "  "
                             << curr_t << "   "
                             << interp_aerial_posn.get(0) << "  "
                             << interp_aerial_posn.get(1) << "  "
                             << interp_aerial_posn.get(2) << "  "
                             << endl;
               track_points_written_flag=true;
            }
         }
         curr_t += dt;
      } // curr_t < stop_time while loop

      if (track_points_written_flag)
      {
         track_points_written_flag=false;
         tracks_stream << endl;
      }

   } // loop over track_counter
   cout << endl;

   filefunc::closefile(tracks_filename,tracks_stream);
   string banner="Exported "+tracks_filename;
   outputfunc::write_big_banner(banner);

// Next export tracks as functions of time rather than track number:

   typedef std::map<int,std::vector<twovector> > AZEL_MAP;
   AZEL_MAP* azel_map_ptr=new AZEL_MAP;
   AZEL_MAP::iterator iter;

// independent integer = track ID
// Dependent STL vec of twovectors = (az,el) as functions of time

   tracks_filename=aerial_tracks_subdir+"projected_tracks_"+start_HHMMSS+"-"+
      stop_HHMMSS+".txt";

   tracks_stream.precision(12);
   filefunc::openfile(tracks_filename,tracks_stream);

   tracks_stream << 
"# Pano_ID  1970_Epoch Track_ID 	Easting     Northing 	Altitude             Az 	El         n_panel      panel_U	      U      V"
                 << endl << endl;

   for (pano_time_iter=pano_time_map.begin(); pano_time_iter !=
           pano_time_map.end(); pano_time_iter++)
   {
      int pano_ID=pano_time_iter->first;
      double curr_t=pano_time_iter->second;

      u0_az_v0_el_iter=u0_az_v0_el_map.find(pano_ID);
      if (u0_az_v0_el_iter==u0_az_v0_el_map.end()) 
      {
         cout << "Error!  Couldn't find pano_ID = " << pano_ID
              << " in u0_az_v0_el_map" << endl;
         continue;
      }
      
      double elevation_lo=u0_az_v0_el_iter->second.get(1)*PI/180.0;
      double panel0_start_az=u0_az_v0_el_iter->second.get(0);
      panel0_start_az=basic_math::phase_to_canonical_interval(
         panel0_start_az,0,360);

//      cout << "pano_ID = " << pano_ID
//           << " panel0_start_az = " << panel0_start_az << endl;
   
      vector<double> panel_start_az;
      panel_start_az.push_back(panel0_start_az);

      for (int p=1; p<=9; p++)
      {
         double curr_panel_az=panel_start_az[p-1]-36;
         curr_panel_az=basic_math::phase_to_canonical_interval(
            curr_panel_az,0,360);
         panel_start_az.push_back(curr_panel_az);
//         cout << "p = " << p 
//              << " start panel az = " << panel_start_az.back()
//              << endl;
      }

      double azdot=NEGATIVEINFINITY;
      double eldot=NEGATIVEINFINITY;
      for (int track_counter=0; track_counter<n_tracks; track_counter++)
      {
         int track_label=track_labels[track_counter];
//         bool track_points_written_flag=false;
         track* aerial_track_ptr=aerial_tracks_group.get_track_ptr(
            track_counter);

         if (aerial_track_ptr->get_interpolated_posn(
            curr_t,interp_aerial_posn) && FSF_track_ptr->get_interpolated_posn(
               curr_t,interp_FSF_posn))
         {

// Modify aerial track point's altitude to compensate for earth's
// curvature:

            geopoint aerial_track_point(
               northern_hemisphere_flag,specified_UTM_zonenumber,
               interp_aerial_posn.get(0),interp_aerial_posn.get(1),
               interp_aerial_posn.get(2));
            geopoint wisp_tangent_point(
               northern_hemisphere_flag,specified_UTM_zonenumber,
               interp_FSF_posn.get(0),interp_FSF_posn.get(1),
               interp_FSF_posn.get(2));
            double eff_track_Z=geofunc::altitude_relative_to_tangent_plane(
               wisp_tangent_point,aerial_track_point);
            interp_aerial_posn.put(2,eff_track_Z);

            threevector rel_XYZ=(interp_aerial_posn-interp_FSF_posn);
//            double range=rel_XYZ.magnitude();
            threevector r_hat=rel_XYZ.unitvector();
//            cout << "range = " << range*0.001 << endl;

            double az=atan2(r_hat.get(1),r_hat.get(0));
            double el=PI/2-acos(r_hat.get(2));
//            cout << "az = " << az*180/PI << " el = " << el*180/PI << endl;

            iter=azel_map_ptr->find(track_label);
            if (iter==azel_map_ptr->end())
            {
               vector<twovector> V;
               V.push_back(twovector(az,el));
               (*azel_map_ptr)[track_label]=V;
            }
            else
            {
               vector<twovector> V=iter->second;
               twovector prev_azel=V.back();
               double prev_az=prev_azel.get(0);
               prev_az=basic_math::phase_to_canonical_interval(
                  prev_az,az-PI,az+PI);
               double d_az=az-prev_az;
               double d_el=el-prev_azel.get(1);
               azdot=d_az/dt;
               eldot=d_el/dt;

//               cout << "d_az = " << d_az*180/PI
//                    << " dt = " << dt << " azdot = " << azdot*180/PI
//                    << endl;
               iter->second.push_back(twovector(az,el));
            }

// Compute panel in which current (az,el) lies as well as its (U,V)
// for that panel:

            cout << "az = " << az*180/PI 
                 << " panel0_start_az = " << panel0_start_az << endl;
            double delta_panel_az=fabs(az*180/PI-panel0_start_az);
            delta_panel_az=basic_math::phase_to_canonical_interval(
               delta_panel_az,0,360);
            double U=delta_panel_az*Umax/360.0;

            cout << "Original delta_panel_az = " << delta_panel_az 
                 << " U = " << U << endl;

            int n_panel=basic_math::mytruncate(delta_panel_az/36);
            cout << " n_panel = " << n_panel << endl;

            delta_panel_az -= n_panel*36;

            double panel_Umax=Umax/10;
            double panel_U=U-n_panel*panel_Umax;
            
            cout << "Umax = " << Umax << endl;
            cout << "panel_U = " << panel_U << endl;

//            outputfunc::enter_continue_char();

            double V=(el-elevation_lo)/IFOV_v;
            cout << "el = " << el 
                 << " elevation_lo = " << elevation_lo 
                 << " V = " << V << endl;

            if (interp_aerial_posn.get(2) > min_Z)
            {
               if (azdot < NEGATIVEINFINITY/2) azdot=0;
               if (eldot < NEGATIVEINFINITY/2) eldot=0;

               tracks_stream << pano_ID << "   "
                             << curr_t  << "   "
                             << track_label << "   "
                             << interp_aerial_posn.get(0) << "   "
                             << interp_aerial_posn.get(1) << "   "
                             << interp_aerial_posn.get(2) << "   "
                             << az*180/PI << "  "
                             << el*180/PI << "  "
                             << n_panel << "  "
                             << panel_U << "  "
                             << U << "  "
                             << V << endl;
            }
         } // interpolated_posn conditional
      } // loop over track_counter
      tracks_stream << endl;

      curr_t += dt;
   } // curr_t < stop_time while loop
   cout << endl;

   filefunc::closefile(tracks_filename,tracks_stream);
   banner="Exported "+tracks_filename;
   outputfunc::write_big_banner(banner);

   banner="Finished running program AERIAL_TRACKS";
   outputfunc::write_banner(banner);
   outputfunc::print_elapsed_time();

}
