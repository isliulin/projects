// ====================================================================
// Program BBOX_WIDTHS imports the XML file generated by
// Davis King's IMGLAB tool.  It generates density and cumulative
// distributions for the labeled image's bbox widths and aspect
// ratios.  It also reports the 10%, 20%, ..., 90% percentile pixel
// widths from the cumulative width distribution.

//	           ./bbox_widths all_labeled_images.xml

// ====================================================================
// Last updated on 5/11/16; 6/5/16; 6/6/16
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
   dlib::array<dlib::array2d<unsigned char> > images;
   vector<vector<dlib::rectangle> > object_locations;

   parser.parse(argc, argv);
   if (parser.number_of_arguments() != 1)
   {
      cout << "Specify XML file with labeled imagery data!" << endl;
      return 1;
   }

   dlib::image_dataset_file img_dataset_file(parser[0]);
   dlib::image_dataset_metadata::dataset labeled_data;
   dlib::image_dataset_metadata::load_image_dataset_metadata(
      labeled_data, img_dataset_file.get_filename());
   int n_images = labeled_data.images.size();


// Compute probability distributions for face bbox pixel widths and
// aspect ratios:

   double min_AR = POSITIVEINFINITY;
   double max_AR = NEGATIVEINFINITY;
   
   vector<double> widths, aspect_ratios;
   int n_object_boxes = 0;
   for(int i = 0; i < n_images; i++)
   {
      vector<dlib::image_dataset_metadata::box> boxes = 
         labeled_data.images[i].boxes;
      for(unsigned int b = 0; b < boxes.size(); b++)
      {
         dlib::image_dataset_metadata::box curr_box = boxes[b];

// FAKE FAKE:  Sun Jun 5 at 6:14 pm

         if(curr_box.label != "face") continue;

         dlib::rectangle curr_rect = curr_box.rect;
         unsigned int px_min = curr_rect.left();
         unsigned int px_max = curr_rect.right();
         unsigned int py_min = curr_rect.top();
         unsigned int py_max = curr_rect.bottom();
         unsigned int bbox_width = abs(px_max - px_min);
         unsigned int bbox_height = abs(py_max - py_min);

         widths.push_back(bbox_width);
         if(bbox_height > 1)
         {
            aspect_ratios.push_back(double(bbox_width)/double(bbox_height));
         }
         
         min_AR=basic_math::min(aspect_ratios.back(), min_AR);
         max_AR=basic_math::max(aspect_ratios.back(), max_AR);
         
         n_object_boxes++;
      } // loop over index b labeling bboxes
   } // loop over index i labeling labeled images
   

   bool gzip_flag = false;

   prob_distribution prob_aspect_ratios(100, 0, 2, aspect_ratios);
   prob_aspect_ratios.set_title(
      "Labeled face bbox aspect ratio density distribution");
   prob_aspect_ratios.set_xlabel("Labeled face bbox aspect ratio");
   prob_aspect_ratios.set_xmax(2);
   prob_aspect_ratios.set_xtic(0.2);
   prob_aspect_ratios.set_xsubtic(0.1);
   prob_aspect_ratios.set_densityfilenamestr("face_bbox_ars_dens.meta");
   prob_aspect_ratios.set_cumulativefilenamestr("face_bbox_ars_cum.meta");
   prob_aspect_ratios.writeprobdists(gzip_flag);

   prob_distribution prob_widths(100, 0, 200, widths);
   prob_widths.set_title("Labeled face bbox width density distribution");
   prob_widths.set_xlabel("Labeled face bbox width (pixels)");
   prob_widths.set_xmax(200);
   prob_widths.set_xtic(50);
   prob_widths.set_xsubtic(25);
   prob_widths.set_densityfilenamestr("face_bbox_widths_dens.meta");
   prob_widths.set_cumulativefilenamestr("face_bbox_widths_cum.meta");
   prob_widths.writeprobdists(gzip_flag);

   double width_10 = prob_widths.find_x_corresponding_to_pcum(0.10);
   double width_20 = prob_widths.find_x_corresponding_to_pcum(0.20);
   double width_25 = prob_widths.find_x_corresponding_to_pcum(0.25);
   double width_30 = prob_widths.find_x_corresponding_to_pcum(0.30);
   double width_40 = prob_widths.find_x_corresponding_to_pcum(0.40);
   double width_50 = prob_widths.find_x_corresponding_to_pcum(0.50);
   double width_60 = prob_widths.find_x_corresponding_to_pcum(0.60);
   double width_70 = prob_widths.find_x_corresponding_to_pcum(0.70);
   double width_75 = prob_widths.find_x_corresponding_to_pcum(0.75);
   double width_80 = prob_widths.find_x_corresponding_to_pcum(0.80);
   double width_90 = prob_widths.find_x_corresponding_to_pcum(0.90);

   cout << "width_10 = " << width_10 << endl;
   cout << "width_20 = " << width_20 << endl;
   cout << "width_25 = " << width_25 << endl;
   cout << "width_30 = " << width_30 << endl;
   cout << "width_40 = " << width_40 << endl;
   cout << "width_50 = " << width_50 << endl;
   cout << "width_60 = " << width_60 << endl;
   cout << "width_70 = " << width_70 << endl;
   cout << "width_75 = " << width_75 << endl;
   cout << "width_80 = " << width_80 << endl;
   cout << "width_90 = " << width_90 << endl;

   cout << "n_images = " << n_images << endl;
   cout << "Number of object bboxes = " << n_object_boxes << endl;
   cout << "widths.size() = " << widths.size() << endl;
   cout << "min_AR = " << min_AR << " max_AR = " << max_AR << endl;

/*

Pixel widths for 34463 face bboxes from 11032 images as of 6/15/16:

width_10 = 7.50549
width_20 = 12.4004
width_25 = 15.0534
width_30 = 17.9286
width_40 = 24.1178
width_50 = 30.4361
width_60 = 38.1931
width_70 = 49.8943
width_75 = 57.3194
width_80 = 67.2405
width_90 = 99.5993



Pixel widths for 26081 face bboxes from 8676 images as of 6/5/16:

width_10 = 6.35129
width_20 = 10.0484
width_25 = 11.9648
width_30 = 13.943
width_40 = 18.5702
width_50 = 24.4765
width_60 = 31.9141
width_70 = 42.9407
width_75 = 50.6891
width_80 = 60.9524
width_90 = 92.8283

*/

}

