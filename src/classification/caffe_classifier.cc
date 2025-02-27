// ==========================================================================
// caffe_classifier class member function definitions
// ==========================================================================
// Last modified on 9/6/16; 9/12/16; 9/13/16; 1/18/17
// ==========================================================================

#include <caffe/net.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <map>
#include <memory>

#include "classification/caffe_classifier.h"
#include "image/graphicsfuncs.h"
#include "general/filefuncs.h"
#include "general/stringfuncs.h"
#include "video/texture_rectangle.h"

using namespace caffe; // NOLINT(build/namespaces)

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ifstream;
using std::ostream;
using std::map;
using std::pair;
using std::string;
using std::vector;

// ---------------------------------------------------------------------
// Initialization, constructor and destructor functions:
// ---------------------------------------------------------------------

void caffe_classifier::allocate_member_objects()
{
}		       

void caffe_classifier::initialize_member_objects()
{
   feature_descriptor = NULL;
   label_tr_ptr = NULL;
   score_tr_ptr = NULL;
   cc_tr_ptr = NULL;

// By default, we assume caffe_classifier object is used for discrete
// classification rather than semantic segmentation:

   segmentation_flag = false; 
}		       

// ---------------------------------------------------------------------
caffe_classifier::caffe_classifier(
   const string& deploy_prototxt_filename,
   const string& trained_caffe_model_filename)
{
   allocate_member_objects();
   initialize_member_objects();

   test_prototxt_filename = deploy_prototxt_filename;
   this->trained_caffe_model_filename = trained_caffe_model_filename;
   load_trained_network();
}
// ---------------------------------------------------------------------
caffe_classifier::caffe_classifier(
   const string& deploy_prototxt_filename,
   const string& trained_caffe_model_filename,
   int n_major_layers, int minor_layer_skip)
{
   allocate_member_objects();
   initialize_member_objects();

   test_prototxt_filename = deploy_prototxt_filename;
   this->trained_caffe_model_filename = trained_caffe_model_filename;
   this->n_major_layers = n_major_layers;
   this->minor_layer_skip = minor_layer_skip;
   load_trained_network();
}

// ---------------------------------------------------------------------
caffe_classifier::~caffe_classifier()
{
   delete [] feature_descriptor;
   delete label_tr_ptr;
   delete score_tr_ptr;
   delete cc_tr_ptr;
}

// ---------------------------------------------------------------------
// Overload << operator:

/*
  ostream& operator<< (ostream& outstream,const caffe_classifier& training)
  {
  outstream << endl;
  outstream << "Example " << training.get_ID() << endl;

  for(unsigned int f = 0; f < training.feature_values.size(); f++)
  {
  outstream << "  Feature " << f << " : " 
  << training.feature_labels[f] << " = "
  << training.feature_values[f] << endl;
  }
   
  outstream << "  Classification value = " 
  << training.classification_value << endl;
  return outstream;
  }
*/

// ==========================================================================
// Member function load_trained_network() imports a test.prototxt file
// as well as a trained caffe model.  

void caffe_classifier::load_trained_network()
{
   cout << "Loading trained network:" << endl;

#ifdef CPU_ONLY
   Caffe::set_mode(Caffe::CPU);
#else
   Caffe::set_mode(Caffe::GPU);
#endif

// Load the network and print out its metadata:

   net_.reset(new Net<float>(test_prototxt_filename, caffe::TEST));
   net_->CopyTrainedLayersFrom(trained_caffe_model_filename);
   print_network_metadata();   

   Blob<float>* input_blob = net_->input_blobs()[0];
   num_data_channels_ = input_blob->channels();
   CHECK(num_data_channels_ == 3 || num_data_channels_ == 1)
      << "Input layer should have 1 or 3 color channels.";

   int datawidth_ = input_blob->width();
   int dataheight_ = input_blob->height();
   input_geometry_ = cv::Size(datawidth_, dataheight_);
}

// ---------------------------------------------------------------------
// Member function print_network_metadata() prints out metadata for the
// trained network's layers, blobs, etc.

