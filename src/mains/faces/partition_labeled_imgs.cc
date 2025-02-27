// ====================================================================
// Program PARTITION_LABELED_IMGS imports the XML file generated by
// Davis King's IMGLAB tool.  It splits this file's entire set of
// labeled images into training, validation and testing subsets.  New
// training, validation and testing XML files are exported to
// subdirectories containing soft links to the training, validation
// and testing images.  

//           partition_labeled_imgs all_labeled_images.xml

// ====================================================================
// Last updated on 5/11/16; 6/7/16; 6/18/16; 7/20/16
// ====================================================================

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <dlib/array.h>
#include <dlib/array2d.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/data_io.h>

#include "general/filefuncs.h"
#include "math/mathfuncs.h"
#include "general/outputfuncs.h"
#include "math/prob_distribution.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "video/videofuncs.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::exception;
using std::flush;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

int main(int argc, char** argv)
{  
   timefunc::initialize_timeofday_clock(); 

   dlib::command_line_parser parser;
   parser.parse(argc, argv);
   if (parser.number_of_arguments() != 1)
   {
      cout << "Must specify XML file containing labeled imagery metadata!" 
           << endl;
      return 1;
   }

   string faces_rootdir = "/data/TrainingImagery/faces/";
   string labeled_faces_subdir = faces_rootdir + "images/";
   string labeled_data_subdir=faces_rootdir+"labeled_data/";   
   filefunc::dircreate(labeled_data_subdir);
   
   int faces_ID = -1;
   cout << "Enter faces ID (-1 for default):" << endl;
   cin >> faces_ID;
   string faces_subdir=labeled_data_subdir+"faces";
   if(faces_ID >= 0)
   {
      faces_subdir += "_"+stringfunc::integer_to_string(faces_ID,2);
   }
   filefunc::add_trailing_dir_slash(faces_subdir);
   filefunc::dircreate(faces_subdir);

   string training_images_subdir=faces_subdir+"training_images/";
   string validation_images_subdir=faces_subdir+"validation_images/";
   string testing_images_subdir=faces_subdir+"testing_images/";
   filefunc::dircreate(training_images_subdir);
   filefunc::dircreate(validation_images_subdir);
   filefunc::dircreate(testing_images_subdir);


   dlib::array<dlib::array2d<unsigned char> > images;
   vector<vector<dlib::rectangle> > object_locations;
   dlib::image_dataset_file img_dataset_file(parser[0]);
   dlib::image_dataset_metadata::dataset labeled_data;
   dlib::image_dataset_metadata::load_image_dataset_metadata(
      labeled_data, img_dataset_file.get_filename());

// Randomize entire set of input labeled images.  Then partition
// labeled images into training, validation and testing sets.  Limit
// number of validation images to relatively small number.  Take
// n_testing > 500:

   int n_images = labeled_data.images.size();
   unsigned int istart = 0;
   unsigned istop = n_images-1;
   cout << "n_labeled_images =  " << n_images << endl;

   vector<int> random_seq = mathfunc::random_sequence(
      istart, istop, istop - istart + 1);

   int n_testing = 1200;
   int n_validation = 600;
   int n_training = n_images - (n_validation + n_testing);
   n_training = n_images - (n_validation + n_testing);

   cout << "n_training = " << n_training << endl;
   cout << "n_validation = " << n_validation << endl;
   cout << "n_testing = " << n_testing << endl;

   dlib::image_dataset_metadata::dataset training_data;
   dlib::image_dataset_metadata::dataset validation_data;
   dlib::image_dataset_metadata::dataset testing_data;

   training_data.name = "training_data";
   validation_data.name = "validation_data";
   testing_data.name = "testing_data";

// Testing images subset:

   for(int i = 0; i < n_testing; i++)
   {
      int curr_image_ID = random_seq[i];
      string currimage_filename = labeled_faces_subdir + 
         labeled_data.images[curr_image_ID].filename;
      string unix_cmd="ln -s "+currimage_filename+" "+testing_images_subdir;
      sysfunc::unix_command(unix_cmd);
      testing_data.images.push_back(labeled_data.images[curr_image_ID]);
   }
   string testing_xml_filename=testing_images_subdir+"testing_images.xml";
   dlib::image_dataset_metadata::save_image_dataset_metadata(
      testing_data, testing_xml_filename);

// Validation images subset:

   for(int i = n_testing; i < n_testing + n_validation; i++)
   {
      int curr_image_ID = random_seq[i];
      string currimage_filename = labeled_faces_subdir + 
         labeled_data.images[curr_image_ID].filename;
      string unix_cmd="ln -s "+currimage_filename+" "+validation_images_subdir;
      sysfunc::unix_command(unix_cmd);
      validation_data.images.push_back(labeled_data.images[curr_image_ID]);
   }
   string validation_xml_filename=validation_images_subdir+
      "validation_images.xml";
   dlib::image_dataset_metadata::save_image_dataset_metadata(
      validation_data, validation_xml_filename);

// Training images subset:

   for(int i = n_testing + n_validation; i < n_images; i++)
   {
      int curr_image_ID = random_seq[i];
      string currimage_filename = labeled_faces_subdir + 
         labeled_data.images[curr_image_ID].filename;
      string unix_cmd="ln -s "+currimage_filename+" "+training_images_subdir;
      sysfunc::unix_command(unix_cmd);
      training_data.images.push_back(labeled_data.images[curr_image_ID]);
   }
   string training_xml_filename=training_images_subdir+"training_images.xml";
   dlib::image_dataset_metadata::save_image_dataset_metadata(
      training_data, training_xml_filename);

   string banner="Exported training, validation and testing image sets to "+
      faces_subdir;
   outputfunc::write_banner(banner);
}

