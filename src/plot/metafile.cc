// ==========================================================================
// Metafile class member function definitions
// ==========================================================================
// Last modified on 1/2/17; 1/5/17 1/6/17; 1/13/17
// ==========================================================================

#include "math/basic_math.h"
#include "color/colorfuncs.h"
#include "general/filefuncs.h"
#include "math/mathfuncs.h"
#include "plot/metafile.h"
#include "templates/mytemplates.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::ios;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

// ---------------------------------------------------------------------
// Initialization, constructor and destructor methods:
// ---------------------------------------------------------------------

void metafile::allocate_member_objects() 
{
}

void metafile::initialize_member_objects() 
{
   legend_flag=false;
   swap_axes=false;
   print_storylines=true;
   plot_only_points=false;
   number_points=false;
   point_number_interval=5;
   thickness=nplots=1;
   currplot=0;
   style=0;
   metafilename=title=subtitle="";
   legendtitle="LEGEND";
   xlabel=ylabel=colored_curve_label=legendlabel="";
   label_color=colorfunc::null;
   xmax=xmin=xtic=xsubtic=xsize=xphysor=0;
   ymax=ymin=ytic=ysubtic=ysize=yphysor=0;
   x_legend_posn=5.2;	// inches relative to physor origin
//   x_legend_posn=6.2;	// inches relative to physor origin
//   x_legend_posn=7.2;	// inches relative to physor origin
   y_legend_posn=6.5;	// inches relative to physor origin
   legend_size=1;
}

metafile::metafile(void)
{
   initialize_member_objects();
   allocate_member_objects();
}

// Copy constructor:

metafile::metafile(const metafile& m)
{
   initialize_member_objects();
   allocate_member_objects();
   docopy(m);
}

// ---------------------------------------------------------------------
// On 6/11/01, we learned from James Wanken that we must explicitly
// delete any object which has been dynamically allocated.  In
// particular, we need to traverse the linked list in the destructor
// and explicitly delete each of the nodes before a linked list goes
// out of scope:

metafile::~metafile()
{
}

// ---------------------------------------------------------------------
void metafile::docopy(const metafile& m)
{
   legend_flag=m.legend_flag;
   swap_axes=m.swap_axes;
   print_storylines=m.print_storylines;
   plot_only_points=m.plot_only_points;
   thickness=m.thickness;
   style=m.style;
   nplots=m.nplots;
   currplot=m.currplot;
   metafilename=m.metafilename;
   pagetitle=m.pagetitle;
   title=m.title;
   subtitle=m.subtitle;
   legendtitle=m.legendtitle;
   xlabel=m.xlabel;
   ylabel=m.ylabel;
   colored_curve_label=m.colored_curve_label;
   legendlabel=m.legendlabel;
   label_color=m.label_color;
   xmax=m.xmax;
   xmin=m.xmin;
   ymax=m.ymax;
   ymin=m.ymin;
   xtic=m.xtic;
   xsubtic=m.xsubtic;
   xsize=m.xsize;
   xphysor=m.xphysor;
   ytic=m.ytic;
   ysubtic=m.ysubtic;
   ysize=m.ysize;
   yphysor=m.yphysor;
   x_legend_posn=m.x_legend_posn;
   y_legend_posn=m.y_legend_posn;
   legend_size=m.legend_size;

   for (unsigned int i=0; i <extraline.size(); i++)
   {
      extraline[i]=m.extraline[i];
   }

   for (unsigned int i=0; i<extrainfo.size(); i++)
   {
      extrainfo[i]=m.extrainfo[i];
   }
}	

// Overload = operator:

metafile& metafile::operator= (const metafile& m)
{
   if (this==&m) return *this;
   docopy(m);
   return *this;
}

// Overload << operator:

ostream& operator<< (ostream& outstream,const metafile& m)
{
   outstream << endl;
   outstream << "plot_only_points = " << m.plot_only_points << endl;
   outstream << "metafilename = " << m.metafilename << endl;
   outstream << "title = " << m.title << endl;
   outstream << "xlabel = " << m.xlabel << endl;
   outstream << "ylabel = " << m.ylabel << endl;
   
   outstream << "xmax = " << m.xmax << endl;
   outstream << "xmin = " << m.xmin << endl;
   outstream << "xtic = " << m.xtic << endl;
   outstream << "xsubtic = " << m.xsubtic << endl;
   outstream << "ymax = " << m.ymax << endl;
   outstream << "ymin = " << m.ymin << endl;
   outstream << "ytic = " << m.ytic << endl;
   outstream << "ysubtic = " << m.ysubtic << endl;
   outstream << endl;
   return outstream;
}

// ==========================================================================
// Plotting initialization member functions
// ==========================================================================

void metafile::openmetafile()
{
   filefunc::openfile(metafilename+".meta",metastream);
}

// ---------------------------------------------------------------------
// Member function write_header writes out preliminary header
// information at top of linked list output file to set up for meta
// plotting.

void metafile::write_header()
{
   const int NSUBTICS=5; // # subtics between major tics on meta plots

// Two sets of "story" commands cannot be requested within one plot.
// So to get more than one column of story output to appear, we need
// to create dummy plots with no content and then call the story
// command.  Title, x axis and y axis calls are mandatory when setting
// up a plot.
   
   if (extrainfo.size() > 0)
   {
      metastream << "title ''" << endl;
      if (xsize > 0 || ysize > 0)
      {
         metastream << "size "+stringfunc::number_to_string(xsize)+" "
            +stringfunc::number_to_string(ysize) << endl;
      }
      if (xphysor > 0 || yphysor > 0)
      {
         metastream << "physor "+stringfunc::number_to_string(xphysor)+" "
            +stringfunc::number_to_string(yphysor) << endl;
      }
      metastream << "x axis min 0 max 0.0001" << endl;
      metastream << "y axis min 0 max 0.0001" << endl;
      
      for (unsigned int i=0; i<extrainfo.size(); i++)
      {
         metastream << "story '"+extrainfo[i]+"'" << endl;
      }
      metastream << "charsize 0.8" << endl;
      metastream << "storyloc 6 6.25" << endl;
      metastream << "" << endl;
   }

   if (pagetitle != "")
   {
      metastream << "pagetitle '"+pagetitle+"'" << endl;
   }
   metastream << "title '"+title+"'" << endl;
   if (subtitle != "")
   {
      metastream << "subtitle '"+subtitle+"'" << endl;
   }

// If swap_axes flag==true, swap all x and f axes parameters:

   if (swap_axes)
   {
      templatefunc::swap(xlabel,ylabel);
      templatefunc::swap(xsize,ysize);
      templatefunc::swap(xphysor,yphysor);
      templatefunc::swap(xmin,ymin);
      templatefunc::swap(xmax,ymax);
      templatefunc::swap(xtic,ytic);
      templatefunc::swap(xsubtic,ysubtic);
   }

   if (xsize > 0 || ysize > 0)
   {
      metastream << "size "+stringfunc::number_to_string(xsize)+" "
         +stringfunc::number_to_string(ysize) << endl;
   }
   if (xphysor > 0 || yphysor > 0)
   {
      metastream << "physor "+stringfunc::number_to_string(xphysor)+" "
         +stringfunc::number_to_string(yphysor) << endl;
   }

   metastream << "x axis min "+stringfunc::number_to_string(xmin)+" max "
	+stringfunc::number_to_string(xmax) << endl;
   metastream << "label '"+xlabel+"'" << endl;

// Allow for manual overriding of automatic x axis tic settings

   if (xtic==0)
   {

// If xmax > 0 and xmin < 0, use larger absolute value of xmax
// and xmin to set tics rather than their difference:

      double xdiff;
      if (xmax*xmin > 0) 
      {
         xdiff=xmax-xmin;
      }
      else
      {
         xdiff=basic_math::max(fabs(xmax),fabs(xmin));
      }
      metastream << "tics "+stringfunc::number_to_string(trunclog(xdiff))+" "
         +stringfunc::number_to_string(trunclog(xdiff)/NSUBTICS) << endl;
   }
   else
   {
      metastream << "tics "+stringfunc::scinumber_to_string(xtic)
         +" "+stringfunc::scinumber_to_string(xsubtic) << endl;
   }
   metastream << "y axis min "+stringfunc::scinumber_to_string(ymin)
      +" max "+stringfunc::scinumber_to_string(ymax) << endl;
   metastream << "label '"+ylabel+"'" << endl;

// Allow for manual overriding of y axis tic settings

   if (ytic != 0)
   {
      metastream << "tics "+stringfunc::scinumber_to_string(ytic)
         +" "+stringfunc::scinumber_to_string(ysubtic) << endl;
   }

// If ymax > 0 and ymin < 0, use larger absolute
// value of ymax and ymin to set tics rather than
// their difference:

   else
   {
      double ydiff;
      if (ymin*ymax > 0) 
      {
         ydiff=(ymax-ymin);
      }
      else
      {
         ydiff=basic_math::max(fabs(ymax),fabs(ymin));
      }
      metastream << "tics "+stringfunc::scinumber_to_string(trunclog(ydiff))
         +" "+stringfunc::scinumber_to_string(trunclog(ydiff)/NSUBTICS) 
                 << endl;
   }

   if (print_storylines)
   {
      outputfunc::print_filename_and_date(metastream,metafilename);
   }

   include_legend_header();
//   include_colored_curve_labels();
   metastream << endl;
}