void caffe_classifier::print_network_metadata()
{
   cout << "..........................................." << endl;
   cout << "Network metadata" << endl;
   
   cout << "Trained network name = " << net_->name() << endl;
   cout << "Trained caffe model filename = "
        << trained_caffe_model_filename << endl;
   cout << "Test prototxt filename = " << test_prototxt_filename << endl;

   n_minor_layers = net_->layer_names().size();
   cout << "n_minor_layers = " << n_minor_layers << endl;
   int n_blobs = net_->blob_names().size();
   cout << "n_blobs = " << n_blobs << endl;

//   int n_input_blobs = net_->input_blobs().size();
//   cout << "n_input_blobs = " << n_input_blobs << endl;
   CHECK_EQ(net_->num_inputs(), 1) 
      << "Network should have exactly one input blob.";

//   int n_output_blobs = net_->output_blobs().size();
//   cout << "n_output_blobs = " << n_output_blobs << endl;
   CHECK_EQ(net_->num_outputs(), 1) 
      << "Network should have exactly one output blob.";

   // On 8/1/16, we empirically confirmed that caffe::TRAIN = 0 and
   // caffe::TEST = 1.

   int network_phase = net_->phase();
   if(network_phase == caffe::TRAIN)
   {
      cout << "network_phase = caffe::TRAIN" << endl;
   }
   else if (network_phase == caffe::TEST)
   {
      cout << "network_phase = caffe::TEST" << endl;
   }
   
   for(int l = 0; l < n_minor_layers; l++)
   {
      cout << "Minor layer l = " << l 
           << " layer_name = " << net_->layer_names().at(l)
           << endl;
//      cout << "param_layer_names.push_back(\""
//           << net_->layer_names().at(l) << "\");" 
//           << endl;
   }
   cout << endl;

// Q: How are parameter blobs associated with layers?  How do we get a
// "name" for a parameter blob (e.g. conv1a, conv2a, etc)?  Following
// call only seems to return bool values which precisely oscillate
// between true and false and don't obviously correlate with conv/FC
// layers in VGG/Face01 networks...

//   vector<string> param_names = net_->param_display_names();
//   for(unsigned int p = 0; p < param_names.size(); p++)
//   {
//      cout << "p = " << p << " param_names[p] = " << param_names[p]
//           << endl;
//   }

   for(int b = 0; b < n_blobs; b++)
   {
      cout << "Blob b = " << b
           << " blob_name = " << net_->blob_names().at(b)
           << " : " << flush;
      cout << net_->blobs().at(b)->shape_string() << endl;
      
/*
      int num_axes = net_->blobs().at(b)->num_axes();
      for(int a = 0; a < num_axes; a++)
      {
         int curr_shape = net_->blobs().at(b)->shape(a);
         cout << curr_shape << " " << flush;
         if(a < num_axes - 1) cout << "x " << flush;
      }
      cout << endl;
*/
   }
   cout << endl;

// Extract total number of weights and biases contained within
// network from all parameter blobs:

   n_param_layers = net_->params().size();
   cout << "Number of layers containing calculated parameters = " 
        << n_param_layers << endl;

   int n_total_params = 0;
   for(int p = 0; p < n_param_layers; p++)
   {
      const shared_ptr<const caffe::Blob<float> > param_blob =
         net_->params().at(p);
      if(param_blob->num_axes() > 1)
      {
         n_total_params += param_blob->count();
      }
      cout << "p = " << p 
           << " param_blob.num_axes() = " << param_blob->num_axes()
           << " param_blob.shape_string() = " << param_blob->shape_string()
           << endl;
   }

   cout << endl;
   cout << "--------------------------------------------" << endl;
   cout << "n_total_params = " << n_total_params
        << endl;
   cout << "--------------------------------------------" << endl;

   int global_weight_node_counter = 0;

   // Layer 0 is defined to hold 3 RGB channels for input image:
   global_weight_node_id_map[DUPLE(0,0)] = global_weight_node_counter++;
   global_weight_node_id_map[DUPLE(0,1)] = global_weight_node_counter++;
   global_weight_node_id_map[DUPLE(0,2)] = global_weight_node_counter++;

// Compute distribution statistics for trained weights and biases
// within each layer containing parameters:

   cout << "n_param_layers = " << n_param_layers << endl;
   for(int p = 0; p < n_param_layers; p++)
   {
      cout << "Parameter layer p = " << p << endl;
      
      const shared_ptr<const caffe::Blob<float> > params_blob =
         net_->params().at(p);

      n_param_layer_nodes.push_back(params_blob->shape(0));
      cout << "n_param_layer_nodes = "
           << n_param_layer_nodes.back() << endl;

//      int init_global_weight_node = global_weight_node_counter;
      for(int w = 0; w < n_param_layer_nodes.back(); w++)
      {
         DUPLE duple(p + 1, w); // layer 0 holds input image
         global_weight_node_id_map[duple] = global_weight_node_counter++;
      }
//      int final_global_weight_node = global_weight_node_counter - 1;
//      cout << "  Initial global weight node ID = " << init_global_weight_node
//           << endl;
//      cout << "  Final global weight node ID = " << final_global_weight_node
//           << endl;

      const float *params_data = params_blob->cpu_data();

      vector<double> params;
      for(int i = 0; i < params_blob->count(); i++)
      {
         params.push_back(params_data[i]);
      }
      
      double params_mu, params_sigma;
      mathfunc::mean_and_std_dev(params, params_mu, params_sigma);
      double params_median, params_quartile_width;
      mathfunc::median_value_and_quartile_width(
         params, params_median, params_quartile_width);

      cout << "  Params:  mu = " << params_mu << " sigma = " << params_sigma 
//           << "   median = " << w_median << " quartile_width = "
//           << w_quartile_width 
           << endl;
   } // loop over index p labeling parameter layers

   cout << endl;
   cout << "////////////////////////////////////////////////" << endl;
   cout << endl;
   cout << "n_minor_layers = " << n_minor_layers << endl;
   cout << "minor_layer_skip = " << minor_layer_skip << endl;
   cout << "n_major_layers = " << n_major_layers << endl;
   cout << "n_param_layers = " << n_param_layers << endl;

   int major_weight_node_counter = 0;

   // Major layer 0 is defined to hold 3 RGB channels for input image:
   major_weight_node_id_map[DUPLE(0,0)] = major_weight_node_counter++;
   major_weight_node_id_map[DUPLE(0,1)] = major_weight_node_counter++;
   major_weight_node_id_map[DUPLE(0,2)] = major_weight_node_counter++;
   cout << "Major_layer_ID = 0 has 3 input RGB nodes" << endl;
   cout << "  Initial major weight node ID = 0" << endl;
   cout << "  Final major weight node ID = 2" << endl;


   for(int major_layer_ID = 1; major_layer_ID < n_major_layers; 
       major_layer_ID++)
   {
      int p = (major_layer_ID - 1) * minor_layer_skip;
			// Recall major layer 0 holds input image
      cout << "Major_layer_ID = " << major_layer_ID
           << " parameter layer p = " << p 
           << " n_param_layer_nodes[p] = "
           << n_param_layer_nodes[p] << endl;

      int init_major_weight_node = major_weight_node_counter;
      for(int w = 0; w < n_param_layer_nodes[p]; w++)
      {
         DUPLE duple(major_layer_ID, w); 
         major_weight_node_id_map[duple] = major_weight_node_counter++;
      }
      int final_major_weight_node = major_weight_node_counter - 1;
      cout << "  Initial major weight node ID = " << init_major_weight_node
           << endl;
      cout << "  Final major weight node ID = " << final_major_weight_node
           << endl;
   } // loop over index p labeling parameter layers

/*   
// Extract trained filters for very first convolutional layer:

   const shared_ptr<const caffe::Blob<float> > weights_blob =
      net_->params().at(0);

   int n_filters = weights_blob->shape(0);
   int n_RGB = weights_blob->shape(1);
   int height = weights_blob->shape(2);
   int width = weights_blob->shape(3);
   cout << "Very first convolutional layer:" << endl;
   cout << " n_filters = " << n_filters << " n_RGB = " << n_RGB
        << " height = " << height << " width = " << width << endl;

   string filters_subdir="./filters/";
   filefunc::dircreate(filters_subdir);

   vector<int> index;   
   texture_rectangle *tr_ptr = new texture_rectangle(width, height, 1, n_RGB,
                                                     NULL);
   for(int f = 0; f < n_filters; f++)
   {
      for(int py = 0; py < height; py++)
      {
         for(int px = 0; px < width; px++)
         {
            double r, g, b;
            for(int c = 0; c < n_RGB; c++)
            {
               index.clear();
               index.push_back(f);
               index.push_back(c);
               index.push_back(py);
               index.push_back(px);
               double curr_weight = weights_blob->data_at(index);
               if(c == 0)
               {
                  b = 128 + 255 * curr_weight;
               }
               else if(c == 1)
               {
                  g = 128 + 255 * curr_weight;
               }
               if(c == 2)
               {
                  r = 128 + 255 * curr_weight;
               }
            } // loop over index c labeling filter color channels
            int R = basic_math::max(0.0, r);
            int G = basic_math::max(0.0, g);
            int B = basic_math::max(0.0, b);

//            cout << "f = " << f 
//                 << " px = " << px << " py = " << py 
//                 << " R = " << R << " G = " << G << " B = " << B << endl;

            tr_ptr->set_pixel_RGB_values(px,py,R,G,B);

         } // loop over index px 
      } // loop over index py
      cout << endl;

      string filter_filename=filters_subdir + 
         "filter_"+stringfunc::integer_to_string(f,2)+".jpg";
      tr_ptr->write_curr_frame(filter_filename);
   } // loop over index f labeling conv1 filters
   
   delete tr_ptr;
*/

   cout << "..........................................." << endl;
//    outputfunc::enter_continue_char();
}


