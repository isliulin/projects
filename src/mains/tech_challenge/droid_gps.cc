// ========================================================================
// Program DROID_GPS parses the GPS log file generated by a DROID cell
// phone.  It extracts instantaneous time stamp, longitude and
// latitude values.  After alpha-filtering the raw geocoordinates,
// this program broadcasts the filtered positions so that they can be
// displayed as a moving dot within the Blue Force Tracker thin client.

//				droid_gps

// ========================================================================
// Last updated on 6/18/10; 6/20/10; 8/23/10; 12/4/10
// ========================================================================

#include <iostream>
#include "astro_geo/Clock.h"
#include "math/constant_vectors.h"
#include "general/filefuncs.h"
#include "filter/filterfuncs.h"
#include "astro_geo/GPS_datastream.h"
#include "math/mathfuncs.h"
#include "messenger/Messenger.h"
#include "track/mover_funcs.h"
#include "math/statevector.h"
#include "general/sysfuncs.h"
#include "general/stringfuncs.h"
#include "track/tracks_group.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::ofstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// New TOC database conventions:

   int fieldtest_ID=2;	// Bike ride around LL hill on Tues 7/27 around 4 pm
   int mission_ID=2;	// 
   int platform_ID=2;	// Peter's bike
   int sensor_ID=2;	// Brian's webcam

   string subdir="/data/tech_challenge/droid/GPS_tracks/";
//   string logfilename="june16_droid_gps.txt";
   string logfilename=subdir+
//      "20100626_154557_60_1277581557061_location_info_gps.txt";
      "20100822_112351_870_1282490631871_location_info_gps.txt";
   cout << "logfilename = " << logfilename << endl;

   if (!filefunc::fileexist(logfilename))
   {
      cout << "Droid log file doesn't exist!" << endl;
      exit(-1);
   }

// ========================================================================

   Clock clock;

   int UTM_zone=19;	// Boston, MA
   clock.compute_UTM_zone_time_offset(UTM_zone);
   cout << "UTM_zone time offset = " << clock.get_UTM_zone_time_offset()
        << endl;

   tracks_group gps_tracks_group;
   track* gps_track_ptr=gps_tracks_group.generate_new_track();

// Instantiate GPS messenger:

   string broker_URL="tcp://127.0.0.1:61616";
   cout << "ActiveMQ broker_URL = " << broker_URL << endl;

   string GPS_message_queue_channel_name="GPS";
   Messenger GPS_messenger( broker_URL, GPS_message_queue_channel_name );

// Open droid log file containing GPS information:

   mover_func::parse_DroidGPS_logfile(logfilename,clock,gps_track_ptr);

// Write out SQL insertion commands so that GPS track can be stored
// within track_points table of TOC database:

   string SQL_track_points_filename="insert_track_points.sql";
   gps_track_ptr->write_SQL_insert_track_commands(
      fieldtest_ID,mission_ID,platform_ID,sensor_ID,
      SQL_track_points_filename);
}



