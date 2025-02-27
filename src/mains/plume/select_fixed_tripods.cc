// ========================================================================
// Program SELECT_FIXED_TRIPODS reads in a set of georegistered
// package files corresponding to fixed tripod cameras.  It also
// imports surveyed camera positions from a text file generated by
// program GEOREGISTER_CAMERAS.  SELECT_FIXED_TRIPODS visualizes the
// candidate georegistered cameras as selectable view frusta and their
// surveyed positions as 3D "signposts".  When a user doubleclicks on
// any view frustum, its corresponding image and package filenames are
// displayed in the terminal window.  

// SELECT_FIXED_TRIPODS will hopefully help a human user cull all
// candidate fixed tripod package files down to a final set of 10.  

// ========================================================================
// Last updated on 9/13/12; 9/18/12
// ========================================================================

#include <iostream>
#include <string>
#include <vector>

#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "osg/osgGraphicals/AnimationController.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgSceneGraph/HiresDataVisitor.h"
#include "osg/ModeController.h"
#include "osg/osgSceneGraph/MyDatabasePager.h"
#include "osg/osgOperations/Operations.h"
#include "passes/PassesGroup.h"
#include "postgres/plumedatabasefuncs.h"
#include "osg/osg3D/PointCloudsGroup.h"
#include "osg/osg3D/PointCloudKeyHandler.h"
#include "osg/osgGeometry/PolyhedraGroup.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "osg/osgWindow/ViewerManager.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::ifstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:
   
   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   string image_list_filename=passes_group.get_image_list_filename();
   cout << " image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string image_sizes_filename=passes_group.get_image_sizes_filename();
   cout << "image_sizes_filename = " << image_sizes_filename << endl;
   string packages_subdir=bundler_IO_subdir+"packages/";

   int cloudpass_ID=passes_group.get_curr_cloudpass_ID();
   cout << "cloudpass_ID = " << cloudpass_ID << endl;

// Import associations between image and package filenames into STL
// map:
   
   string images_vs_packages_filename=bundler_IO_subdir+
      "images_vs_packages.dat";
   filefunc::ReadInfile(images_vs_packages_filename);

   typedef map<string,string> IMAGE_PACKAGE_MAP;
   IMAGE_PACKAGE_MAP image_package_map;

   for (int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i]);
      string image_filename=substrings[0];
      string package_filename=substrings[1];
      image_package_map[image_filename]=package_filename;
   }

// Search for symbolic links within images subdirectory.  As of
// 9/18/2012, we assume any such link points to a fixed tripod image.
// So we add association between such symbolic links and the package
// files associated with their resolved filenames to
// image_package_map:

   string images_subdir=bundler_IO_subdir+"images/";
   vector<string> image_filenames=filefunc::image_files_in_subdir(
      images_subdir);
   for (int i=0; i<image_filenames.size(); i++)
   {
      if (!filefunc::symboliclink_exist(image_filenames[i])) continue;

      string dirname=filefunc::getdirname(image_filenames[i]);
      string resolved_filename=dirname+filefunc::resolve_linked_filename(
         image_filenames[i]);
//      cout << "symlink = " << image_filenames[i]
//           << " resolved name = " << resolved_filename << endl;
      
      IMAGE_PACKAGE_MAP::iterator iter=
         image_package_map.find(resolved_filename);
      if (iter != image_package_map.end())
      {
         string package_filename=iter->second;
         image_package_map[image_filenames[i],package_filename];
      }
   } // loop over index i labeling image filenames within images_subdir