// ---------------------------------------------------------------------
int caffe_classifier::get_param_layer_n_input_nodes(int param_layer_index)
{
   const shared_ptr<const caffe::Blob<float> > param_blob =
      net_->params().at(param_layer_index);
   int n_input_nodes = param_blob->shape(1);
   return n_input_nodes;
}

int caffe_classifier::get_param_layer_n_output_nodes(int param_layer_index)
{
   const shared_ptr<const caffe::Blob<float> > param_blob =
      net_->params().at(param_layer_index);
   int n_output_nodes = param_blob->shape(0);
   return n_output_nodes;
}

// ---------------------------------------------------------------------
// Member function get_weight() takes in layer_index which labels
// weights between input layer layer_index+1 and and output layer
// layer_index+2 (one-based rather than zero-based layer counting).
// It also takes in the IDs for the nodes within the input and output
// layers.  For convolutional layers, we must also specify the height
// and width indices within the convolutional filters.  (For fully
// connected layers, height = width = 1).  get_weight() then returns
// the trained scalar weight value for the specified edge in the
// network.

float caffe_classifier::get_weight(
   int param_layer_index, 
   int input_node_ID, int output_node_ID, int height, int width)
{
   const shared_ptr<const caffe::Blob<float> > weights_blob =
      net_->params().at(param_layer_index);

   return weights_blob->data_at(output_node_ID, input_node_ID, height, width);
}