// ---------------------------------------------------------------------
void metafile::include_legend_header()
{
   if (legend_flag)
   {
      metastream << "legend '" << legendtitle << "'" << endl;
      metastream << "legloc " << x_legend_posn << " " << y_legend_posn 
                 << endl;
      metastream << "legsize " << legend_size << endl;
      metastream << endl;
   }
}

// ---------------------------------------------------------------------
void metafile::include_colored_curve_labels()
{
   if (nplots > 1 && colored_curve_label != "")
   {
      double xstep=(xmax-xmin)/10;
      double ystep=(ymax-ymin)/10;
      metastream << "textcolor "+colorfunc::get_colorstr(label_color) << endl;
      metastream << "textsize 1.5" << endl;
      metastream << "text "+stringfunc::number_to_string(xmin+xstep)+" "
         +stringfunc::number_to_string(ymax-(currplot+1)*ystep)+" '"
         +colored_curve_label+"'" << endl;
   }
}

// ---------------------------------------------------------------------

void metafile::closemetafile()
{
   filefunc::closefile(metafilename+".meta",metastream);
}

void metafile::appendmetafile()
{
   filefunc::appendfile(metafilename+".meta",metastream);
}

// ---------------------------------------------------------------------
int metafile::set_metastream_precision()
{
   const int nprecision=3;
   metastream.setf(ios::fixed);
   metastream.setf(ios::showpoint);  
   metastream.precision(nprecision);
   return nprecision;
}

// ---------------------------------------------------------------------
// Member function add_extralines inserts any manually added extra
// lines into the metastream before data is written out to the
// metafile:

void metafile::add_extralines()
{
   for (unsigned int i=0; i<extraline.size(); i++)
   {
      metastream << extraline[i] << endl;
   }
   metastream << endl;
}

// ==========================================================================
// Plotting member functions
// ==========================================================================

void metafile::write_curve(
   double Xstart, double Xstop, const vector<double>& Y)
{
   write_curve(Xstart, Xstop, Y, colorfunc::red);
}

void metafile::write_curve(
   double Xstart, double Xstop, const vector<double>& Y,
   colorfunc::Color curve_color)
{
   vector<double> X;
   int n_samples = Y.size();

   for(int i = 0; i < n_samples; i++)
   {
      double curr_X = Xstart + i * (Xstop - Xstart) / double(n_samples - 1);
      X.push_back(curr_X);
   }
   write_curve(X,Y,curve_color);
}

void metafile::write_curve(const vector<double>& X,const vector<double>& Y)
{
   write_curve(X,Y,colorfunc::red);
}