// As of Sep 2012, we assume that fixed tripod image names start with
// their camera numbers (e.g. 17_6268.rd.jpg or
// 18_IMG_0728_DAY4D.JPG).  So we search through the images
// subdirectory and count the number of fixed tripod image files which
// start with an integer:

   int n_tripod_images=0;
   vector<string> package_filenames;
   for (int i=0; i<image_filenames.size(); i++)
   {
      string image_basename=filefunc::getbasename(image_filenames[i]);

      string separator_chars="_";
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         image_basename,separator_chars);
      if (stringfunc::is_number(substrings[0]))
      {
         n_tripod_images++;
         string tripod_filename;
         if (filefunc::symboliclink_exist(image_filenames[i])) 
         {
            string dirname=filefunc::getdirname(image_filenames[i]);
            string resolved_filename=dirname+filefunc::resolve_linked_filename(
               image_filenames[i]);
            tripod_filename=resolved_filename;
         }
         else
         {
            tripod_filename=image_filenames[i];
         }

         string package_filename="";
         IMAGE_PACKAGE_MAP::iterator iter=
            image_package_map.find(tripod_filename);
         if (iter != image_package_map.end())
         {
            package_filename=iter->second;
         }
         package_filenames.push_back(package_filename);
      }
   }

   passes_group.interpret_arguments(package_filenames);
   passes_group.generate_passes_from_arguments();

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
//   photogroup_ptr->read_photographs(passes_group,image_sizes_filename);
   photogroup_ptr->read_photographs(passes_group);
   int n_photos=photogroup_ptr->get_n_photos();

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("3D imagery");
   
// Create OSG root node:

   osg::Group* root = new osg::Group;

// Instantiate Operations object to handle mode, animation and image
// number control:

   bool display_movie_state=false;
   bool display_movie_number=false;
   bool display_movie_world_time=false;
   bool hide_Mode_HUD_flag=false;
//   bool hide_Mode_HUD_flag=true;
   Operations operations(
      ndims,window_mgr_ptr,passes_group,
      display_movie_state,display_movie_number,display_movie_world_time,
      hide_Mode_HUD_flag);
   root->addChild(operations.get_OSGgroup_ptr());

   ModeController* ModeController_ptr=operations.get_ModeController_ptr();
   AnimationController* AnimationController_ptr=
      operations.get_AnimationController_ptr();

// Add a custom manipulator to the event handler list:

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_ptr,window_mgr_ptr);
   CM_3D_ptr->set_min_camera_height_above_grid(0.1);	// meters
   CM_3D_ptr->set_enable_underneath_looking_flag(true);	
   window_mgr_ptr->set_CameraManipulator(CM_3D_ptr);

// Instantiate group to hold all decorations:
   
   Decorations decorations(window_mgr_ptr,ModeController_ptr,CM_3D_ptr);

// Instantiate signpost decoration groups:

   SignPostsGroup* SignPostsGroup_ptr=
      decorations.add_SignPosts(ndims,passes_group.get_pass_ptr(cloudpass_ID));
//   SignPostsGroup_ptr->set_CM_3D_ptr(CM_3D_ptr);
   SignPostsGroup_ptr->set_altitude_dependent_size_flag(false);

// If a SignPosts text file has previously been generated by program
// GEOREGISTER_CAMERAS, import it to display surveyed camera tripod
// locations:

   bool smallgrid_flag=true;
   string signposts_filename=bundler_IO_subdir+"measured_camera_signposts.txt";
   if (filefunc::fileexist(signposts_filename))
   {
      SignPostsGroup_ptr->read_info_from_file(signposts_filename);
      SignPostsGroup_ptr->change_size(0.5);
      SignPostsGroup_ptr->change_size(0.5);
      SignPostsGroup_ptr->change_size(0.5);

      threevector SignPost_posn;
      for (int s=0; s<SignPostsGroup_ptr->get_n_Graphicals(); s++)
      {
         SignPost* SignPost_ptr=SignPostsGroup_ptr->get_SignPost_ptr(s);
         SignPost_ptr->get_UVW_coords(
            SignPostsGroup_ptr->get_curr_t(),
            SignPostsGroup_ptr->get_passnumber(),SignPost_posn);
      }

      if (SignPost_posn.magnitude() > 35)
      {
         smallgrid_flag=false;
      }
   } // signposts_filename exists conditional