// ---------------------------------------------------------------------
// Member function get_weight_sum() returns an integrated weight
// summed over the height and width channels if they exist.

float caffe_classifier::get_weight_sum(
   int param_layer_index, int input_node_ID, int output_node_ID)
{
//   cout << "inside caffe_classifier::get_weight_sum()" << endl;
//   cout << "param_layer_index = " << param_layer_index
//        << " input_node_ID = " << input_node_ID
//        << " output_node_ID = " << output_node_ID << endl;
   
   const shared_ptr<const caffe::Blob<float> > weights_blob =
      net_->params().at(param_layer_index);

//   cout << "weights_blob->num_axes() = " << weights_blob->num_axes() << endl;
//   cout << " weights_blob.shape_string() = " << weights_blob->shape_string()
//        << endl;

   int height = 1, width = 1;
   if(weights_blob->num_axes() > 2)
   {
      height = weights_blob->shape(2);
      width = weights_blob->shape(3);
   }
//   cout << "height = " << height << " width = " << width << endl;

   float weight_sum = 0;
   for(int h = 0; h < height; h++)
   {
      for(int w = 0; w < width; w++)
      {
         weight_sum += get_weight(
            param_layer_index, input_node_ID, output_node_ID, h, w);
      }
   }
   return weight_sum;
}

