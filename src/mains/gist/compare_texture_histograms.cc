// ==========================================================================
// Program COMPARE_TEXTURE_HISTOGRAMS imports a set of texture
// histograms generated by program COMPUTE_TEXTURE_HISTOGRAMS.
// Looping over all imported histograms, it computes dotproducts
// between successive pairs.  The dotproducts are sorted in descending
// order and exported to an output text file with the corresponding
// pair's image filenames.

//			  ./compare_texture_histograms

// ==========================================================================
// Last updated on 10/28/13; 11/8/13; 12/21/13
// ==========================================================================

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "optimum/emdL1.h"
#include "general/filefuncs.h"
#include "math/ltduple.h"
#include "templates/mytemplates.h"
#include "general/outputfuncs.h"
#include "math/prob_distribution.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::ios;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

   bool keyframes_flag=true;
   bool different_clips_flag=false;

   string input_char;
   cout << "Enter 'k' to process just video keyframes:" << endl;
   cin >> input_char;
   if (input_char != "k")
   {
      keyframes_flag=false;
   }
   cout << "Enter 'w' to compare histograms within and not between clips:"
        << endl;
   cin >> input_char;
   if (input_char != "w")
   {
      different_clips_flag=true;
   }

   string ImageEngine_subdir="/data/ImageEngine/";
   string BostonBombing_subdir=ImageEngine_subdir+"BostonBombing/";
   string JAV_subdir=BostonBombing_subdir+"clips_1_thru_25/";
   string root_subdir=JAV_subdir;
   string images_subdir=root_subdir+"jpg_frames/";
   string keyframes_subdir=images_subdir+"keyframes/";
   double min_dotproduct=0.0;
   if (!different_clips_flag) min_dotproduct=0.79;

/*
//   string JAV_subdir="/data/video/JAV/NewsWraps/early_Sep_2013/";
   string JAV_subdir="/data/video/JAV/NewsWraps/w_transcripts/";
//   string JAV_subdir="/data/video/JAV/UIUC_Broadcast_News/";
   string root_subdir=JAV_subdir;
   string images_subdir=root_subdir+"jpg_frames/";
   string keyframes_subdir=images_subdir+"keyframes/";
//   double min_dotproduct=0.6423;	// NewsWraps
   double min_dotproduct=0.4;	// NewsWraps
//   if (!different_clips_flag) min_dotproduct=0.80;
  if (!different_clips_flag) min_dotproduct=0.79;
//   if (!different_clips_flag) min_dotproduct=0.78;
*/


/*
   string tidmarsh_subdir=ImageEngine_subdir+"tidmarsh/";
   string images_subdir=tidmarsh_subdir;
   string root_subdir=tidmarsh_subdir;
   const double min_dotproduct=0.6;
*/

// Report all texture matching scores when we're comparing keyframes
// within same video clips:

   if (keyframes_flag && !different_clips_flag) min_dotproduct=0;

// Store keyframes' clip and frame IDs within STL map keyframes_map:

   typedef map<DUPLE,string> KEYFRAMES_MAP;
   KEYFRAMES_MAP keyframes_map;

// independent DUPLE holds video image clip and frame ID
// dependent string holds video image's filename

   if (keyframes_flag)
   {
      vector<string> keyframe_filenames=filefunc::image_files_in_subdir(
         keyframes_subdir);

      for (int i=0; i<keyframe_filenames.size(); i++)
      {
         vector<string> substrings=
            stringfunc::decompose_string_into_substrings(
               filefunc::getbasename(keyframe_filenames[i])," _-");
//         for (int s=0; s<substrings.size(); s++)
//         {
//            cout << "i = " << i << " s = " << s << " substring[s] = " 
//                 << substrings[s] << endl;
//         }

// If input filename has form clip_0000_frame-00001.jpg, record clip's
// ID within STL vector:

         int clip_ID=-1;
         int frame_ID=-1;
         if (substrings.size()==4)
         {
            if (substrings[0]=="clip")
            {
               clip_ID=stringfunc::string_to_number(substrings[1]);
               frame_ID=stringfunc::string_to_number(substrings[3]);
               keyframes_map[DUPLE(clip_ID,frame_ID)]=keyframe_filenames[i];
            }
         }
      } // loop over index i labeling keyframe filenames
      
   } // keyframes_flag conditional
   cout << "keyframes_map.size() = " << keyframes_map.size() << endl;

   string texture_histograms_subdir=root_subdir+"texture_histograms/";

   vector<string> allowed_suffixes;
   allowed_suffixes.push_back("texture_hist");
   vector<string> histogram_filenames=
      filefunc::files_in_subdir_matching_specified_suffixes(
         allowed_suffixes,texture_histograms_subdir);
   int n_images=histogram_filenames.size();