// Instantiate Grid:

   AlirtGrid* grid_ptr=decorations.add_AlirtGrid(
      ndims,passes_group.get_pass_ptr(cloudpass_ID));
   threevector* grid_origin_ptr=grid_ptr->get_world_origin_ptr();
   CM_3D_ptr->set_Grid_ptr(grid_ptr);
   decorations.set_grid_origin_ptr(grid_origin_ptr);

   if (smallgrid_flag)
   {
      grid_ptr->initialize_ALIRT_grid(-25,25,-25,25,0);	// Nov 2011 Expt 2B
      grid_ptr->set_delta_xy(5,5);
      grid_ptr->set_axis_char_label_size(5);
      grid_ptr->set_tick_char_label_size(2);
   }
   else
   {
      grid_ptr->initialize_ALIRT_grid(-50,50,-50,50,0); // Nov 2011 Expt 5C
      grid_ptr->set_delta_xy(10,10);			// & Nov 2012 Day 2
      grid_ptr->set_axis_char_label_size(10);
      grid_ptr->set_tick_char_label_size(4);
   }
   grid_ptr->set_axes_labels(
      "Relative East (meters)","Relative North (meters)");
   grid_ptr->update_grid();
      
//   cout << "*grid_origin_ptr = " << *grid_origin_ptr << endl;

// Instantiate group to hold pointcloud information:

   PointCloudsGroup clouds_group(
      passes_group.get_pass_ptr(cloudpass_ID),grid_origin_ptr);
   clouds_group.generate_Clouds(passes_group);
   clouds_group.set_Terrain_Manipulator_ptr(CM_3D_ptr);
   clouds_group.set_auto_resize_points_flag(false);
   decorations.set_PointCloudsGroup_ptr(&clouds_group);

   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new PointCloudKeyHandler(&clouds_group,ModeController_ptr,CM_3D_ptr));

// Instantiate a MyDatabasePager to handle paging of files from disk:

   viewer::MyDatabasePager* MyDatabasePager_ptr=new viewer::MyDatabasePager(
      clouds_group.get_SetupGeomVisitor_ptr(),
      clouds_group.get_ColorGeodeVisitor_ptr());
   clouds_group.get_HiresDataVisitor_ptr()->setDatabasePager(
      MyDatabasePager_ptr);

// Instantiate Polyhedra decoration groups:

   PolyhedraGroup* PolyhedraGroup_ptr=decorations.add_Polyhedra(
      passes_group.get_pass_ptr(cloudpass_ID));
   PolyhedraGroup_ptr->set_OFF_subdir(
      "/home/cho/programs/c++/svn/projects/src/mains/plume/OFF/");