// ---------------------------------------------------------------------
// Member function get_fully_connected_weight_sum() is a special case
// which needs to be called in the transition from the last
// convolutional layer to the first fully connected layer.  It sums
// all weights for each pixel within the reduced image chip for a
// given pair (reduced_input_node_ID = input_node_ID / reduced_chip_size, 
// output_node_ID).

float caffe_classifier::get_fully_connected_weight_sum(
   int param_layer_index, int reduced_chip_size, 
   int reduced_input_node_ID, int output_node_ID)
{
   const shared_ptr<const caffe::Blob<float> > weights_blob =
      net_->params().at(param_layer_index);

//   cout << "inside get_fully_connected_weight_sum()" << endl;
//   cout << "param_layer_index = " << param_layer_index
//        << " reduced_chip_size = " << reduced_chip_size
//        << endl;
//   cout << "reduced_input_node_ID = " << reduced_input_node_ID
//        << " output_node_ID = " << output_node_ID << endl;
//   cout << " weights_blob.shape_string() = " << weights_blob->shape_string() 
//        << endl;

   float weight_sum = 0;
   for(int r = 0; r < reduced_chip_size; r++)
   {
      int input_node_ID = reduced_input_node_ID * reduced_chip_size + r;
      weight_sum += get_weight(
         param_layer_index, input_node_ID, output_node_ID, 1, 1);
   }
   return weight_sum;
}

// ---------------------------------------------------------------------
// Member function get_global_weight_node_ID() takes in an index for some
// parameter layer as well as an index for some node within that
// layer.  It returns the global ID for the specified weight node.

int caffe_classifier::get_global_weight_node_ID(int minor_layer_index, int node_index)
{
   DUPLE duple(minor_layer_index, node_index);
   NODE_ID_MAP::iterator global_weight_node_id_iter = 
      global_weight_node_id_map.find(duple);
   if(global_weight_node_id_iter == global_weight_node_id_map.end())
   {
      cout << "Error in caffe_classifier::get_global_weight_node_ID()" << endl;
      cout << "minor_layer_index = " << minor_layer_index
           << " node_index = " << node_index << endl;
      outputfunc::enter_continue_char();
      return -1;
   }
   else
   {
      return global_weight_node_id_iter->second;
   }
}

// ---------------------------------------------------------------------
// Member function get_major_weight_node_ID() takes in an ID for some
// major layer as well as an index for some node within that layer.
// It returns the global ID for the specified weight node.

int caffe_classifier::get_major_weight_node_ID(int major_layer_ID, int node_index)
{
   DUPLE duple(major_layer_ID, node_index);
   NODE_ID_MAP::iterator major_weight_node_id_iter = 
      major_weight_node_id_map.find(duple);
   if(major_weight_node_id_iter == major_weight_node_id_map.end())
   {
      cout << "Error in caffe_classifier::get_major_weight_node_ID()" << endl;
      cout << "major_layer_ID = " << major_layer_ID
           << " node_index = " << node_index << endl;
      outputfunc::enter_continue_char();
      return -1;
   }
   else
   {
      return major_weight_node_id_iter->second;
   }
}

// ---------------------------------------------------------------------
// Member function rgb_img_to_bgr_fvec() subtracts off mean BGR values
// from the input RGB image.  It then converts the RGB values within
// the renormalized two-dimensional RGB image into a one-dimensional
// feature vector.  The 1D vector looks like

//             B1 B2 B3 .... G1 G2 G3 .... R1 R2 R3...

