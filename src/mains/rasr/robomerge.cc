// ==========================================================================
// Program ROBOMERGE reads in bounding box parameters and translated
// XYZ point clouds from individual ladar room scans generated by
// ROBOCEILING.  It first determines bbox corner point identifications
// for all input bounding boxes.  ROBOMERGE next determines
// corresponding direction vectors and side dimensions each input
// bbox.  It then rotates and scales the translated XYZ point clouds
// so that they can be trivially merged.  ROBOMERGE writes out two
// registered XYZ text files - one with ceiling points and one without
// which can be turned into OSGA binary files via ROBO2TDP.  Finally,
// this program exports a PNG image file which represents a projected
// floor plan for the room based upon all input ladar scans.
// ==========================================================================
// Last updated on 11/26/10; 11/28/10; 12/2/10
// ==========================================================================

#include <iostream>
#include <iomanip>
#include <set>
#include <string>
#include <vector>
#include "general/filefuncs.h"
#include "math/Genarray.h"
#include "image/imagefuncs.h"
#include "image/myimage.h"
#include "general/outputfuncs.h"
#include "image/recursivefuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "osg/osg3D/tdpfuncs.h"

#include "osg/osgGraphicals/AnimationController.h"
#include "image/binaryimagefuncs.h"
#include "image/connectfuncs.h"
#include "geometry/contour.h"
#include "image/drawfuncs.h"
#include "image/graphicsfuncs.h"
#include "datastructures/Hashtable.h"
#include "templates/mytemplates.h"
#include "numrec/nrfuncs.h"
#include "geometry/plane.h"
#include "math/prob_distribution.h"
#include "math/rotation.h"
#include "video/texture_rectangle.h"
#include "math/twovector.h"

using std::cin;
using std::cout;
using std::endl;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

   double xmin=POSITIVEINFINITY;
   double xmax=NEGATIVEINFINITY;
   double ymin=POSITIVEINFINITY;
   double ymax=NEGATIVEINFINITY;
   double zmin=POSITIVEINFINITY;
   double zmax=NEGATIVEINFINITY;

   int n_bbox_files=2;
//   int n_bbox_files=4;
   vector<double> WallAngle,Width,Length;
   vector<twovector> BboxCenter;
   Genarray<pair<double,double> >* Corners_ptr=
      new Genarray<pair<double,double> >(n_bbox_files,4);
   Genarray<pair<double,double> >* Corresponding_Corners_ptr=
      new Genarray<pair<double,double> >(n_bbox_files,4);

// Read in bounding box text files generated by program ROBOCEILING
// for each ladar room scan.  Store bbox corner coordinates within
// *Corners_ptr as a function of bbox index i and corner index c=0,1,2,3:

   for (int i=0; i<n_bbox_files; i++)
   {
      string bbox_filename="bbox"+stringfunc::number_to_string(i+1)+".dat";
      cout << "bbox_filename = " << bbox_filename << endl;
      filefunc::ReadInfile(bbox_filename);
      
      for (int j=0; j<filefunc::text_line.size(); j++)
      {
//         cout << filefunc::text_line[j] << endl;
         vector<string> substrings=
            stringfunc::decompose_string_into_substrings(
               filefunc::text_line[j]);
         if (j==0)
         {
            WallAngle.push_back(stringfunc::string_to_number(substrings[1]));
         }
         else if (j==1)
         {
            Width.push_back(stringfunc::string_to_number(substrings[1]));
         }
         else if (j==2)
         {
            Length.push_back(stringfunc::string_to_number(substrings[1]));
         }
         else if (j >= 3)
         {
            double x=stringfunc::string_to_number(substrings[1]);
            double y=stringfunc::string_to_number(substrings[2]);
            twovector V(x,y);

            if (j==3)
            {
               BboxCenter.push_back(V);
            }
            else
            {
               pair<double,double> P(x,y);
               int c=j-4;
               Corners_ptr->put(i,c,P);
            }
         }
      } // loop over index j labeling input text file line number
   } // loop over index i labeling input bbox text file

// Print out bbox parameters read in from text files:

   for (int i=0; i<n_bbox_files; i++)
   {
      cout << "i = " << i
           << " WallAngle = " << WallAngle[i]
           << " Width = " << Width[i]
           << " Length = " << Length[i] << endl;
      cout << "BboxCenter = " << BboxCenter[i] << endl;

      for (int c=0; c<4; c++)
      {
         pair<double,double> P=Corners_ptr->get(i,c);
         cout << "corner c = " << c 
              << " x = " << P.first
              << " y = " << P.second << endl;
      }
   } // loop over index i labeling bbox text file

