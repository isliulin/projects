// ==========================================================================
// Program BOW_IMAGE_COMPARISONS imports a probabilistic binary
// classifier decision function generated via SVM_GLOBAL_DESCRIPTORS.
// It also imports BoW matching scores for pairs of images.  If the
// decision function operating on an image pair's global BoW matching scores
// exceeds some user-specified threshold, the image pair is declared
// to represent a match.  A montage for the pair of matching images is
// then generated and stored within a subfolder of matches_subdir
// labeled by matching score.

//			   ./BoW_image_comparisons

// ==========================================================================
// Last updated on 12/27/13; 12/28/13; 12/31/13
// ==========================================================================

#include  <iostream>
#include  <map>
#include  <string>
#include  <vector>
#include "dlib/svm.h"

#include "general/filefuncs.h"
#include "video/descriptorfuncs.h"
#include "image/imagefuncs.h"
#include "math/ltduple.h"
#include "math/mathfuncs.h"
#include "general/outputfuncs.h"
#include "image/pngfuncs.h"
#include "math/prob_distribution.h"
#include "datastructures/Quadruple.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

int main(int argc, char* argv[])
{
   cout.precision(12);

   timefunc::initialize_timeofday_clock();      

//   bool video_frames_input_flag=true;
   bool video_frames_input_flag=false;
   bool keyframes_flag=video_frames_input_flag;
   bool different_clips_flag=false;

   if (video_frames_input_flag)
   {
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
   } // video_frames_input_flag conditional

//   bool sqrt_dotproduct_flag=true;
   bool sqrt_dotproduct_flag=false;
   bool sqrt_diff_over_sum_flag=true;
//   bool sqrt_diff_over_sum_flag=false;

// Import basic HOG BoW processing parameters:

   string gist_subdir="../gist/";
   string BoW_params_filename=gist_subdir+"BoW_params.dat";
   filefunc::ReadInfile(BoW_params_filename);
   int n_HOG_bins=stringfunc::string_to_number(filefunc::text_line[0]);
   cout << "n_HOG_bins = " << n_HOG_bins << endl;
//   string kernel_type=filefunc::text_line[1];
//   cout << "kernel_type = " << kernel_type << endl;

   const int K=2048;
   if (n_HOG_bins != K)
   {
      cout << "Error! : K = " << K << endl;
      cout << "n_HOG_bins = " << n_HOG_bins << endl;
      exit(-1);
   }

//   int max_clip_ID=stringfunc::string_to_number(filefunc::text_line[2]);
//   int max_frame_ID=stringfunc::string_to_number(filefunc::text_line[3]);
//   cout << "max_clip_ID = " << max_clip_ID << endl;
//   cout << "max_frame_ID = " << max_frame_ID << endl;

// Import probabilistic decision function generated by an SVM 
// on matching and nonmatching images by program SVM_GLOBAL_DESCRIPS:

   typedef dlib::matrix<double, K, 1> sample_type;

// LINEAR KERNEL
//   typedef dlib::linear_kernel<sample_type> kernel_type;

// HISTOGRAM INTERSECTION KERNEL (for BoW)
   typedef dlib::histogram_intersection_kernel<sample_type> kernel_type;

   sample_type sample;

   typedef dlib::probabilistic_decision_function<kernel_type> 
      probabilistic_funct_type;  
   typedef dlib::normalized_function<probabilistic_funct_type> pfunct_type;
   pfunct_type pfunct;

   string BoW_matches_subdir="./BoW_matches/";
   string learned_funcs_subdir=BoW_matches_subdir+"learned_functions/";
   string learned_pfunct_filename=learned_funcs_subdir+"pfunct.dat";

   ifstream fin6(learned_pfunct_filename.c_str(),ios::binary);
   dlib::deserialize(pfunct, fin6);

   double minimum_matching_prob=0.1;
   cout << "Enter minimum matching probability threshold:" << endl;
   cin >> minimum_matching_prob;


   typedef std::map<long long,vector<float>*> BOW_DESCRIPTOR_SCORES_MAP;
   BOW_DESCRIPTOR_SCORES_MAP BoW_descriptor_scores_map;
   BOW_DESCRIPTOR_SCORES_MAP::iterator iter,iter_start;


   dlib::array<dlib::array2d<unsigned char> > images;
   vector<string> JAV_subdirs;

   int starting_frame_ID=-1;
   if (video_frames_input_flag)
   {
      JAV_subdirs.push_back("/data/video/JAV/NewsWraps/early_Sep_2013/");
      JAV_subdirs.push_back("/data/video/JAV/NewsWraps/w_transcripts/");
      JAV_subdirs.push_back(
         "/data/ImageEngine/BostonBombing/clips_1_thru_25/");
   }
   else
   {
      JAV_subdirs.push_back("/data2/bundler/tidmarsh/");
//      cout << "Enter starting frame ID:" << endl;
//      cin >> starting_frame_ID;
   }

   for (unsigned int J=0; J<JAV_subdirs.size(); J++)
   {
      string root_subdir=JAV_subdirs[J];
      string matches_subdir=root_subdir+"matching_images__"
         +stringfunc::number_to_string(minimum_matching_prob,2)+"/";   
      filefunc::dircreate(matches_subdir);

      string images_subdir,keyframes_subdir;
      if (video_frames_input_flag)
      {
         images_subdir=root_subdir+"jpg_frames/";
         keyframes_subdir=images_subdir+"keyframes/";
         cout << "Processing keyframes from "+keyframes_subdir << endl;
      }
      else
      {
         images_subdir=root_subdir+"standard_sized_images/";
         cout << "Processing images from "+images_subdir << endl;
      }

// Store video keyframes' clip and frame IDs within STL map keyframes_map:

      typedef map<DUPLE,string> KEYFRAMES_MAP;
      KEYFRAMES_MAP keyframes_map;
      KEYFRAMES_MAP::iterator keyframes_iter;

// independent DUPLE holds video image clip and frame ID
// dependent string holds video image's filename

      if (keyframes_flag)
      {
         vector<string> keyframe_filenames=filefunc::image_files_in_subdir(
            keyframes_subdir);

         for (unsigned int i=0; i<keyframe_filenames.size(); i++)
         {
            vector<string> substrings=
               stringfunc::decompose_string_into_substrings(
                  filefunc::getbasename(keyframe_filenames[i])," _-");

// If input filename has form clip_0000_frame-00001.jpg, record clip
// and frame IDs with keyframes_map:

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

      string BoW_subdir=root_subdir+"BoWs/";
      string BoW_comparisons_subdir=BoW_subdir+"comparisons/";

// Import BoW histograms generated by program COMPUTE_BoW_HISTOGRAMS:

      vector<string> allowed_suffixes;
      allowed_suffixes.push_back("BoW_hist");
      vector<string> histogram_filenames=
         filefunc::files_in_subdir_matching_specified_suffixes(
            allowed_suffixes,BoW_subdir);
//   cout << "histogram_filenames.size() = " << histogram_filenames.size()
//        << endl;

      string banner="Importing BoW histograms from text files:";
      outputfunc::write_banner(banner);

// Store BoW histograms as functions of video keyframe clip and frame
// IDs within STL map BoW_histograms_map:

      typedef map<DUPLE,double*> BOW_HISTOGRAMS_MAP;
      BOW_HISTOGRAMS_MAP BoW_histograms_map;
      BOW_HISTOGRAMS_MAP::iterator BoW_histogram_iter1,BoW_histogram_iter2;

// independent DUPLE holds video image clip and frame ID.  If we're
// working with digital stills rather than video clips, clip_ID=0
// while frame_ID=image_ID.

// dependent double* holds BoW histogram

      int n_bins=-1;
      for (unsigned int i=0; i<histogram_filenames.size(); i++)
      {
         outputfunc::update_progress_fraction(
            i,100,histogram_filenames.size());
         string basename=filefunc::getbasename(histogram_filenames[i]);
//      cout << "basename = " << basename << endl;
      
         int clip_ID=-1;
         int frame_ID=-1;
         DUPLE curr_duple;
         if (keyframes_flag && keyframes_map.size() > 0)
         {
            vector<string> substrings=
               stringfunc::decompose_string_into_substrings(
                  stringfunc::prefix(basename)," _-");

// Ignore input histogram if its filename has form
// clip_0000_frame-00001 and a corresponding clip_ID/frame_ID duple
// entry is NOT found within keyframes_map:

            if (substrings.size()==4)
            {
               if (substrings[0]=="clip")
               {
                  clip_ID=stringfunc::string_to_number(substrings[1]);
                  frame_ID=stringfunc::string_to_number(substrings[3]);
                  curr_duple=DUPLE(clip_ID,frame_ID);
                  KEYFRAMES_MAP::iterator keyframe_iter=
                     keyframes_map.find(curr_duple);
                  if (keyframe_iter==keyframes_map.end())
                  {
                     continue;
                  }
               } // substrings[0] conditional
            } // substrings.size()==4 conditional
         }
         else
         {
            clip_ID=0;
            frame_ID=stringfunc::string_to_number(basename.substr(3,5));
            curr_duple=DUPLE(clip_ID,frame_ID);
         } // keyframes_flag conditional

         vector<double> curr_bin_values=filefunc::ReadInNumbers(
            histogram_filenames[i]);
      
         n_bins=curr_bin_values.size();
         double* curr_histogram_ptr=new double[n_bins];
         BoW_histograms_map[curr_duple]=curr_histogram_ptr;
      
         for (int b=0; b<n_bins; b++)
         {
            if (sqrt_dotproduct_flag || sqrt_diff_over_sum_flag)
            {
               (curr_histogram_ptr)[b]=sqrt(curr_bin_values[b]);
            }
            else
            {
               (curr_histogram_ptr)[b]=curr_bin_values[b];
            }
         }
      } // loop over index i labeling video keyframe HOG BoW histograms
      cout << endl;

      cout << "n_BoW_histograms = " << BoW_histograms_map.size() << endl;
      cout << "n_HOG_bins = " << n_bins << endl;

// Loop over pairs of BoW histograms.  Output BoW matching score vs
// pair image filenames to text file:

      string scores_filename=matches_subdir+"scores.dat";
      ofstream scores_stream;
      filefunc::openfile(scores_filename,scores_stream);

      scores_stream << "# matching_score image1_filename image2_filename matching_score" 
                    << endl << endl;
      scores_stream << "# Minimum matching probability threshold = "
                    << minimum_matching_prob << endl << endl;

      for (BoW_histogram_iter1=BoW_histograms_map.begin(); 
           BoW_histogram_iter1 != BoW_histograms_map.end();
           BoW_histogram_iter1++)
      {
         outputfunc::print_elapsed_time();
         DUPLE curr_duple=BoW_histogram_iter1->first;

         if (keyframes_flag && keyframes_map.size() > 0)
         {
            KEYFRAMES_MAP::iterator keyframe_iter=
               keyframes_map.find(curr_duple);
            if (keyframe_iter==keyframes_map.end())
            {
               continue;
            }
         }

         int curr_clip_ID=curr_duple.first;
         int curr_frame_ID=curr_duple.second;

         if (curr_frame_ID < starting_frame_ID) continue;

         double* curr_histogram_ptr=BoW_histogram_iter1->second;

         for (BoW_histogram_iter2=BoW_histogram_iter1;
              BoW_histogram_iter2 != BoW_histograms_map.end();
              BoW_histogram_iter2++)
         {
            if (BoW_histogram_iter2==BoW_histogram_iter1) continue;

            DUPLE next_duple=BoW_histogram_iter2->first;
            int next_clip_ID=next_duple.first;
            int next_frame_ID=next_duple.second;
            
            if (different_clips_flag && curr_clip_ID==next_clip_ID)
            {
               continue;
            }
            
            if (keyframes_flag && keyframes_map.size() > 0)
            {
               KEYFRAMES_MAP::iterator keyframe_iter=
                  keyframes_map.find(next_duple);
               if (keyframe_iter==keyframes_map.end())
               {
                  continue;
               }
            }

            double* next_histogram_ptr=BoW_histogram_iter2->second;            

// Calculate comparison score between current and next BoW histograms:

            for (int b=0; b<n_HOG_bins; b++)
            {
               float overlap_score=0;

// Recall curr and next histogram values have already been
// square-rooted if sqrt_diff_over_sum_flag==true or
// sqrt_dotproduct_flag==true:

               if (sqrt_diff_over_sum_flag)
               {
                  float sqrt_curr_hist=curr_histogram_ptr[b];
                  float sqrt_next_hist=next_histogram_ptr[b];
                  
                  float numer=sqrt_curr_hist-sqrt_next_hist;
                  float denom=sqrt_curr_hist+sqrt_next_hist;
                  if (denom > 0)
                  {
                     overlap_score=1-fabs(numer)/denom;
                  }
                  else
                  {
                     overlap_score=1;
                  }
               } // overlap score computation conditional

               sample(b)=overlap_score;

            } // loop over index b labeling HOG bins

            double match_prob=pfunct(sample);
            if (match_prob < minimum_matching_prob) continue;


            string curr_image_filename,next_image_filename;
            if (video_frames_input_flag)
            {
               curr_image_filename=images_subdir+
                  "clip_"+stringfunc::integer_to_string(curr_clip_ID,4)
                  +"_frame-"+
                  stringfunc::integer_to_string(curr_frame_ID,5)+".jpg";
               next_image_filename=images_subdir+
                  "clip_"+stringfunc::integer_to_string(next_clip_ID,4)
                  +"_frame-"+
                  stringfunc::integer_to_string(next_frame_ID,5)+".jpg";
            }
            else
            {
               curr_image_filename=images_subdir+
                  "pic"+stringfunc::integer_to_string(curr_frame_ID,5)+".jpg";
               next_image_filename=images_subdir+
                  "pic"+stringfunc::integer_to_string(next_frame_ID,5)+".jpg";
            }

//      cout << "curr_image_filename = " 
//           << filefunc::getbasename(curr_image_filename) << endl;
//      cout << "next_image_filename = " 
//           << filefunc::getbasename(next_image_filename) << endl;

//            double counter_frac=double(counter-counter_start)/
//               (BoW_histograms_map.size()-counter_start);
//            cout << "counter-counter_start = " << counter-counter_start
//                 << " n_BoW_descriptor_pairs-counter_start = " 
//                 << n_BoW_descriptor_pairs-counter_start
//                 << " counter_frac = " << counter_frac 
//                 << endl;
//            cout << "counter_frac = " << counter_frac 
//            outputfunc::print_elapsed_and_remaining_time(counter_frac);

            if (video_frames_input_flag)
            {
               cout << "clip1_ID = " << curr_clip_ID
                    << " frame1_ID = " << curr_frame_ID
                    << " clip2_ID = " << next_clip_ID
                    << " frame2_ID = " << next_frame_ID 
                    << " match prob = " 
                    << stringfunc::number_to_string(match_prob,3)
                    << endl;
            }
            else
            {
               cout << "frame1_ID = " << curr_frame_ID
                    << " frame2_ID = " << next_frame_ID 
                    << " match prob = " 
                    << stringfunc::number_to_string(match_prob,3)
                    << endl;
            }

            string unix_cmd="montageview "+curr_image_filename+" "+
               next_image_filename+" NO_DISPLAY";
            sysfunc::unix_command(unix_cmd);
            string prob_str=stringfunc::number_to_string(match_prob,2);

            string montage_jpg_filename=
               "montage_"+filefunc::getprefix(curr_image_filename)+"___"+
               filefunc::getbasename(next_image_filename);
            string montage_w_score_jpg_filename=
               prob_str+"_montage_"+filefunc::getprefix(curr_image_filename)+
               "___"+filefunc::getprefix(next_image_filename)+".jpg";

            scores_stream << match_prob << " "
                          << filefunc::getprefix(curr_image_filename) << " "
                          << filefunc::getprefix(next_image_filename) 
                          << endl;

            double trunc_prob=0.1*basic_math::mytruncate(match_prob/0.1);
            prob_str=stringfunc::number_to_string(trunc_prob,1);
            string prob_subdir=matches_subdir+prob_str+"/";
            filefunc::dircreate(prob_subdir);

            unix_cmd="mv "+montage_jpg_filename+" "+
               prob_subdir+montage_w_score_jpg_filename;
            sysfunc::unix_command(unix_cmd);

         } // loop over BoW_histogram_iter2
      } // loop over BoW_histograms_iter1
      
      filefunc::closefile(scores_filename,scores_stream);

// Purge dynamically instantiated BoW histograms

      for (BoW_histogram_iter1=BoW_histograms_map.begin(); 
           BoW_histogram_iter1 != BoW_histograms_map.end();
           BoW_histogram_iter1++)
      {
         double* curr_histogram_ptr=BoW_histogram_iter1->second;
         delete curr_histogram_ptr;
      }

   } // loop over index J labeling JAV subdirs

   cout << "At end of program BOW_IMAGE_COMPARISONS" << endl;
   outputfunc::print_elapsed_time();
}