void caffe_classifier::rgb_img_to_bgr_fvec(texture_rectangle& curr_img)
{
//  cout << "inside caffe_classifier::rgb_img_to_bgr_fvec()" << endl;
   
   input_img_xdim = curr_img.getWidth();
   input_img_ydim = curr_img.getHeight();
   int n_dims = num_data_channels_ * input_img_ydim * input_img_xdim;
   feature_descriptor = new float[n_dims];

   int R, G, B;
   int index = 0;
   for(int c = 0; c < num_data_channels_; c++){
      for(int py = 0; py < input_img_ydim; py++){

         for(int px = 0; px < input_img_xdim; px++){

            float curr_value;
            if(c == 0)   // blue channel first
            {
               if(segmentation_flag)
               {
                  B = curr_img.fast_get_pixel_B_value(
                     px, input_img_ydim - 1 - py);
               }
               else
               {
                  B = curr_img.fast_get_pixel_B_value(px, py);
               }
               curr_value = B - mean_bgr.first;
            }
            else if (c == 1) // green channel second
            {
               if(segmentation_flag)
               {
                  G = curr_img.fast_get_pixel_R_value(
                     px, input_img_ydim - 1 - py);
               }
               else
               {
                  G = curr_img.fast_get_pixel_G_value(px, py);
               }
               curr_value = G - mean_bgr.second;
            }
            else  // red channel third
            {
               if(segmentation_flag)
               {
                  R = curr_img.fast_get_pixel_R_value(
                     px, input_img_ydim - 1 - py);
               }
               else
               {
                  R = curr_img.fast_get_pixel_R_value(px, py);
               }
               curr_value = R - mean_bgr.third;
            }
            
            feature_descriptor[index] = curr_value;
            index++;
         } // loop over index px
      } // loop over index py
   } // loop over index c labeling color channels
}

// ---------------------------------------------------------------------
// Member function generate_dense_map() checks whether the "data" for
// the loaded trained network corresponds to a Caffe layer or a Caffe
// blob.

void caffe_classifier::generate_dense_map()
{
   if(net_->has_layer("data"))
   {
      cout << "Testing network has a 'data' layer" << endl;
   }
   else if(net_->has_blob("data"))
   {
//      cout << "Testing network has a 'data' blob" << endl;
      generate_dense_map_data_blob();
   }
   else 
   {
      cout << "Network needs either an input layer or input blob with name data" << endl;
   }
}

// ---------------------------------------------------------------------
// Member function generate_dense_map_data_blob() reshapes a new data
// blob based upon input image's number of channels and pixel
// dimensions.  It then copies vectorized data from the input image
// into the new data blob.  The data blob is then passed into the
// trained network.  

void caffe_classifier::generate_dense_map_data_blob()
{
//   cout << "inside caffe_classifier::generate_dense_map_data_blob() " << endl;

// Create the data blob:

   caffe::Blob<float> data_blob;
   vector<int> shape;
   shape.push_back(1);
   shape.push_back(num_data_channels_);
   shape.push_back(input_img_ydim);
   shape.push_back(input_img_xdim);
   data_blob.Reshape(shape);
   float *data = data_blob.mutable_cpu_data();

// Copy new test image data into data blob:

   memcpy(data, feature_descriptor, 
          shape[0] * shape[1] * shape[2] * shape[3] * sizeof(float));

// Perform the forward pass on the data blob:

//    cout << "Performing forward inference pass" << endl;
   vector<caffe::Blob<float>*> input_blobs;
   input_blobs.push_back(&data_blob);

   const vector<caffe::Blob<float> *>& result_blobs = 
      net_->Forward(input_blobs);

   const caffe::Blob<float>* result_blob = result_blobs[0];

   if(segmentation_flag)
   {
      export_segmentation_mask(result_blob);
   }
   else
   {
      retrieve_classification_results(result_blob);
   }

   delete [] feature_descriptor;
   feature_descriptor=NULL;
}

// ---------------------------------------------------------------------
// Member function retrieve_layer_activations() takes in a data blob
// for some network layer.  It returns a list of node IDs and which
// are sorted in descending order according to their activation
// scores.  This method also returns the total number of nodes in the
// specified layer whose activations effectively equal zero.