//   int n_colors=1;
   int n_colors=3;
   int n_horiz_subimage_bins=6;
   int n_vert_subimage_bins=6;
   int n_frac_mag_bins=7;
   int n_theta_bins=4;
   int n_bins=n_colors*n_horiz_subimage_bins*n_vert_subimage_bins*
      n_frac_mag_bins*n_theta_bins;
//   cout << "n_histogram_files = " << histogram_filenames.size() << endl;
//   cout << "n_bins = " << n_bins << endl;

   timefunc::initialize_timeofday_clock();      

// Import texture histograms generated by program
// COMPUTE_TEXTURE_HISTOGRAMS:

   string banner="Importing texture histograms from text files:";
   outputfunc::write_banner(banner);

   vector<string> hist_filenames;
   vector<double*> histogram_ptrs;
   for (int i=0; i<histogram_filenames.size(); i++)
   {
      outputfunc::update_progress_fraction(i,100,histogram_filenames.size());

      if (keyframes_flag && keyframes_map.size() > 0)
      {
         string basename=filefunc::getbasename(histogram_filenames[i]);
         vector<string> substrings=
            stringfunc::decompose_string_into_substrings(
               stringfunc::prefix(basename)," _-");
//         for (int s=0; s<substrings.size(); s++)
//         {
//            cout << " s = " << s << " substring[s] = " 
//                 << substrings[s] << endl;
//         }

// If input filename has form clip_0000_frame-00001, record clip's
// ID within STL vector:

         int clip_ID=-1;
         int frame_ID=-1;
         if (substrings.size()==4)
         {
            if (substrings[0]=="clip")
            {
               clip_ID=stringfunc::string_to_number(substrings[1]);
               frame_ID=stringfunc::string_to_number(substrings[3]);
               KEYFRAMES_MAP::iterator keyframe_iter=keyframes_map.find(
                  DUPLE(clip_ID,frame_ID));
               if (keyframe_iter==keyframes_map.end())
               {
                  continue;
               }
//               else
//               {
//                  cout << "clip_ID = " << clip_ID << " frame_ID = " << frame_//ID
//                       << " is a keyframe" << endl;
//               }
            }
         } // substrings.size() conditional
      } // keyframes_flag conditional

      vector<double> curr_bin_values=filefunc::ReadInNumbers(
         histogram_filenames[i]);
      double* curr_histogram_ptr=new double[n_bins];
      histogram_ptrs.push_back(curr_histogram_ptr);
      hist_filenames.push_back(histogram_filenames[i]);

      for (int j=0; j<curr_bin_values.size(); j++)
      {
         (curr_histogram_ptr)[j]=curr_bin_values[j];
      }
   } // loop over index i labeling input histogram files

   int n_histograms=histogram_ptrs.size();
   cout << "n_histograms = " << n_histograms << endl;


   cout << endl;
   cout << "Computing texture histogram dotproducts:" << endl;
   cout << endl;

