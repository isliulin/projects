// ==========================================================================
// spaceinv class member function definitions
// ==========================================================================
// Last modified on 12/5/16; 12/7/16; 12/8/16; 12/9/16
// ==========================================================================

#include <iostream>
#include <string>
#include "math/basic_math.h"
#include "general/filefuncs.h"
#include "numrec/nrfuncs.h"
#include "games/spaceinv.h"
#include "video/videofuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::map;
using std::ofstream;
using std::ostream;
using std::pair;
using std::string;
using std::vector;

// ---------------------------------------------------------------------
// Initialization, constructor and destructor functions:
// ---------------------------------------------------------------------

void spaceinv::initialize_output_subdirs()
{
   output_subdir="./cropped_frames/";
   filefunc::dircreate(output_subdir);
   orig_subdir=output_subdir+"orig_frames/";
   filefunc::dircreate(orig_subdir);
   pooled_subdir=output_subdir+"pooled_frames/";
   filefunc::dircreate(pooled_subdir);
   differenced_subdir=output_subdir+"differenced_frames/";
   filefunc::dircreate(differenced_subdir);
}

void spaceinv::initialize_grayscale_output_buffer()
{
   ALEScreen curr_screen = ale.getScreen();
   screen_width = curr_screen.width();
   screen_height = curr_screen.height();
   int n_pixels = screen_width * screen_height;
   grayscale_output_buffer.reserve(n_pixels);
}

void spaceinv::initialize_member_objects()
{
//    cout << "inside spaceinv::init_member_objs()" << endl;
   
   int random_seed = 1000 * nrfunc::ran1();
   ale.setInt("random_seed", random_seed);

   ale.setFloat("repeat_action_probability", 0);
//   ale.setFloat("repeat_action_probability", 0.25);
   ale.setBool("display_screen", false);
//   ale.setBool("display_screen", true);
   ale.loadROM("/usr/local/ALE/roms/space_invaders.bin");


// No screen content influencing game play appears outside following
// pixel bbox [but which retains top region for mother ship!]:

   min_px = 20;
   max_px = 140;
   min_py = 10;
   max_py = 190;

//   min_px = 28;
//   max_px = 132;
//   min_py = 12;
//   max_py = 188;

   n_reduced_xdim = (max_px - min_px) / 2;   // 52  --> 60
//   n_reduced_ydim = (max_py - min_py) / 2;   // 88  --> 90
   n_reduced_ydim = (max_py - min_py) / 3;   // 88  --> 90
   n_reduced_pixels = n_reduced_xdim * n_reduced_ydim;  // 4717 

// No screen content influencing game play appears before following
// episode frame number:

   min_episode_framenumber = 100;

   max_score_per_episode = 1000;  // Reasonable guestimate

   screen_state_counter = 0;

   mu_z = 20.2752;	// Estimate from 70 random episodes
   sigma_z = 45.0156;   // Estimate from 70 random episodes
   mu_zdiff = 0.94;     // Estimate from few hundred random episodes
   sigma_zdiff = 11.0;  // Estimated from few hundred random episodes

   difference_counter = 0;

   initialize_output_subdirs();
   initialize_grayscale_output_buffer();
}		       

void spaceinv::allocate_member_objects()
{
   screen0_state_ptr = new genvector(n_reduced_pixels);
   screen1_state_ptr = new genvector(n_reduced_pixels);
   curr_state_ptr = screen0_state_ptr;
   next_state_ptr = screen1_state_ptr;

   for(int s = 0; s < n_screen_states; s++)
   {
      genvector* curr_screen_state_ptr = new genvector(n_reduced_pixels);
      curr_screen_state_ptr->clear_values();
      screen_state_ptrs.push_back(curr_screen_state_ptr);
   }

   curr_big_state_ptr = new genvector(n_screen_states * n_reduced_pixels);
   curr_big_state_ptr->clear_values();
}		       

// ---------------------------------------------------------------------
// Member function update_curr_big_state() assembles the contents of
// screen_state_ptrs[screen_state_counter],
// screen_state_ptrs[screen_state_counter+1 mod n_screen_states], ...,
// screen_state_ptrs[screen_state_counter+n_screen_states-1 mod
// n_screen_states] into member genvector *curr_big_state_ptr.

genvector* spaceinv::update_curr_big_state()
{
   int big_index = 0;
   for(int s = screen_state_counter - (n_screen_states - 1); 
       s <= screen_state_counter; s++)
   {
      int reduced_s = modulo(s, n_screen_states);
      for(int i = 0; i < n_reduced_pixels; i++)
      {
         curr_big_state_ptr->put(
            big_index, screen_state_ptrs[reduced_s]->get(i));
         big_index++;
      }
   } // loop over index s labeling screen states
   return curr_big_state_ptr;
}

// ---------------------------------------------------------------------
spaceinv::spaceinv(int n_screen_states)
{
   this->n_screen_states = n_screen_states;
   initialize_member_objects();
   allocate_member_objects();
}

// Copy constructor:

spaceinv::spaceinv(const spaceinv& S)
{
//   docopy(T);
}