// Start with zeroth corner of first bbox.  Find corner index offset
// (if any) which minimizes 2D distance of each bbox corner in second,
// third and fourth bboxes to counterpart corners in first bbox.
// Store 2D coordinates for corresponding bbox corners within
// *Corresponding_Corners_ptr as a function of bbox index i and corner
// index c=0,1,2,3:
    
   double zeroth_width=Width[0];
   double zeroth_length=Length[0];

   vector<int> corner_index_offset;
   for (int i=0; i<n_bbox_files; i++)
   {
      cout << "bbox file index i = " << i << endl;

// Use bbox width and length information to restrict possible matches
// between zeroth bbox corners and those of all other bboxes:

      double next_width=Width[i];
      double next_length=Length[i];
      const double fitting_error=0.3;	// meter

      int c_offset_start;
      if ( (fabs(next_width-zeroth_width) < fitting_error) &&
           (fabs(next_length-zeroth_length) < fitting_error) )
      {
         c_offset_start=0;
      }
      else
      {
         c_offset_start=1;
      }

      int best_c_offset=0;
      double min_separation_integral=POSITIVEINFINITY;
      for (int c_offset=c_offset_start; c_offset<4; c_offset += 2)
      {

         double separation_integral=0;
         for (int c=0; c<4; c++)
         {
            int d=modulo(c+c_offset,4);            

            pair<double,double> P=Corners_ptr->get(0,c);
            twovector zeroth_bbox_corner(P.first,P.second);
  
            pair<double,double> Q=Corners_ptr->get(i,d);
            twovector next_bbox_corner(Q.first,Q.second);

//            cout << "corner d = " << d
//                 << " x = " << Q.first
//                 << " y = " << Q.second << endl;

            separation_integral += (next_bbox_corner-zeroth_bbox_corner).
               magnitude();
         } // loop over corner index c for zeroth bbox

         cout << "c_offset = " << c_offset
              << " separation_integral = " << separation_integral << endl;

         if (separation_integral < min_separation_integral)
         {
            min_separation_integral=separation_integral;
            best_c_offset=c_offset;
         }
      } // loop over c_offset index

      cout << " corner_index_offset = " << best_c_offset << endl;

      for (int c=0; c<4; c++)
      {
         int d=modulo(c+best_c_offset,4);
         pair<double,double> Q=Corners_ptr->get(i,d);
         twovector next_bbox_corner(Q.first,Q.second);
         Corresponding_Corners_ptr->put(i,c,Q);
      }

   } // loop over index i labeling bbox text file
   
// Compute what = direction vector from corner0 to corner1 and lhat =
// direction vector from corner 1 to corner 2 for each bbox.  Store
// these bbox edge direction vectors in STL vectors whats & lhats:

   string banner="Corresponding bbox corners";
   outputfunc::write_big_banner(banner);

   Width.clear();
   Length.clear();
   vector<threevector> whats,lhats;
   for (int i=0; i<n_bbox_files; i++)
   {
      vector<twovector> corners;
      for (int c=0; c<4; c++)
      {
         pair<double,double> Q=Corresponding_Corners_ptr->get(i,c);
         twovector curr_corner(Q.first,Q.second);
         corners.push_back(curr_corner);
         cout << "i = " << i << " c = " << c
              << " x = " << curr_corner.get(0) 
              << " y = " << curr_corner.get(1) << endl;
      }
      Width.push_back( (corners[1]-corners[0]).magnitude());
      Length.push_back( (corners[2]-corners[1]).magnitude());
      
      whats.push_back( 
         threevector(corners[1]-corners[0]).unitvector() );
      lhats.push_back( 
         threevector(corners[2]-corners[1]).unitvector() );

      cout << "width = " << Width.back()
           << " length = " << Length.back() << endl << endl;
   } // loop over index i labeling bboxes
   
   double avg_width=mathfunc::mean(Width);
   double avg_length=mathfunc::mean(Length);
   cout << "Avg width = " << avg_width
        << " Avg length = " << avg_length << endl;