// Instantiate OBSFRUSTA decoration group:

   OBSFRUSTAGROUP* OBSFRUSTAGROUP_ptr=decorations.add_OBSFRUSTA(
      passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->set_enable_OBSFRUSTA_blinking_flag(false);
   OBSFRUSTAGROUP_ptr->set_erase_other_OBSFRUSTA_flag(true);
   OBSFRUSTAGROUP_ptr->populate_image_vs_package_names_map(packages_subdir);

// Instantiate an individual OBSFRUSTUM for every photograph:

   bool multicolor_frusta_flag=false;
   bool thumbnails_flag=true;
//   OBSFRUSTAGROUP_ptr->generate_still_imagery_frusta_for_photogroup(
//      photogroup_ptr,multicolor_frusta_flag,thumbnails_flag);

   double frustum_sidelength=2.5;
//   double frustum_sidelength=7.5;
   double movie_downrange_distance=-1;
   OBSFRUSTAGROUP_ptr->generate_still_imagery_frusta_for_photogroup(
      photogroup_ptr,frustum_sidelength,movie_downrange_distance,
      multicolor_frusta_flag,thumbnails_flag);

   decorations.get_OBSFRUSTUMPickHandler_ptr()->
      set_mask_nonselected_OSGsubPATs_flag(true);
   decorations.get_OBSFRUSTAKeyHandler_ptr()->set_SignPostsGroup_ptr(
      SignPostsGroup_ptr);

/*
// ========================================================================
// Mark 3D locations of November 2011 grid sensors using colored
// Geometricals (hemispheres, boxes, cylinders, cones)
// ========================================================================

// Constants

   const double feet_per_meter=3.2808;
   double alpha=0.99;

// Repeated variable declarations

   double dz;
   vector<threevector> XYZ;

   double thetax=0;
   double thetay=0;
   double thetaz=0*PI/180;

//   cout << "Enter theta_z in degs:" << endl;
//   cin >> thetaz;

   thetaz=0;  // Angle needed to align sensors grid with recon pt cloud
//   thetaz=39;  // Angle needed to align sensors grid with recon pt cloud

   thetaz *= PI/180;
   rotation R(thetax,thetay,thetaz);

   double delta_x,delta_y;

//   cout << "Enter delta x in meters:" << endl;
//   cin >> delta_x;
//   cout << "Enter delta y in meters:" << endl;
//   cin >> delta_y;

// Average position for all 10 reconstructed camera tripods determined
// for Day 5 on 12/16/2011:

   threevector avg_camera_posn(0.080055 , 0.377872 , 0.043724 ) ;

   threevector sensor_grid_center=avg_camera_posn+threevector(delta_x,delta_y);
   cout << "sensor grid center = " << sensor_grid_center << endl;

// Instantiate SphereSegments decoration group:

   SphereSegmentsGroup* SphereSegmentsGroup_ptr=
      decorations.add_SphereSegments(
         passes_group.get_pass_ptr(cloudpass_ID));
   
   dz=0;

   XYZ.clear();
   XYZ.push_back(threevector(-6,-6,dz));
   XYZ.push_back(threevector(0,-6,dz));
   XYZ.push_back(threevector(6,-6,dz));
   XYZ.push_back(threevector(-6,0,dz));
   XYZ.push_back(threevector(6,0,dz));
   XYZ.push_back(threevector(-6,6,dz));
   XYZ.push_back(threevector(0,6,dz));
   XYZ.push_back(threevector(6,6,dz));
   
   colorfunc::Color hemisphere_color=colorfunc::yegr;
   double hemisphere_radius=0.5;
   for (int i=0; i<XYZ.size(); i++)
   {
      threevector rel_hemisphere_posn(R*XYZ[i]);
      threevector hemisphere_posn=sensor_grid_center+rel_hemisphere_posn;
      SphereSegment* SphereSegment_ptr=SphereSegmentsGroup_ptr->
         generate_new_hemisphere(hemisphere_radius,hemisphere_posn);
      SphereSegment_ptr->set_permanent_color(colorfunc::get_OSG_color(
         hemisphere_color,alpha));
   }

// Instantiate boxes decoration group:

   BoxesGroup* BoxesGroup_ptr=decorations.add_Boxes(
      passes_group.get_pass_ptr(cloudpass_ID));
   BoxesGroup_ptr->set_wlh(0.5,0.5,0.5);

   dz=0 /feet_per_meter;	// meters

   XYZ.clear();
   XYZ.push_back(threevector(-16,-16,dz));
   XYZ.push_back(threevector(16,-16,dz));

   XYZ.push_back(threevector(-10,0,dz));
   XYZ.push_back(threevector(0,10,dz));
   XYZ.push_back(threevector(0,-10,dz));
   XYZ.push_back(threevector(10,0,dz));

   XYZ.push_back(threevector(16,16,dz));
   XYZ.push_back(threevector(-16,16,dz));

   XYZ.push_back(threevector(3,0,dz));
   XYZ.push_back(threevector(-3,0,dz));
   XYZ.push_back(threevector(0,3,dz));
   XYZ.push_back(threevector(0,-3,dz));

   colorfunc::Color box_color=colorfunc::red;
   for (int i=0; i<XYZ.size(); i++)
   {
      threevector rel_box_posn(R*XYZ[i]);
      threevector box_center=sensor_grid_center+rel_box_posn;
      Box* Box_ptr=BoxesGroup_ptr->generate_new_Box(box_center);
      Box_ptr->set_permanent_color(colorfunc::get_OSG_color(
         box_color,alpha));
   }

// Instantiate cylinders decoration group:

   CylindersGroup* CylindersGroup_ptr=
      decorations.add_Cylinders(passes_group.get_pass_ptr(cloudpass_ID),
                                AnimationController_ptr);
   osg::Quat q(0,0,0,1);

   double cyl_height=10 /feet_per_meter;	// meters
   CylindersGroup_ptr->set_rh(0.5,0.5*cyl_height);

   XYZ.clear();
   XYZ.push_back(threevector(-12,-12,0));
   XYZ.push_back(threevector(0,-12,0));
   XYZ.push_back(threevector(12,-12,0));
   XYZ.push_back(threevector(-12,0,0));
   XYZ.push_back(threevector(12,0,0));
   XYZ.push_back(threevector(12,12,0));
   XYZ.push_back(threevector(0,12,0));
   XYZ.push_back(threevector(-12,12,0));

   colorfunc::Color cyl_color=colorfunc::blgr;
   for (int i=0; i<XYZ.size(); i++)
   {
      threevector rel_cyl_posn(R*XYZ[i]);
      threevector cyl_center=sensor_grid_center+rel_cyl_posn+0.5*cyl_height*
         z_hat;
      Cylinder* Cylinder_ptr=CylindersGroup_ptr->generate_new_Cylinder(
         cyl_center,q,cyl_color);
      Cylinder_ptr->set_permanent_color(
         colorfunc::get_OSG_color(cyl_color,alpha));
   }

// Instantiate cones decoration group:

   ConesGroup* ConesGroup_ptr=
      decorations.add_Cones(passes_group.get_pass_ptr(cloudpass_ID));

   double cone_height=10 /feet_per_meter;	// meters
   double cone_radius=0.5;
   ConesGroup_ptr->set_rh(cone_radius,cone_height);

   XYZ.clear();
   XYZ.push_back(threevector(0,0,0));		// Marks sensor grid center

   XYZ.push_back(threevector(-20,-20,0));
   XYZ.push_back(threevector(-7,-20,0));
   XYZ.push_back(threevector(7,-20,0));
   XYZ.push_back(threevector(20,-20,0));

   XYZ.push_back(threevector(-12,12,0));
   XYZ.push_back(threevector(0,12,0));
   XYZ.push_back(threevector(12,12,0));

   XYZ.push_back(threevector(-20,7,0));
   XYZ.push_back(threevector(20,7,0));

   XYZ.push_back(threevector(-12,0,0));
   XYZ.push_back(threevector(12,0,0));

   XYZ.push_back(threevector(-20,-7,0));
   XYZ.push_back(threevector(20,-7,0));

   XYZ.push_back(threevector(-12,-12,0));
   XYZ.push_back(threevector(0,-12,0));
   XYZ.push_back(threevector(12,-12,0));

   XYZ.push_back(threevector(20,20,0));
   XYZ.push_back(threevector(-7,20,0));
   XYZ.push_back(threevector(7,20,0));
   XYZ.push_back(threevector(-20,20,0));

   colorfunc::Color cone_color=colorfunc::yellow;
   for (int i=0; i<XYZ.size(); i++)
   {
      if (i > 0)
      {
         cone_color=colorfunc::blue;	// 5' particle counters
      }

      threevector rel_cone_posn(R*XYZ[i]);
      threevector cone_base=rel_cone_posn;
      threevector cone_tip=cone_base+threevector(0,0,cone_height);

      Cone* Cone_ptr=ConesGroup_ptr->generate_new_Cone();
      Cone_ptr->set_theta(PI);

      threevector trans=cone_tip;
      trans += sensor_grid_center - *grid_origin_ptr;

      Cone_ptr->scale_rotate_and_then_translate(
         ConesGroup_ptr->get_curr_t(),ConesGroup_ptr->get_passnumber(),trans);

      Cone_ptr->set_color(
         colorfunc::get_OSG_color(cone_color,alpha));
   } 
*/

// ========================================================================

   root->addChild(decorations.get_OSGgroup_ptr());
//   root->addChild(clouds_group.get_OSGgroup_ptr());

// Attach scene graph to viewer:

   window_mgr_ptr->setSceneData(root);

// Create the windows and run the threads.  Viewer's realize method
// calls the CustomManipulator's home() method:

   window_mgr_ptr->realize();
   
   while( !window_mgr_ptr->done() )
   {
      window_mgr_ptr->process();
   }

   delete window_mgr_ptr;
}