int caffe_classifier::retrieve_layer_activations(
   string blob_name, vector<int>& node_IDs, vector<double>& node_activations,
   bool sort_activations_flag)
{
   const shared_ptr<const caffe::Blob<float> > curr_blob =
      net_->blob_by_name(blob_name);
   const float *blob_data = curr_blob->cpu_data();
   int n_filters = curr_blob->shape(1);

/*
   int num_axes = curr_blob->num_axes();
   int filter_width = 1;
   int filter_height = 1;
   if(num_axes > 2)
   {
      filter_width = curr_blob->shape(2);
      filter_height = curr_blob->shape(3);
   }

   cout << " curr_blob.shape_string() = " << curr_blob->shape_string() << end//l;
   cout << "n_filters = " << n_filters << endl;
   cout << " filter width = " << filter_width
        << " filter height = " << filter_height
        << endl;

   for(int a = 0; a < num_axes; a++)
   {
      int curr_shape = curr_blob->shape(a);
      cout << curr_shape << " " << flush;
      if(a < num_axes - 1) cout << "x " << flush;
   }
   cout << endl;
*/

   int n_tiny_activations = 0;
   const double TINY = 1E-10;
   node_IDs.clear();
   node_activations.clear();
   for(int n = 0; n < n_filters; n++)
   {
      node_IDs.push_back(n);
      node_activations.push_back(blob_data[n]);
      if(fabs(node_activations.back()) < TINY)
      {
         n_tiny_activations++;
      }
   }
   double tiny_activation_frac = double(n_tiny_activations) / n_filters;

//   cout << "n_tiny_activations = " << n_tiny_activations 
//        << " tiny_activation_frac = " << tiny_activation_frac << endl;

   if(sort_activations_flag)
   {
      templatefunc::Quicksort_descending(node_activations, node_IDs);
   }

   return n_tiny_activations;
}

// ---------------------------------------------------------------------
// Member function retrieve_classification_results() extracts
// n_classes softmax probability values from the "prob" blob.  The
// maximal softmax probability is returned as the classification
// score.  The label corresponding to the class with the highest
// classification probability is returned as the classification
// result.

void caffe_classifier::retrieve_classification_results(
   const caffe::Blob<float>* result_blob)
{
//   cout << "inside caffe_classifier::retrieve_classification_results()"
//        << endl;

// Softmax "prob" blob should have shape = 1 x n_classes :

   const shared_ptr<const caffe::Blob<float> > probs_blob =
      net_->blob_by_name("prob");

   int n_classes = probs_blob->shape(1);
   const float *probs_out = probs_blob->cpu_data();

   vector<float> class_probs;
   for(int c = 0; c < n_classes; c++)
   {
      class_probs.push_back(probs_out[c]);
//      cout << "c = " << c << " class_probs = " << class_probs.back()
//           << endl;
   }

   const float *class_argmaxes = result_blob->cpu_data();
   classification_result = class_argmaxes[0];
   classification_score = class_probs[classification_result];

//   cout << "classification_result = " << classification_result
//        << " classification_score = " << classification_score
//        << endl << endl;
}

// ---------------------------------------------------------------------
// Member function export_segmentation_mask()

void caffe_classifier::export_segmentation_mask(
   const caffe::Blob<float>* result_blob)
{
//   cout << "inside caffe_classifier::export_segmentation_mask()"
//        << endl;

   const float *argmaxs = result_blob->cpu_data();
//   cout << "argmaxs = " << argmaxs << endl;

   int height = result_blob->shape(2);
   int width = result_blob->shape(3);

   if(height != input_img_ydim || width != input_img_xdim)
   {
      cout << "Warning: Input and output image pixel dimensions are not equal" 
           << endl;
      cout << "   Output width = " << width 
           << " input_img_xdim = " << input_img_xdim << endl;
      cout << "   Output height = " << height 
           << " input_img_ydim = " << input_img_ydim << endl;
//       exit(-1);
   }

   label_tr_ptr=new texture_rectangle(width, height, 1, 1, NULL);
   score_tr_ptr=new texture_rectangle(width, height, 1, 1, NULL);

   label_tr_ptr->instantiate_ptwoDarray_ptr();
   score_tr_ptr->instantiate_ptwoDarray_ptr();

   // Fill label map:

   twoDarray* ptwoDarray_ptr = label_tr_ptr->get_ptwoDarray_ptr();
   for(int py = 0; py < height; py++)
   {
      for(int px = 0; px < width; px++)
      {
         float curr_label = argmaxs[py * width + px];
         ptwoDarray_ptr->put(px, height - 1 - py, curr_label);
      } // loop over px
   } // loop over py

//   string mask_filename="mask.jpg";
//   label_tr_ptr->initialize_twoDarray_image(ptwoDarray_ptr, 3, false);
//   label_tr_ptr->write_curr_frame(mask_filename);

//  Fill score map:
   ptwoDarray_ptr = score_tr_ptr->get_ptwoDarray_ptr();
   for(int py = 0; py < height; py++)
   {
      for(int px = 0; px < width; px++)
      {
         double curr_score = argmaxs[height * width + py * width + px];
         ptwoDarray_ptr->put(px, height - 1 - py, curr_score);
      } // loop over px
   } // loop over py

}