void metafile::write_curve(const vector<double>& X,const vector<double>& Y,
                           colorfunc::Color curve_color)
{
   metastream << endl;
   metastream << "curve color "+colorfunc::get_colorstr(curve_color)
              << endl;
   metastream << "thick " << get_thickness() << endl;

   if(legend_flag && legendlabel.size() > 0)
   {
      metastream << "label '" << legendlabel << "'" << endl;
   }

// As of 1/13/17, we restrict the total number of (X,Y) pairs exported 
// to output metafiles to be less than max_n_exported_points:

   const int max_n_exported_points = 100 * 1000;
   int iskip = 1;
   if(Y.size() >= X.size() && Y.size() >  max_n_exported_points)
   {
      iskip = Y.size() / max_n_exported_points + 1;
   }
   else if(X.size() > Y.size() && X.size() >  max_n_exported_points)
   {
      iskip = X.size() / max_n_exported_points + 1;
   }

// As of 1/5/17, we experiment with allowing X and Y to have different
// sizes.  But we assume that they are both functions of some common
// underlying parameter (e.g. time) whose span is the same for both
// arrays:

   if(X.size() == Y.size())
   {
      for (unsigned int i=0; i<Y.size(); i += iskip)
      {
         if(mathfunc::my_isnan(X[i]) || mathfunc::my_isnan(Y[i])) continue;
         metastream << X[i] << "\t" << Y[i] << endl;
      }
   }
   else if (X.size() > Y.size()) // Downsample X so that it has same size as Y
   {
      for (unsigned int i=0; i<Y.size(); i += iskip)
      {
         double frac = double(i) / (Y.size() - 1);
         int j_max = X.size() - 1;
         double j = frac * j_max;

         int j_lo = basic_math::mytruncate(j);
         int j_hi = basic_math::min(j_lo + 1, j_max);

         if(mathfunc::my_isnan(X[j_lo]) || mathfunc::my_isnan(X[j_hi])
            || mathfunc::my_isnan(Y[i])) continue;

         double alpha = j - j_lo;
         double Xinterp = alpha * X[j_lo] + (1 - alpha) * X[j_hi];
         metastream << Xinterp << "\t" << Y[i] << endl;
      } // loop over index i 
   }
   else if (X.size() < Y.size()) // Downsample Y so that it has same size as X
   {
      for (unsigned int i=0; i<X.size(); i += iskip)
      {
         double frac = double(i) / (X.size() - 1);
         int j_max = Y.size() - 1;
         double j = frac * j_max;

         int j_lo = basic_math::mytruncate(j);
         int j_hi = basic_math::min(j_lo + 1, j_max);

         if(mathfunc::my_isnan(X[i]) || mathfunc::my_isnan(Y[j_lo])
            || mathfunc::my_isnan(Y[j_hi])) continue;

         double alpha = j - j_lo;
         double Yinterp = alpha * Y[j_lo] + (1 - alpha) * Y[j_hi];
         metastream << X[i] << "\t" << Yinterp << endl;
      } // loop over index i 
   }
}

void metafile::write_markers(const std::vector<double>& X,
                             const std::vector<double>& Y)
{
   string marker_color="red";
   write_markers(marker_color,X,Y);
}


void metafile::write_markers(
   string marker_color,const vector<double>& X,
   const vector<double>& Y)
{
   metastream << "curve color "+marker_color << endl;
   metastream << "marks -1" << endl;
   metastream << "markerstyle 0" << endl;
   
   for (unsigned int i=0; i<X.size(); i++)
   {
      metastream << X[i] << "\t" << Y[i] << endl;
   }
   
}

void metafile::write_markers(
   const vector<int>& labels,const vector<double>& X,const vector<double>& Y)
{
   int min_label = 1000;
   int max_label = -1000;
   for (unsigned int i=0; i<labels.size(); i++)
   {
      min_label = basic_math::min(min_label, labels[i]);
      max_label = basic_math::max(max_label, labels[i]);
   }
   for (int curr_label=min_label; curr_label<=max_label; curr_label++)
   {
      string curr_color = colorfunc::getcolor(curr_label);
      metastream << "curve label \"" << curr_label << "\" color "
                 << curr_color << " mstyle 0 marks -1" << endl;
   }

   for (unsigned int i=0; i<labels.size(); i++)
   {
      string curr_color = colorfunc::getcolor(labels[i]);
      metastream << "curve marks -1 markerstyle 0 color "+curr_color+" ";
      metastream << X[i] << "\t" << Y[i] << endl;
   }
   
}