// ---------------------------------------------------------------------
spaceinv::~spaceinv()
{
   delete screen0_state_ptr;
   delete screen1_state_ptr;

   for(int s = 0; s < n_screen_states; s++)
   {
      delete screen_state_ptrs[s];
   }
}

// ==========================================================================

void spaceinv::crop_pool_difference_curr_frame(bool export_frames_flag)
{
   int curr_framenumber = ale.getEpisodeFrameNumber();
//   cout << "inside spaceinv::crop_pool_difference_curr_frame()" << endl;
//   cout << "curr framenumber = " << curr_framenumber << endl;

// Retrieve curr frame's greyscale pixel values:

   grayscale_output_buffer.clear();
   ale.getScreenGrayscale(grayscale_output_buffer);

// Crop out center RoI from current frame:

   vector<vector<unsigned char > > byte_array;
   for(int py = min_py; py < max_py; py++)
   {
      vector<unsigned char> curr_byte_row;
      for(int px = min_px; px <  max_px; px++)
      {
         int curr_pixel = px + py * screen_width;
         curr_byte_row.push_back(grayscale_output_buffer[curr_pixel]);
      } // loop over px
      byte_array.push_back(curr_byte_row);
   } // loop over py

   if(export_frames_flag)
   {
      string output_frame = orig_subdir+"frame_"+
         stringfunc::integer_to_string(curr_framenumber,5)+".png";
      videofunc::write_8bit_greyscale_pngfile(byte_array, output_frame);
   }

// Ping-pong pooled_byte_array and other_pooled_byte_array:
      
   if(difference_counter == 0)
   {
      pooled_byte_array_ptr = &pooled_byte_array0;
      other_pooled_byte_array_ptr = &pooled_byte_array1;
   }
   else
   {
      pooled_byte_array_ptr = &pooled_byte_array1;
      other_pooled_byte_array_ptr = &pooled_byte_array0;
   }

// 2x2 max pool cropped center for current frame:

   for(unsigned int r = 0; r < byte_array.size(); r += 2)
   {
      vector<unsigned char> pooled_byte_row;
      for (unsigned int c = 0; c < byte_array[0].size(); c+= 2)
      {
         unsigned char z00 = byte_array[r][c];
         unsigned char z01 = byte_array[r][c+1];
         unsigned char z10 = byte_array[r+1][c];
         unsigned char z11 = byte_array[r+1][c+1];
         unsigned char max_z = basic_math::max(z00, z01, z10, z11);
         pooled_byte_row.push_back(max_z);

//         double zpool(max_z);
//         pooled_scrn_values.push_back(zpool);
      } // loop over index c
      pooled_byte_array_ptr->push_back(pooled_byte_row);
   } // loop over index r

   if(export_frames_flag)
   {
      string pooled_frame = pooled_subdir+"pooled_frame_"+
         stringfunc::integer_to_string(curr_framenumber,5)+".png";
      videofunc::write_8bit_greyscale_pngfile(
         *pooled_byte_array_ptr, pooled_frame);
   }

// Compute differences between current and other pooled byte arrays:

   vector<vector<unsigned char > > diff_pooled_byte_array;
   if(other_pooled_byte_array_ptr->size() > 0)
   {
      int p = 0;
      for(unsigned int py = 0; py < pooled_byte_array_ptr->size(); py++)
      {
         vector<unsigned char> diff_pooled_byte_row;
         for(unsigned int px = 0; 
             px < pooled_byte_array_ptr->at(0).size(); px++)
         {
            unsigned char z_diff = 
               pooled_byte_array_ptr->at(py).at(px) - 
               other_pooled_byte_array_ptr->at(py).at(px);
            double ren_z_diff = (double(z_diff) - mu_zdiff) / sigma_zdiff;

            if(difference_counter == 0)
            {
               screen0_state_ptr->put(p++, ren_z_diff);
            }
            else
            {
               screen1_state_ptr->put(p++, ren_z_diff);
            }

            diff_pooled_byte_row.push_back(z_diff);
         } // loop over px
         diff_pooled_byte_array.push_back(diff_pooled_byte_row);
      } // loop over py 

      if(export_frames_flag)
      {
         string diff_frame = differenced_subdir+"differenced_frame_"+
            stringfunc::integer_to_string(curr_framenumber,5)+".png";
         videofunc::write_8bit_greyscale_pngfile(
            diff_pooled_byte_array, diff_frame);
      }
   } // other_pooled_byte_array.size > 0 conditional
               
// Clear other pooled byte array:
   for(unsigned int py = 0; py < other_pooled_byte_array_ptr->size(); py++)
   {
      other_pooled_byte_array_ptr->at(py).clear();
   }
   other_pooled_byte_array_ptr->clear();
   
   pingpong_curr_and_next_states();
   difference_counter = 1 - difference_counter;
}

// ---------------------------------------------------------------------
// Member function pingpong_curr_and_next_states() resets
// curr_state_ptr and next_state_ptr to point to screen0_state_ptr and
// streen1_state_ptr based upon the current value of
// difference_counter.