// ---------------------------------------------------------------------
// Member function display_segmentation_labels() loops over all
// pixels within *label_tr_ptr.  It resets the RGB values for any
// pixel labeled as background to greyscale.  

string caffe_classifier::display_segmentation_labels(
   texture_rectangle& curr_img, string output_subdir)
{
   const double SMALL = 0.001;

   double h, s, v;
   twoDarray* ptwoDarray_ptr = label_tr_ptr->get_ptwoDarray_ptr();
   for(unsigned int py = 0; py < ptwoDarray_ptr->get_ydim(); py++)
   {
      for(unsigned int px = 0; px < ptwoDarray_ptr->get_xdim(); px++)
      {
         curr_img.get_pixel_hsv_values(px, py, h, s, v);
         float curr_label = ptwoDarray_ptr->get(px, py);

         if(curr_label < SMALL)
         {
            s = 0;
            v *= 0.85;
         }
         else
         {
            double smin = 0.66;
            double smax = 1.0;
            double vmin = 0.5;
            double vmax = 1.0;

            h = 77 * (curr_label - 1);
//            h = 220 * curr_label; // for vertical segmentation charts only

            s = smin + s * (smax - smin);
            v = vmin + v * (vmax - vmin);
         }
         curr_img.set_pixel_hsv_values(px, py, h, s, v);
      } // loop over px
   } // loop over py

   string segmented_filename=curr_img.get_video_filename();
   string basename = filefunc::getbasename(segmented_filename);
   string basename_prefix=stringfunc::prefix(basename);
   segmented_filename=output_subdir+basename_prefix+"_segmented.png";

   curr_img.write_curr_frame(segmented_filename);
   cout << "Exported " << segmented_filename << endl;
   return segmented_filename;
}

// ---------------------------------------------------------------------
// Member function display_segmentation_scores() imports curr_img
// which we assume has previously been colored via a call to
// display_segmentation_labels().  It loops over all pixels within
// *score_tr_ptr.  The labeled pixels are recolored on a hot/red -
// cold/purple hue scale to indicate segmentation confidences.
// Coloring for background-class pixels remain as greyscale.

string caffe_classifier::display_segmentation_scores(
   texture_rectangle& curr_img, string output_subdir)
{
//   cout << "inside caffe_classifier::display_segmentation_scores()" << endl;
   twoDarray* ptwoDarray_ptr = score_tr_ptr->get_ptwoDarray_ptr();

   double minvalue, maxvalue;
   ptwoDarray_ptr->minmax_values(minvalue, maxvalue);
//   cout << "       minvalue = " << minvalue << " maxvalue = " << maxvalue 
//        << endl;
   double min_score = 0.3;	 // Empirically determined on 6/16/16
   double max_score = 1.0;

   for(unsigned int py = 0; py < ptwoDarray_ptr->get_ydim(); py++)
   {
      for(unsigned int px = 0; px < ptwoDarray_ptr->get_xdim(); px++)
      {
         double h,s,v;
         curr_img.get_pixel_hsv_values(px, py, h, s, v);
         float curr_score = ptwoDarray_ptr->get(px, py);

         double renorm_score = 
            (curr_score - min_score) / (max_score - min_score);
         h = 300 * (1 - renorm_score);

         curr_img.set_pixel_hsv_values(px, py, h, s, v);
      } // loop over px
   } // loop over py

   string score_filename=curr_img.get_video_filename();
   string basename = filefunc::getbasename(score_filename);
   string basename_prefix=stringfunc::prefix(basename);
   score_filename=output_subdir+basename_prefix+"_score.png";

   curr_img.write_curr_frame(score_filename);
   cout << "Exported " << score_filename << endl;
   return score_filename;
}

// ---------------------------------------------------------------------
// Member function cleanup_memory()

void caffe_classifier::cleanup_memory()
{
   delete label_tr_ptr;
   label_tr_ptr = NULL;

   delete score_tr_ptr;
   score_tr_ptr = NULL;

   delete cc_tr_ptr;
   cc_tr_ptr = NULL;
}