//   bool conventional_dotproduct_flag=true;
   bool conventional_dotproduct_flag=false;
   int i_max=n_histograms;
   int j_max=i_max;

   vector<int> basename_i_indices,basename_j_indices;
   vector<float> texture_dotproducts;

   for (int i=0; i<i_max-1; i++)
   {
      outputfunc::update_progress_fraction(i,100,n_histograms);
      double* curr_histogram_ptr=histogram_ptrs[i];

      double curr_norm=0;
      if (conventional_dotproduct_flag)
      {
         for (int b=0; b<n_bins; b++) 
         {
            curr_norm += sqr(curr_histogram_ptr[b]);
         }
         curr_norm=sqrt(curr_norm);
      }

//       if (!different_clips_flag) j_max=basic_math::min(i+2,i_max);
      
      for (int j=i+1; j<j_max; j++)
      {
         double* next_histogram_ptr=histogram_ptrs[j];

         double dotproduct=0;
         if (conventional_dotproduct_flag)
         {
            double next_norm=0;
            for (int b=0; b<n_bins; b++)
            {
               dotproduct += curr_histogram_ptr[b]*next_histogram_ptr[b];
               next_norm += sqr(next_histogram_ptr[b]);
            }
            next_norm=sqrt(next_norm);
            dotproduct /= (curr_norm*next_norm);
         }
         else
         {

// Nonparametric dissimilarity measure:

            for (int b=0; b<n_bins; b++)
            {
               double numer=fabs(next_histogram_ptr[b]-curr_histogram_ptr[b]);
               double denom=fabs(next_histogram_ptr[b]+curr_histogram_ptr[b]);

               if (nearly_equal(numer,0) || nearly_equal(denom,0)) continue;
               dotproduct += numer/denom;
            }
            dotproduct /= n_bins;

// Transform "dotproduct" so that unity [zero] value indicates
// similarity [dissimilarity]:

            dotproduct=1-dotproduct;

//            cout << "Nonparameteric dissimilarity measure = " 
// 		   << dotproduct << endl;
         }

         if (dotproduct < min_dotproduct) continue;

         texture_dotproducts.push_back(dotproduct);

         basename_i_indices.push_back(i);
         basename_j_indices.push_back(j);

      } // loop over index j 
   } // loop over index i 

   cout << "texture_dotproducts.size() = " << texture_dotproducts.size() 
        << endl;
   
   prob_distribution prob(texture_dotproducts,100,0);
   prob.set_densityfilenamestr(
      texture_histograms_subdir+"texture_matches_density.meta");
   prob.set_cumulativefilenamestr(
      texture_histograms_subdir+"texture_matches_cum.meta");
   prob.writeprobdists(false);


   templatefunc::Quicksort_descending(
      texture_dotproducts,basename_i_indices,basename_j_indices);

   string output_filename=texture_histograms_subdir;
   if (different_clips_flag)
   {
      output_filename += "image_textures.comparison";
   }
   else
   {
      output_filename += "image_textures_sameclips.comparison";
   }

   if (keyframes_flag)
   {
      output_filename += ".keyframes";
   }

   ofstream outstream;
   filefunc::openfile(output_filename,outstream);
   outstream << "# Dotproduct    Image_prefix   Image'_prefix   " 
             << endl << endl;
   outstream << "# min_dotproduct = " << min_dotproduct << endl << endl;

   for (int t=0; t<texture_dotproducts.size(); t++)
   {
      string basename_i=filefunc::getbasename(
         hist_filenames[basename_i_indices[t]]);
      string basename_j=filefunc::getbasename(
         hist_filenames[basename_j_indices[t]]);
      string image_pair_names=
         stringfunc::prefix(basename_i)+"  "+
         stringfunc::prefix(basename_j);
//      cout << "t = " << t
//           << " image_pair_names = " << image_pair_names << endl;

      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         image_pair_names," _-");

      int clip_ID_i=-1;
      int clip_ID_j=-2;
      int frame_ID_i,frame_ID_j;
      if (substrings.size()==8)
      {
         if (substrings[0]=="clip")
         {
            clip_ID_i=stringfunc::string_to_number(substrings[1]);
            frame_ID_i=stringfunc::string_to_number(substrings[3]);
            clip_ID_j=stringfunc::string_to_number(substrings[5]);
            frame_ID_j=stringfunc::string_to_number(substrings[7]);
//            cout << "clip_ID_i = " << clip_ID_i
//                 << " clip_ID_j = " << clip_ID_j
//                 << endl;
         }
      } 
      else if (substrings.size()==4)
      {
         if (substrings[0]=="frame")    // July 2011 NewsWrap 
         {
            clip_ID_i=0;
            frame_ID_i=stringfunc::string_to_number(substrings[1]);
            clip_ID_j=0;
            frame_ID_j=stringfunc::string_to_number(substrings[3]);
         }
      } 
      else if (substrings.size()==10)
      {
         if (substrings[0]=="clip")    // Boston bombing YouTube
         {
            clip_ID_i=stringfunc::string_to_number(substrings[1]);
            frame_ID_i=stringfunc::string_to_number(substrings[3]);
            clip_ID_j=stringfunc::string_to_number(substrings[6]);
            frame_ID_j=stringfunc::string_to_number(substrings[8]);
         }
         else if (substrings[0]=="transcripted") // transcripted NL clip
         {
            clip_ID_i=stringfunc::string_to_number(substrings[2]);
            frame_ID_i=stringfunc::string_to_number(substrings[4]);
            clip_ID_j=stringfunc::string_to_number(substrings[7]);
            frame_ID_j=stringfunc::string_to_number(substrings[9]);
         }
      } // substrings.size() conditional


      if ( ((clip_ID_i != clip_ID_j) && different_clips_flag) ||
      ((clip_ID_i==clip_ID_j) && 
//      (frame_ID_j==frame_ID_i+1) && 
      !different_clips_flag) )
      {
         outstream << texture_dotproducts[t] << "   "
                   << image_pair_names << endl;
      }
   } // loop over index t labeling texture histogram dotproducts
   filefunc::closefile(output_filename,outstream);

   banner="Exported image texture histogram comparison to "+
      output_filename;
   outputfunc::write_big_banner(banner);

   cout << "At end of program COMPARE_TEXTURE_HISTOGRAMS" << endl;
   outputfunc::print_elapsed_time();
}
