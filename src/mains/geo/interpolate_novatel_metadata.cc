// ========================================================================
// Program INTERPOLATE_NOVATEL_METADATA reads in the metadata ascii
// file

//			  interpolate_novatel_metadata

// ========================================================================
// Last updated on 9/8/11
// ========================================================================

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "geometry/bounding_box.h"
#include "astro_geo/Clock.h"
#include "general/filefuncs.h"
#include "astro_geo/geopoint.h"
#include "geometry/polyline.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "track/tracks_group.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::ofstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

   tracks_group GPStracks_group;
   track* GPS_track_ptr=GPStracks_group.generate_new_track();

   string GPS_waypoints_filename="GPS_waypoints.txt";
   filefunc::ReadInfile(GPS_waypoints_filename);
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      cout << filefunc::text_line[i] << endl;
      vector<double> column_values=stringfunc::string_to_numbers(
         filefunc::text_line[i]); 
      double curr_t=column_values[1];
      double curr_x=column_values[2];
      double curr_y=column_values[3];
      double curr_z=column_values[4];
      threevector curr_posn(curr_x,curr_y,curr_z);
      threevector curr_velocity(0,0,0);
      
      GPS_track_ptr->set_XYZ_coords(curr_t,curr_posn);
   }
   
   cout.precision(10);
//   cout << "*GPS_track_ptr = " << *GPS_track_ptr << endl;
//   outputfunc::enter_continue_char();

//   int start_frame_number=125;
//   int stop_frame_number=310;
//   double start_time=1306330241.22;  // CreditUnion_May25_flight1_0125.rd.jpg 
//   double stop_time= 1306330443.23;  // CreditUnion_May25_flight1_0310.rd.jpg 

//   int start_frame_number=181;
//   int stop_frame_number=1999;
//   double start_time=1305400500.9;	// GC clip #5
//   double stop_time=1305402574.9;	// GC clip #5

   int start_frame_number=1;
   int stop_frame_number=1478;
   double start_time=1305400500.9;	// GC clip #6
   double stop_time= 1305404285.4;      // GC clip #6

   string output_filename="interp_GPS_waypoints.txt";
   ofstream outstream;
   outstream.precision(12);
   filefunc::openfile(output_filename,outstream);
   outstream << "# Filename      Epoch Time (s) Easting (m)    Northing (m) Altitude (m)" << endl;
   outstream << endl;
 
   threevector curr_posn;
   for (int frame_number=start_frame_number; frame_number <= stop_frame_number;
        frame_number++)
   {
      double curr_time=start_time+(stop_time-start_time)/(stop_frame_number-
        start_frame_number)*(frame_number-start_frame_number);

//      if (GPS_track_ptr->get_interpolated_posn(curr_time,curr_posn))
      GPS_track_ptr->get_interpolated_posn(curr_time,curr_posn);
      {
//         string image_filename="CreditUnion_May25_flight1_"
//            +stringfunc::integer_to_string(frame_number,4)+".jpg";
//         string image_filename="GC_FLIR_clip5_-"
//            +stringfunc::integer_to_string(frame_number,5)+".jpg";
         string image_filename="GCFLIR"
            +stringfunc::integer_to_string(frame_number,5)+".jpg";

         outstream << image_filename << "  "
                   << stringfunc::number_to_string(curr_time,2) << "  "
                   << stringfunc::number_to_string(curr_posn.get(0),2) << "  "
                   << stringfunc::number_to_string(curr_posn.get(1),2) << "  "
                   << stringfunc::number_to_string(curr_posn.get(2),2) << endl;
         
         cout << "frame = " << frame_number
              << " t = " << curr_time 
              << " x = " << curr_posn.get(0)
              << " y = " << curr_posn.get(1)
              << " z = " << curr_posn.get(2) << endl;
      }
   } // loop over frame_number index
   

   filefunc::closefile(output_filename,outstream);


}