// Grid continuous XYZ points read in from "translated_pointsN.dat"
// generated by program ROBOCEILING onto raster images *ztwoDarray_ptr
// and *binary_ztwoDarray_ptr:

   double dx=0.01;	// meters
   double dy=0.01;	// meters
   xmax=0.5*avg_width+1;
   xmin=-xmax;
   ymax=0.5*avg_length+1;
   ymin=-ymax;
   int mdim=(xmax-xmin)/dx;
   int ndim=(ymax-ymin)/dy;
   
   twoDarray* ztwoDarray_ptr=new twoDarray(mdim,ndim);
   ztwoDarray_ptr->init_coord_system(xmin,xmax,ymin,ymax);
   ztwoDarray_ptr->clear_values();

   twoDarray* binary_ztwoDarray_ptr=new twoDarray(ztwoDarray_ptr);
   binary_ztwoDarray_ptr->clear_values();

   for (int i=0; i<n_bbox_files; i++)
   {

// Compute 2D rotation R about z_hat by which translated_pointsN.dat
// need to be so that all translated points align with xhat and yhat:

      double wall_angle=atan2(whats[i].get(1),whats[i].get(0));
      rotation R(0,0,-wall_angle);
//      cout << "R = " << R << endl;

// Compute scale factors which force each rotated, translated bounding
// box to have the same horizontal and vertical size:
      
      double w_scalefactor=avg_width/Width[i];
      double l_scalefactor=avg_length/Length[i];
      cout << "i = " << i 
           << " w_scale = " << w_scalefactor
           << " l_scale = " << l_scalefactor << endl;

      string translated_filename="translated_points"+
         stringfunc::number_to_string(i+1)+".dat";
      cout << "trans_filename = " << translated_filename << endl;
      filefunc::ReadInfile(translated_filename);
      
      vector<double> X,Y,Z;
      for (int j=0; j<filefunc::text_line.size(); j++)
      {
         vector<string> substrings=
            stringfunc::decompose_string_into_substrings(
               filefunc::text_line[j]);
         double x=stringfunc::string_to_number(substrings[0]);
         double y=stringfunc::string_to_number(substrings[1]);
         double z=stringfunc::string_to_number(substrings[2]);
         threevector init_posn(x,y,z);
         threevector new_posn=R*init_posn;
         new_posn.put(0,w_scalefactor*new_posn.get(0));
         new_posn.put(1,l_scalefactor*new_posn.get(1));
         X.push_back(new_posn.get(0));
         Y.push_back(new_posn.get(1));
         Z.push_back(new_posn.get(2));
      } // loop over index j 

// Export final set of registered points which have been translated,
// rotated and scaled so that multi-ladar scans can be trivially
// merged:
      
      string registered_filename="registered_points"+
         stringfunc::number_to_string(i+1)+".dat";
      ofstream outstream;
      filefunc::openfile(registered_filename,outstream);

      zmin=POSITIVEINFINITY;
      for (int p=0; p<Z.size(); p++)
      {
         outstream << X[p] << "   "
                   << Y[p] << "   "
                   << Z[p] << endl;
         zmin=min(Z[p],zmin);
      } // loop over index p
      filefunc::closefile(registered_filename,outstream);
   
      prob_distribution zprob(Z,100,zmin);
      double z2=zprob.find_x_corresponding_to_pcum(0.02);
      double z98=zprob.find_x_corresponding_to_pcum(0.98);
      cout << "z2 = " << z2 << endl << endl;
      cout << "z98 = " << z98 << endl << endl;
      double room_height=z98-z2;
      cout << "room_height = " << room_height << endl;

// For 3D point cloud viewing purposes, write out another set of
// points which does NOT include the ceiling:
      
      string non_ceiling_registered_filename="registered_points_wo_ceiling"+
         stringfunc::number_to_string(i+1)+".dat";
      filefunc::openfile(non_ceiling_registered_filename,outstream);

      zmin=POSITIVEINFINITY;
      for (int p=0; p<Z.size(); p++)
      {
         double renorm_z=Z[p]-z2;
         Z[p]=renorm_z;
         if (renorm_z > room_height-0.5) continue;
         if (renorm_z < 0) continue;
         
         outstream << X[p] << "   "
                   << Y[p] << "   "
                   << Z[p] << endl;

         xmin=min(xmin,X[p]);
         xmax=max(xmax,X[p]);
         ymin=min(ymin,Y[p]);
         ymax=max(ymax,Y[p]);
         zmin=min(zmin,Z[p]);
         zmax=max(ymax,Z[p]);

// Store 2D floorplan within *ztwoDarray_ptr by setting each of its
// pixels values equal to maximal chimney Z value:

         int px,py;
         const double z_above_floor=0.1;	// meter

         ztwoDarray_ptr->point_to_pixel(X[p],Y[p],px,py);
         double curr_z=ztwoDarray_ptr->get(px,py);
         if (Z[p] > z_above_floor && Z[p] > curr_z) 
            ztwoDarray_ptr->put(px,py,Z[p]);
         if (Z[p] > z_above_floor) binary_ztwoDarray_ptr->put(px,py,1);

      } // loop over index p labeling points within X,Y,Z STL vectors
      filefunc::closefile(non_ceiling_registered_filename,outstream);

   } // loop over index i labeling bboxes

   cout << "xmin = " << xmin << " xmax = " << xmax << endl;
   cout << "ymin = " << ymin << " ymax = " << ymax << endl;
   cout << "zmin = " << zmin << " zmax = " << zmax << endl;

// Write 2D floorplan to output PNG file via intermediate
// texture_rectangle:
   
   AnimationController* AnimationController_ptr=new AnimationController;

   int n_images=1;
   int n_channels=4;
   texture_rectangle* texture_rectangle_ptr=
      new texture_rectangle(mdim,ndim,n_images,n_channels,
      AnimationController_ptr);
   texture_rectangle_ptr->initialize_twoDarray_image(
      ztwoDarray_ptr,n_channels);   

   double hue_min=300;
   double hue_max=30;
   double output_alpha=1;
   texture_rectangle_ptr->convert_greyscale_image_to_hue_colored(
      hue_min,hue_max,output_alpha);
   string output_filename="floorplan.png";
   texture_rectangle_ptr->write_curr_frame(output_filename);

   texture_rectangle_ptr->initialize_twoDarray_image(
      binary_ztwoDarray_ptr);   
   output_filename="binary_floorplan.png";
   texture_rectangle_ptr->write_curr_frame(output_filename);
}