void spaceinv::pingpong_curr_and_next_states()
{
   if(difference_counter == 0)
   {
      curr_state_ptr = screen1_state_ptr;
      next_state_ptr = screen0_state_ptr;
   }
   else
   {
      curr_state_ptr = screen0_state_ptr;
      next_state_ptr = screen1_state_ptr;
   }

   if(screen_state_counter == 0)
   {
      curr_state_ptr = screen_state_ptrs[1];
      next_state_ptr = screen_state_ptrs[0];
   }
   else if (screen_state_counter == 1)
   {
      curr_state_ptr = screen_state_ptrs[0];
      next_state_ptr = screen_state_ptrs[1];
   }
   
//   cout << "inside spacenv::pingpong_curr_and_next_states()" << endl;
//   cout << "|next_s - curr_s| = " 
//        << (*next_state_ptr - *curr_state_ptr).magnitude() 
//        << " diff_counter = " << difference_counter << endl;
}

// ---------------------------------------------------------------------
// Member function crop_pool_curr_frame()

void spaceinv::crop_pool_curr_frame(bool export_frames_flag)
{
   int curr_framenumber = ale.getEpisodeFrameNumber();
//   cout << "inside spaceinv::crop_pool_curr_frame()" << endl;
//   cout << "curr framenumber = " << curr_framenumber << endl;



// Retrieve curr frame's greyscale pixel values:

   grayscale_output_buffer.clear();
   ale.getScreenGrayscale(grayscale_output_buffer);

// Crop out center RoI from current frame:

   vector<vector<unsigned char > > byte_array;
   for(int py = min_py; py < max_py; py++)
   {
      vector<unsigned char> curr_byte_row;
      for(int px = min_px; px <  max_px; px++)
      {
         int curr_pixel = px + py * screen_width;
         curr_byte_row.push_back(grayscale_output_buffer[curr_pixel]);
      } // loop over px
      byte_array.push_back(curr_byte_row);
   } // loop over py

   if(export_frames_flag)
   {
      string output_frame = orig_subdir+"frame_"+
         stringfunc::integer_to_string(curr_framenumber,5)+".png";
      videofunc::write_8bit_greyscale_pngfile(byte_array, output_frame);
   }

   screen_state_counter = curr_framenumber;
   genvector *curr_screen_state_ptr = screen_state_ptrs[
      screen_state_counter % n_screen_states];

   pooled_byte_array_ptr = &pooled_byte_array0;

// rskip x cskip max pool cropped center for current frame:

   int reduced_pixel_counter = 0;
   const int rskip = 3;
//   const int rskip = 2;
   const int cskip = 2;
   for(unsigned int r = 0; r < byte_array.size(); r += rskip)
   {
      vector<unsigned char> pooled_byte_row;
      for (unsigned int c = 0; c < byte_array[0].size(); c+= cskip)
      {
         unsigned char z00 = byte_array[r][c];
         unsigned char z01 = byte_array[r][c+1];
         unsigned char z10 = byte_array[r+1][c];
         unsigned char z11 = byte_array[r+1][c+1];
         unsigned char z20 = byte_array[r+2][c];
         unsigned char z21 = byte_array[r+2][c+1];
         unsigned char max_z = basic_math::max(z00, z01, z10, z11, z20, z21);

         double zpool(max_z);

         double ren_zpool = (zpool - mu_z) / sigma_z;
         curr_screen_state_ptr->put(reduced_pixel_counter, ren_zpool);
         reduced_pixel_counter++;

         pooled_byte_row.push_back(max_z);
      } // loop over index c
      pooled_byte_array_ptr->push_back(pooled_byte_row);
   } // loop over index r

   if(export_frames_flag)
   {
      string pooled_frame = pooled_subdir+"pooled_frame_"+
         stringfunc::integer_to_string(curr_framenumber,5)+".png";
      videofunc::write_8bit_greyscale_pngfile(
         *pooled_byte_array_ptr, pooled_frame);
   }

// Clear other pooled byte array:
   for(unsigned int py = 0; py < pooled_byte_array_ptr->size(); py++)
   {
      pooled_byte_array_ptr->at(py).clear();
   }
   pooled_byte_array_ptr->clear();

}

// ---------------------------------------------------------------------
void spaceinv::mu_and_sigma_for_pooled_zvalues()
{
   double mu, sigma;
   mathfunc::mean_and_std_dev(pooled_scrn_values, mu, sigma);
   cout << "pooled_scrn_values.size = " << pooled_scrn_values.size()
        << endl;
   cout << "mu = " << mu << " sigma = " << sigma << endl;
}

// ---------------------------------------------------------------------
// Member function save_screen() writes out an RGB image corresponding
// to the current ALE screen.

void spaceinv::save_screen(string curr_screen_filename)
{
   string curr_screen_path = screen_exports_subdir + curr_screen_filename;
   screen_exporter_ptr->save(ale.getScreen(), curr_screen_path);
//   cout << "Exported " << curr_screen_path << endl;
}
