// ========================================================================
// Program CLASSIFY_GENDERS is a variant of the CLASSIFY_CHARS.  It
// takes in a caffe model trained from scratch along with folder
// containing a set of input test images.  CLASSIFY_GENDERS loops over
// each test image and prints out the label for the top class which
// the trained caffe model predicts for the input image.  It exports
// correctly and incorrectly classified image chips to separate
// subfolders.  If ground truth is not known, CLASSIFY_GENDERS exports
// text file gender.classifications which contains gender assignments
// and classifier scores.  

// ./classify_genders 
// /data/caffe/faces/trained_models/test_96.prototxt                     
// /data/caffe/faces/trained_models/Aug2_184K_T3/train_iter_200000.caffemodel 
// /data/caffe/faces/image_chips/testing/Jul30_and_31_96x96

// ========================================================================
// Last updated on 9/10/16; 9/11/16; 9/15/16; 9/19/16
// ========================================================================

#include "classification/caffe_classifier.h"
#include "general/filefuncs.h"
#include "math/genmatrix.h"
#include "image/imagefuncs.h"
#include "math/prob_distribution.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using namespace caffe;  // NOLINT(build/namespaces)
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

int main(int argc, char** argv) 
{
   string facenet_model_label;
   cout << "Enter facenet model label: (e.g. 2e, 2n, 2r, 2t, 2u)" << endl;
   cin >> facenet_model_label;

   vector<string> blob_names;

// Enumerate blob names as functions of Facenet model label:

   if(facenet_model_label == "1")
   {
      blob_names.push_back("conv1a");
      blob_names.push_back("conv2a");
      blob_names.push_back("conv3a");
      blob_names.push_back("conv4a");
      blob_names.push_back("fc5");
      blob_names.push_back("fc6");
      blob_names.push_back("fc7_faces");
   }
   else if(facenet_model_label == "2e")
   {
      blob_names.push_back("conv1");
      blob_names.push_back("conv2");
      blob_names.push_back("conv3");
      blob_names.push_back("conv4");
      blob_names.push_back("fc5");
      blob_names.push_back("fc6");
      blob_names.push_back("fc7_faces");
   }
   else if (facenet_model_label == "2n" ||
            facenet_model_label == "2q" ||
            facenet_model_label == "2r" ||
            facenet_model_label == "2s" ||
            facenet_model_label == "2t")
   {
      blob_names.push_back("conv1");
      blob_names.push_back("conv2");
      blob_names.push_back("conv3a");
      blob_names.push_back("conv3b");
      blob_names.push_back("conv4a");
      blob_names.push_back("conv4b");
      blob_names.push_back("fc5");
      blob_names.push_back("fc6");
      blob_names.push_back("fc7_faces");
   }
   else if (facenet_model_label == "2u")
   {
      blob_names.push_back("conv1");
      blob_names.push_back("conv2");
      blob_names.push_back("conv3a");
      blob_names.push_back("conv3b");
      blob_names.push_back("conv4a");
      blob_names.push_back("conv4b");
      blob_names.push_back("fc5");
      blob_names.push_back("fc6_faces");
   }
   else
   {
      cout << "Unsupported facenet model label " << endl;
      exit(-1);
   }
   int n_major_layers = blob_names.size() + 1;
   int n_blob_layers = n_major_layers - 1;

   int minor_layer_skip = -1;
   if(facenet_model_label == "1")
   {
      minor_layer_skip = 2;	
   }
   else if(facenet_model_label == "2e" ||
           facenet_model_label == "2n")
   {
      minor_layer_skip = 2;	
   }
   else if(facenet_model_label == "2q" || facenet_model_label == "2r" ||
           facenet_model_label == "2s" || facenet_model_label == "2t" ||
           facenet_model_label == "2u")
   {
      minor_layer_skip = 6;   
   }

//   double male_score_threshold = 0.5;
//   double female_score_threshold = 0.5;
//   cout << "Enter minimal score for male classification:" << endl;
//   cin >> male_score_threshold;
//   cout << "Enter minimal score for female classification:" << endl;
//   cin >> female_score_threshold;

   double incorrect_weight_frac;
//   cout << "Enter incorrect weight fraction (relative to unsure score):" 
//        << endl;
//   cin >> incorrect_weight_frac;
   
   // best male_score_threshold = 0.62;
   // best female_score_threshold = 0.66;
   // frac correct = 0.798; frac unsure = 0.106; frac incorrect = 0.095
   // frac male correct = 0.926; frac female correct = 0.854

   double nonface_score_threshold = 0.5;
   double min_male_score_threshold = 0.5;
//   double max_male_score_threshold = 0.5;
   double max_male_score_threshold = 0.70;
   double min_female_score_threshold = 0.5;
//   double max_female_score_threshold = 0.5;
   double max_female_score_threshold = 0.70;
   double delta_score_threshold = 0.020;

//   min_male_score_threshold = 0.50;
//   max_male_score_threshold = 0.50;
//   min_female_score_threshold = 0.50;
//   max_female_score_threshold = 0.50;

   min_male_score_threshold = 0.52;
   max_male_score_threshold = 0.52;
   min_female_score_threshold = 0.52;
   max_female_score_threshold = 0.52;

//   incorrect_weight_frac = 0.50;
//   incorrect_weight_frac = 0.6;
//   incorrect_weight_frac = 0.65;
   incorrect_weight_frac = 0.6625;
//    incorrect_weight_frac = 0.675;
//   incorrect_weight_frac = 0.7;

   timefunc::initialize_timeofday_clock();

//   bool copy_image_chips_flag = true;
   bool copy_image_chips_flag = false;

   bool truth_known_flag = true;
//   bool truth_known_flag = false;

   string test_prototxt_filename   = argv[1];
   string caffe_model_filename = argv[2];
   string input_images_subdir = argv[3];
   filefunc::add_trailing_dir_slash(input_images_subdir);

   string output_chips_subdir = input_images_subdir+"classified_genders/";
   ofstream output_stream;
   string output_text_filename=output_chips_subdir + "gender.classifications";

   if(!truth_known_flag)
   {
      filefunc::dircreate(output_chips_subdir);
      copy_image_chips_flag = true;
      filefunc::openfile(output_text_filename, output_stream);
      output_stream << "# Index   imagefile_basename   gender   score"
                    << endl << endl;
   }
   
   if (argc != 4) {
      cerr << "Usage: " << argv[0]
           << " test.prototxt network.caffemodel input_images_subdir"
           << endl;
      return 1;
   }

   string classified_chips_subdir="./classified_chips/";
   string correct_chips_subdir=classified_chips_subdir+"correct/";
   string incorrect_chips_subdir=classified_chips_subdir+"incorrect/";
   string unsure_chips_subdir=classified_chips_subdir+"unsure/";

   if(copy_image_chips_flag)
   {
      filefunc::dircreate(classified_chips_subdir);
      filefunc::dircreate(correct_chips_subdir);
      filefunc::dircreate(incorrect_chips_subdir);
      filefunc::dircreate(unsure_chips_subdir);
      filefunc::purge_files_in_subdir(correct_chips_subdir);
      filefunc::purge_files_in_subdir(incorrect_chips_subdir);
      filefunc::purge_files_in_subdir(unsure_chips_subdir);
   }

   caffe_classifier classifier(test_prototxt_filename, caffe_model_filename,
                               n_major_layers, minor_layer_skip);

// Mean RGB values derived from O(386K) 96x96 training face image chips

   double Bmean = 89.0;   // Minimal black padding with cleaned test chips
   double Gmean = 96.6;   // Minimal black padding with cleaned test chips
   double Rmean = 111.6;  // Minimal black padding with cleaned test chips

// Imagenet mean RGB values:

//   double Bmean = 104.008;
//   double Gmean = 116.669;
//   double Rmean = 122.675;

   classifier.set_mean_bgr(Bmean, Gmean, Rmean);

   classifier.add_label("non");
   classifier.add_label("male");
   classifier.add_label("female");
   classifier.add_label("unsure");
   int unsure_label = classifier.get_n_labels() - 1;

   int n_classes = classifier.get_n_labels();
   genmatrix confusion_matrix(n_classes,n_classes);

   bool search_all_children_dirs_flag = false;
//   bool search_all_children_dirs_flag = true;
   vector<string> image_filenames=filefunc::image_files_in_subdir(
      input_images_subdir, search_all_children_dirs_flag);

   int n_images = image_filenames.size();
   cout << "n_images = " << n_images << endl;
   vector<int> shuffled_image_indices = mathfunc::random_sequence(n_images);

   double best_male_score_threshold, best_female_score_threshold;
   double best_frac_correct, best_frac_incorrect, best_frac_unsure;
   double best_frac_nonface_correct, best_frac_male_correct,
      best_frac_female_correct;
   double min_score = 1E10;

   for(double male_score_threshold = min_male_score_threshold;
       male_score_threshold <= max_male_score_threshold;
       male_score_threshold += delta_score_threshold)
   {
      for(double female_score_threshold = min_female_score_threshold;
          female_score_threshold <= max_female_score_threshold;
          female_score_threshold += delta_score_threshold)
      {
         vector<double> correct_nonface_scores, incorrect_nonface_scores;
         vector<double> correct_male_scores, correct_female_scores;
         vector<double> incorrect_male_scores, incorrect_female_scores;
         vector<double> unsure_male_scores, unsure_female_scores;

         confusion_matrix.clear_matrix_values();

         int istart=0;
         int istop = n_images;
         for(int i = istart; i < istop; i++)
         {
            outputfunc::update_progress_and_remaining_time(
               i, 500, (istop-istart));

            int image_ID = shuffled_image_indices[i];
            if(!truth_known_flag) image_ID = i;
      
            string orig_image_filename = image_filenames[image_ID];
            string image_filename = orig_image_filename;
            string image_basename = filefunc::getbasename(image_filename);
//            cout << "i = " << i << " image_basename = " << image_basename 
//                 << endl;

            vector<string> substrings = 
               stringfunc::decompose_string_into_substrings(
                  image_basename, "_.");
            string fullimage_ID_str="";
            if(substrings[1] == "face")//20K female & male faces labeled by PC
            {
               fullimage_ID_str = substrings[2];
            }
//            cout << "fullimage_ID_str = " << fullimage_ID_str << endl;

// Ignore any input 96x96 training image chip which does not
// correspond to an original cropping from an interent image.  In the
// future, we can try experimenting with classification via voting...

            if(substrings[0] != "non")
            {
               int augmentation_ID = stringfunc::string_to_number(
                  substrings[3]);
               if(augmentation_ID > 0) continue;
            }
            
            unsigned int input_img_width, input_img_height;
            imagefunc::get_image_width_height(
               orig_image_filename, input_img_width, input_img_height);

            if(input_img_width != 96 || input_img_height != 96)
            {
               cout << "Error" << endl;
               cout << "input_img_width = " << input_img_width << endl;
               cout << "input_img_height = " << input_img_height << endl;
            }

//      cout << "image_filename = " << image_filename << endl;
            texture_rectangle curr_image(image_filename, NULL);
            classifier.rgb_img_to_bgr_fvec(curr_image);
            classifier.generate_dense_map();

            int classification_label = classifier.get_classification_result();
            double classification_score=classifier.get_classification_score();
            string classified_chip_imagename;

            if(!truth_known_flag)
            {
               string gender="unsure";
               if(classification_score > nonface_score_threshold &&
                  classification_label == 0)
               {
                  gender = classifier.get_label_name(0);
               }
               else if(classification_score > male_score_threshold &&
                  classification_label == 1)
               {
                  gender = classifier.get_label_name(1);
               }
               else if(classification_score > female_score_threshold &&
                       classification_label == 2)
               {
                  gender = classifier.get_label_name(2);
               }

               string classified_chip_basename=
                  stringfunc::prefix(image_basename)+"_"+
                  gender+"."+stringfunc::suffix(image_basename);
               classified_chip_imagename = output_chips_subdir+
                  classified_chip_basename;

               output_stream << i << "   "
                             << image_basename << "   "
                             << gender << "   "
                             << classification_score 
                             << endl;
            }
            else
            {
               int true_label = -1;
               string true_gender_label = substrings[0];
               if(true_gender_label == "non")
               {
                  true_label = 0;
               }
               else if(true_gender_label == "male")
               {
                  true_label = 1;
               }
               else if(true_gender_label == "female")
               {
                  true_label = 2;
               }

               string classification_gender_label;
               if(classification_label == 0)
               {
                  classification_gender_label = "non";
               }
               else if(classification_label == 1)
               {
                  classification_gender_label = "male";
               }
               else if(classification_label == 2)
               {
                  classification_gender_label = "female";
               }

               string correct_chip_basename=true_gender_label+"_";
               if(fullimage_ID_str.size() > 0)
               {
                  correct_chip_basename += fullimage_ID_str+"_";
               }
               correct_chip_basename += 
                  stringfunc::number_to_string(classification_score)+"_"+
                  stringfunc::integer_to_string(i,5)+".jpg";

               string incorrect_chip_basename=true_gender_label+"_"+
                  classification_gender_label+"_";
               if(fullimage_ID_str.size() > 0)
               {
                  incorrect_chip_basename += fullimage_ID_str+"_";
               }
               incorrect_chip_basename += 
                  stringfunc::number_to_string(classification_score)+"_"+
                  stringfunc::integer_to_string(i,5)+".jpg";
               
               if(true_label == 0 &&
                  classification_label == true_label && 
                  classification_score >= nonface_score_threshold)
               {
                  correct_nonface_scores.push_back(classification_score);
                  classified_chip_imagename = correct_chips_subdir+
                     correct_chip_basename;
               }
               else if(true_label == 0 &&
                       classification_label != true_label && 
                       classification_score >= nonface_score_threshold)
               {
                  incorrect_nonface_scores.push_back(classification_score);
                  classified_chip_imagename = incorrect_chips_subdir+
                     incorrect_chip_basename;
               }
               else if(true_label == 1 &&
                  classification_label == true_label && 
                  classification_score >= male_score_threshold)
               {
                  correct_male_scores.push_back(classification_score);
                  classified_chip_imagename = correct_chips_subdir+
                     correct_chip_basename;
               }
               else if (true_label == 1 &&
                        classification_label != true_label &&
                        classification_score >= male_score_threshold)
               {
                  incorrect_male_scores.push_back(classification_score);
                  classified_chip_imagename = incorrect_chips_subdir+
                     incorrect_chip_basename;
               }

               else if(true_label == 1 && 
                       classification_score < male_score_threshold)
               {
                  classification_label = unsure_label;
                  unsure_male_scores.push_back(classification_score);
                  classified_chip_imagename = unsure_chips_subdir+
                     correct_chip_basename;
               }

               else if(true_label == 2 &&
                       classification_label == true_label && 
                       classification_score >= female_score_threshold)
               {
                  correct_female_scores.push_back(classification_score);
                  classified_chip_imagename = correct_chips_subdir+
                     correct_chip_basename;
               }
               else if (true_label == 2 && 
                        classification_label != true_label &&
                        classification_score >= female_score_threshold)
               {
                  incorrect_female_scores.push_back(classification_score);
                  classified_chip_imagename = incorrect_chips_subdir+
                     incorrect_chip_basename;
               }
               else if(true_label == 2 && 
                       classification_score < female_score_threshold)
               {
                  classification_label = unsure_label;
                  unsure_female_scores.push_back(classification_score);
                  classified_chip_imagename = unsure_chips_subdir+
                     correct_chip_basename;
               }

               confusion_matrix.put(
                  true_label, classification_label,
                  confusion_matrix.get(true_label, classification_label) + 1);
            } // truth_known_flag conditional

            if(copy_image_chips_flag)
            {
               string unix_cmd = "cp "+orig_image_filename+" "+
                  classified_chip_imagename;
               sysfunc::unix_command(unix_cmd);
            }

         } // loop over index i labeling input image tiles

         cout << "test.prototxt = " << test_prototxt_filename << endl;
         cout << "trained caffe model = " << caffe_model_filename << endl;
         cout << "input_images_subdir = " << input_images_subdir << endl;

         if(truth_known_flag)
         {
            int n_nonface_correct = correct_nonface_scores.size();
            int n_nonface_incorrect = incorrect_nonface_scores.size();
            int n_male_correct = correct_male_scores.size();
            int n_female_correct = correct_female_scores.size();

            int n_correct = n_nonface_correct + 
               n_male_correct + n_female_correct;
            int n_male_unsure = unsure_male_scores.size();
            int n_female_unsure = unsure_female_scores.size();
            int n_unsure = n_male_unsure + n_female_unsure;
            int n_male_incorrect = incorrect_male_scores.size();
            int n_female_incorrect = incorrect_female_scores.size();
            int n_incorrect = n_nonface_incorrect + n_male_incorrect + 
               n_female_incorrect;

            int n_nonface = n_nonface_correct + n_nonface_incorrect;
            int n_male = n_male_correct + n_male_unsure + n_male_incorrect;
            int n_female = n_female_correct + n_female_unsure + 
               n_female_incorrect;

            cout << "n_nonface = " << n_nonface
                 << " n_male = " << n_male 
                 << " n_female = " << n_female 
                 << endl << endl;

            cout << "n_correct = " << n_correct << endl;
            cout << " n_nonface_correct = " << n_nonface_correct
                 << " n_male_correct = " << n_male_correct
                 << " n_female_correct = " << n_female_correct << endl
                 << endl << endl;

            cout << "n_incorrect = " << n_incorrect << endl;
            cout << " n_nonface_incorrect = " << n_nonface_incorrect
                 << " n_male_incorrect = " << n_male_incorrect
                 << " n_female_incorrect = " << n_female_incorrect 
                 << endl << endl;

            cout << " n_unsure = " << n_unsure << endl;
            cout << " n_male_unsure = " << n_male_unsure 
                 << " n_female_unsure = " << n_female_unsure 
                 << endl << endl;

            double frac_correct = 
               double(n_correct)/(n_correct+n_incorrect+n_unsure);
            double frac_unsure = 
               double(n_unsure)/(n_correct+n_incorrect+n_unsure);
            double frac_incorrect = 
               double(n_incorrect)/(n_correct+n_incorrect+n_unsure);


            double frac_nonface_correct = double(n_nonface_correct) / 
               double(n_nonface);
            double frac_male_correct = double(n_male_correct) / 
               double (n_male);
            double frac_female_correct = double(n_female_correct) / 
               double (n_female);

            double frac_male_unsure = double(n_male_unsure) / 
               double (n_male);
            double frac_female_unsure = double(n_female_unsure) / 
               double (n_female);

            double frac_nonface_incorrect = double(n_nonface_incorrect) / 
               double(n_nonface);
            double frac_male_incorrect = double(n_male_incorrect) / 
               double (n_male);
            double frac_female_incorrect = double(n_female_incorrect) / 
               double (n_female);

            cout << "frac_correct = " << frac_correct
                 << " frac_unsure = " << frac_unsure 
                 << " frac_incorrect = " << frac_incorrect
                 << endl;
            cout << endl;

            cout << "frac_nonface_correct = " << frac_nonface_correct << endl;
            cout << "frac_nonface_incorrect = " << frac_nonface_incorrect
                 << endl;
            cout << endl;

            cout << "frac_male_correct = " << frac_male_correct << endl;
            cout << "frac_male_unsure = " << frac_male_unsure << endl;
            cout << "frac_male_incorrect = " << frac_male_incorrect << endl;
            cout << endl;

            cout << "frac_female_correct = " << frac_female_correct << endl;
            cout << "frac_female_unsure = " << frac_female_unsure << endl;
            cout << "frac_female_incorrect = " << frac_female_incorrect 
                 << endl;
            cout << endl;

            cout << "Confusion matrix:" << endl;
            cout << confusion_matrix << endl << endl;

            cout << "male_score_threshold = " << male_score_threshold
                 << " female_score_threshold = " << female_score_threshold
                 << endl;
/*
  prob_distribution prob_male_correct(correct_male_scores, 100, 0);
  prob_distribution prob_female_correct(correct_female_scores, 100, 0);
  prob_distribution prob_male_incorrect(incorrect_male_scores, 100, 0);
  prob_distribution prob_female_incorrect(incorrect_female_scores, 100, 0);

  prob_male_correct.set_title(
  "Classification scores for correctly classified males");
  prob_female_correct.set_title(
  "Classification scores for correctly classified females");
  prob_male_incorrect.set_title(
  "Classification scores for incorrectly classified males");
  prob_female_incorrect.set_title(
  "Classification scores for incorrectly classified females");

  prob_male_correct.set_xtic(0.2);
  prob_male_correct.set_xsubtic(0.1);
  prob_male_correct.set_xlabel("Classification score");

  prob_female_correct.set_xtic(0.2);
  prob_female_correct.set_xsubtic(0.1);
  prob_female_correct.set_xlabel("Classification score");

  prob_male_incorrect.set_xtic(0.2);
  prob_male_incorrect.set_xsubtic(0.1);
  prob_male_incorrect.set_xlabel("Classification score");      

  prob_female_incorrect.set_xtic(0.2);
  prob_female_incorrect.set_xsubtic(0.1);
  prob_female_incorrect.set_xlabel("Classification score");      

  prob_male_correct.set_densityfilenamestr("correct_male_dens.meta");
  prob_female_correct.set_densityfilenamestr("correct_female_dens.meta");
  prob_male_incorrect.set_densityfilenamestr("incorrect_male_dens.meta");
  prob_female_incorrect.set_densityfilenamestr(
  "incorrect_female_dens.meta");

  prob_male_correct.set_cumulativefilenamestr("correct_male_cum.meta");
  prob_female_correct.set_cumulativefilenamestr("correct_female_cum.meta");
  prob_male_incorrect.set_cumulativefilenamestr("incorrect_male_cum.meta");
  prob_female_incorrect.set_cumulativefilenamestr(
  "incorrect_female_cum.meta");

  prob_male_correct.writeprobdists(false);
  prob_female_correct.writeprobdists(false);
  prob_male_incorrect.writeprobdists(false);
  prob_female_incorrect.writeprobdists(false);
*/

            if(copy_image_chips_flag)
            {
               string banner="Copied correctly classified chips to "+
                  correct_chips_subdir;
               outputfunc::write_banner(banner);
               banner="Copied incorrectly classified chips to "+
                  incorrect_chips_subdir;
               outputfunc::write_banner(banner);
               banner="Copied unsure chips to "+ unsure_chips_subdir;
               outputfunc::write_banner(banner);
            }

            double curr_score = 
               (1 - incorrect_weight_frac) * 
               (frac_male_unsure + frac_female_unsure)
               + incorrect_weight_frac * 
               (frac_male_incorrect + frac_female_incorrect);
            if(curr_score < min_score)
            {
               min_score = curr_score;
               best_male_score_threshold = male_score_threshold;
               best_female_score_threshold = female_score_threshold;

               best_frac_nonface_correct = frac_nonface_correct;
               best_frac_male_correct = frac_male_correct;
               best_frac_female_correct = frac_female_correct;
               best_frac_correct = frac_correct;
               best_frac_unsure = frac_unsure;
               best_frac_incorrect = frac_incorrect;
            }

            cout << "min_score = " << min_score << endl;
            cout << "incorrect_weight_frac = " << incorrect_weight_frac 
                 << endl;
            cout << "best_male_score_threshold = " 
                 << best_male_score_threshold
                 << " best_female_score_threshold = " 
                 << best_female_score_threshold << endl;
            cout << "best_frac_correct = " << best_frac_correct
                 << " best_frac_unsure = " << best_frac_unsure 
                 << " best_frac_incorrect = " << best_frac_incorrect
                 << endl;

            cout << "best_frac_nonface_correct = " << best_frac_nonface_correct
                 << " best_frac_male_correct = " << best_frac_male_correct
                 << " best_frac_female_correct = " << best_frac_female_correct
                 << endl;
         }
         else
         {
            filefunc::closefile(output_text_filename, output_stream);
            string banner="Exported gender classifications to "+
               output_text_filename;
            outputfunc::write_banner(banner);

            banner="Exported classified chips to " + output_chips_subdir;
            outputfunc::write_banner(banner);
         } // truth_known_flag conditional
         
         
      } // loop over female_score_threshold
   } // loop over male_score_threshold
}
   
