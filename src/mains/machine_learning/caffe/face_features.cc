// ========================================================================
// Program FACE_FEATURES imports a trained FACENET caffe model.  It
// also takes in some set of training, validation and/or testing
// images.  It queries the user to enter the name of the trained
// network layer whose activations will be used as feature descriptors
// for the input images.  Looping over each input image, FACE_FEATURES
// checks if pixel width and height equal 96.  If not, it crops out a
// 96x96 subimage from the input image's center.  It then retrieves
// the activation values from the specified network layer for the
// 96x96 image chip.  

// The activation values along with the full pathname for the original
// chip are written to an output text file.  We also export an index
// file containing feature file basenames vs corresponding test image
// file basenames.

// ./face_features                                          
// /data/caffe/faces/trained_models/Sep15_2t_T1/test.prototxt           
// /data/caffe/faces/trained_models/Sep15_2t_T1/train_iter_172000.caffemodel
// /data/caffe/faces/image_chips/training/Sep10_106x106/non_augmented/

// ========================================================================
// Last updated on 9/17/16; 9/20/16; 9/21/16
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
using std::map;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

// ------------------------------------------------------------------
// First 6170 feature descriptors correspond to testing and validation
// face image chips

int main(int argc, char** argv) 
{
//   bool copy_files_to_image_engine_subdir = true;
   bool copy_files_to_image_engine_subdir = false;
   
   string facenet_model_label = "2t";
   timefunc::initialize_timeofday_clock();

   string test_prototxt_filename   = argv[1];
   string caffe_model_filename = argv[2];
   string input_images_subdir = argv[3];
   filefunc::add_trailing_dir_slash(input_images_subdir);

   string network_subdir = "./vis_facenet/network/";
   string base_activations_subdir = network_subdir + "activations/";
   string activations_subdir = base_activations_subdir + "model_"+
      facenet_model_label+"/";
   string features_subdir = activations_subdir + "features/";
   filefunc::dircreate(features_subdir);
   string image_engine_subdir = "/data/ImageEngine/faces/";

// Enumerate blob names as functions of Facenet model label:

   vector<string> blob_names;

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
   else
   {
      cout << "Unsupported facenet model label " << endl;
      exit(-1);
   }
   int n_major_layers = blob_names.size() + 1;

   int minor_layer_skip = -1;
   if(facenet_model_label == "1")
   {
      minor_layer_skip = 2;	
   }
   else if(facenet_model_label == "2e")
   {
      minor_layer_skip = 2;	
   }
   else if(facenet_model_label == "2n")
   {
      minor_layer_skip = 2;   
   }
   else if(facenet_model_label == "2q" || facenet_model_label == "2r" ||
           facenet_model_label == "2s" || facenet_model_label == "2t")
   {
      minor_layer_skip = 6;   
   }

   caffe_classifier classifier(test_prototxt_filename, caffe_model_filename,
                               n_major_layers, minor_layer_skip);

// Mean RGB values derived from O(386K) 96x96 training face image chips

   double Bmean = 89.0;   // Minimal black padding with cleaned test chips
   double Gmean = 96.6;   // Minimal black padding with cleaned test chips
   double Rmean = 111.6;  // Minimal black padding with cleaned test chips
   classifier.set_mean_bgr(Bmean, Gmean, Rmean);

   classifier.add_label("non");
   classifier.add_label("male");
   classifier.add_label("female");
   classifier.add_label("unsure");

   string feature_layer_name = "fc5";
   cout << "Enter name of major layer (e.g. fc5, fc6) " << endl;
   cout << "  for which image features should be exported:" << endl;
//   cin >> feature_layer_name;
   string image_features_subdir = features_subdir + feature_layer_name;
   filefunc::add_trailing_dir_slash(image_features_subdir);
   filefunc::dircreate(image_features_subdir);

   bool search_all_children_dirs_flag = false;
   vector<string> image_filenames=filefunc::image_files_in_subdir(
      input_images_subdir, search_all_children_dirs_flag);
   int n_images = image_filenames.size();
   vector<int> shuffled_image_indices = mathfunc::random_sequence(n_images);
   cout << "Total number of input images = " << n_images << endl;

   int istart = 0;
   int n_descriptors = n_images;
   int istop = n_descriptors;

// Assemble feature descriptors into rows of matrix M:

   int d_dims = 256;  // dimension of fc6, fc5 layer
   genmatrix M(n_descriptors, d_dims);  

   for(int i = istart; i < istop; i++)
   {
      outputfunc::update_progress_and_remaining_time(
         i-istart, 1000, (istop-istart));
      int image_ID = shuffled_image_indices[i-istart];
      string image_filename = image_filenames[image_ID];
      string image_basename = filefunc::getbasename(image_filename);

      if(copy_files_to_image_engine_subdir)
      {
         string unix_cmd = "cp "+image_filename+" "+
            image_engine_subdir;
         cout << unix_cmd << endl;
         sysfunc::unix_command(unix_cmd);
         continue;
      }

      vector<string> substrings = 
         stringfunc::decompose_string_into_substrings(image_basename,"_");
      string true_gender_label = substrings[0];

      texture_rectangle curr_image(image_filename, NULL);

      int xdim = curr_image.getWidth();
      int ydim = curr_image.getHeight();
      if(xdim > 96 || ydim > 96)
      {
         unsigned int px_start = (xdim - 96) / 2;
         unsigned int px_stop = px_start + 95;
         unsigned int py_start = (ydim - 96) / 2;
         unsigned int py_stop = py_start + 95;

         string reduced_image_filename = "./reduced_imagechip.jpg";
         curr_image.write_curr_frame(
            px_start, px_stop, py_start, py_stop,
            reduced_image_filename);
         texture_rectangle reduced_image(reduced_image_filename, NULL);
         classifier.rgb_img_to_bgr_fvec(reduced_image);
      }
      else
      {
         classifier.rgb_img_to_bgr_fvec(curr_image);
      }
      classifier.generate_dense_map();

// Retrieve node activations for specified network layer:

      vector<int> local_node_IDs;
      vector<double> node_activations;

      bool sort_activations_flag = false;
      classifier.retrieve_layer_activations(
         feature_layer_name, local_node_IDs, node_activations,
         sort_activations_flag);

      int n_layer_nodes = node_activations.size();
      for(int d = 0; d < n_layer_nodes; d++)
      {
         M.put(i, d, node_activations[d]);
      } // loop over index n labeling nodes within current layer
   } // loop over index i labeling input test images

// Compute SVD of data matrix M:

   genmatrix Usorted(n_descriptors, d_dims);
   genmatrix Wsorted(d_dims, d_dims);
   genmatrix Vsorted(d_dims, d_dims);
   
   if(!M.sorted_singular_value_decomposition(Usorted, Wsorted, Vsorted))
   {
      cout << "SVD of data matrix M failed" << endl;
      exit(-1);
   }

   double max_eigenvalue = 1, eigenvalue_ratio;
   for(int d = 0; d < d_dims; d++)
   {
      double singular_value = Wsorted.get(d,d);
      double eigenvalue = sqr(singular_value);
      if(d == 0)
      {
         max_eigenvalue = eigenvalue;
         eigenvalue_ratio = 1.0;
      }
      else
      {
         eigenvalue_ratio = eigenvalue / max_eigenvalue;
      }
      
      cout << "d = " << d << " singular value = " << singular_value
           << " eigenvalue = " << eigenvalue 
           << " eigenvalue/max_eigenvalue = " << eigenvalue_ratio
           << endl;
   }
   
// For fc5 layer of model 2t, eigenvalue/max_eigenvalue < 1E-3 for d >= 86
// For fc6 layer of model 2t, eigenvalue/max_eigenvalue < 1E-4 for d >= 24

   int d_max = 86; // fc5 layer
//   int d_max = 24; // fc6 layer

   genvector curr_column(n_descriptors);
   genmatrix Ureduced(n_descriptors, d_max);
   genmatrix Wreduced(d_max, d_max);
   Wreduced.clear_matrix_values();
   for(int c = 0; c < d_max; c++)
   {
      Usorted.get_column(c, curr_column);
      Ureduced.put_column(c, curr_column);
      Wreduced.put(c,c,Wsorted.get(c,c));
   }

   genmatrix Mreduced(n_descriptors, d_max);
   Mreduced = Ureduced * Wreduced;

   ofstream lookup_stream;
   string lookup_filename = image_features_subdir+"features_vs_images.dat";
   filefunc::openfile(lookup_filename, lookup_stream);

   typedef std::map<string,  string > FACE_FEATURES_MAP;
// independent string contains basename for test image chip
// dependent string contains filename for feature descriptor
   FACE_FEATURES_MAP face_features_map;

   for(int i = istart; i < istop; i++)
   {
      outputfunc::update_progress_and_remaining_time(
         i-istart, 1000, (istop-istart));

      int image_ID = shuffled_image_indices[i-istart];
      string image_filename = image_filenames[image_ID];
      string image_basename = filefunc::getbasename(image_filename);
      vector<string> substrings = 
         stringfunc::decompose_string_into_substrings(image_basename,"_");
      string true_gender_label = substrings[0];

// Open text file for current image into which we save reduced feature
// descriptor:

      string currimage_features_filename=image_features_subdir+
         feature_layer_name+"_"+stringfunc::integer_to_string(i,6)+".dat";

      lookup_stream << filefunc::getbasename(currimage_features_filename)
                    << "  "
                    << filefunc::getbasename(image_filename) << endl;

      face_features_map[filefunc::getbasename(image_filename)] = 
         currimage_features_filename;

/*
      ofstream image_features_stream;
      filefunc::openfile(
         currimage_features_filename, image_features_stream);
      image_features_stream << image_filename << endl;
      image_features_stream << true_gender_label << endl;
      image_features_stream << endl;

      for(int d = 0; d < d_max; d++)
      {
         image_features_stream << Mreduced.get(i,d) << endl;
      } 

      filefunc::closefile(
         currimage_features_filename, image_features_stream);
*/

      if(i-istart%100 == 0)
      {
         cout << "i-istart = " << i-istart
              << " image_filename = " << image_filename
              << endl;
         string banner="Exported "+currimage_features_filename;
         outputfunc::write_banner(banner);
      }
   } // loop over index i labeling test images

   filefunc::closefile(lookup_filename, lookup_stream);
   string banner="Exported feature file basenames vs test image basenames to "+
      lookup_filename;
   outputfunc::write_banner(banner);
}

