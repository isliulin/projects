// =========================================================================
// 
// =========================================================================
// Last updated on 8/31/09
// =========================================================================

// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine


#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/simple_point.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <boost/random/linear_congruential.hpp>
#include <boost/progress.hpp>
#include <boost/shared_ptr.hpp>

#include "general/filefuncs.h"
#include "general/stringfuncs.h"

using namespace boost;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

void usage()
{
   cerr << "Usage: fr_layout [options] <width> <height>\n"
        << "Arguments:\n"
        << "\t<width>\tWidth of the display area (floating point)\n"
        << "\t<Height>\tHeight of the display area (floating point)\n\n"
        << "Options:\n"
        << "\t--iterations n\tNumber of iterations to execute.\n" 
        << "\t\t\tThe default value is 100.\n"
        << "Input:\n"
        << "  Input is read from standard input as a list of edges, one per line.\n"
        << "  Each edge contains two string labels (the endpoints) separated by a space.\n\n"
        << "Output:\n"
        << "  Vertices and their positions are written to standard output with the label,\n  x-position, and y-position of a vertex on each line, separated by spaces.\n";
}

typedef adjacency_list<listS, vecS, undirectedS, 
                       property<vertex_name_t, string> > Graph;

typedef graph_traits<Graph>::vertex_descriptor Vertex;

typedef map<string, Vertex> NameToVertex;

Vertex get_vertex(const string& name, Graph& g, NameToVertex& names)
{
   NameToVertex::iterator i = names.find(name);
   if (i == names.end())
      i = names.insert(make_pair(name, add_vertex(name, g))).first;
   return i->second;
}

class progress_cooling : public linear_cooling<double>
{
   typedef linear_cooling<double> inherited;

  public:
   explicit progress_cooling(size_t iterations) : inherited(iterations) 
      {
         display.reset(new progress_display(iterations + 1, cerr));
      }

   double operator()()
      {
         ++(*display);
         return inherited::operator()();
      }

  private:
   shared_ptr<boost::progress_display> display;
};

int main(int argc, char* argv[])
{
   int iterations = 100;

   if (argc < 3) { usage(); return -1; }

   double width = 0;
   double height = 0;

   for (int arg_idx = 1; arg_idx < argc; ++arg_idx) {
      string arg = argv[arg_idx];
      if (arg == "--iterations") {
         ++arg_idx;
         if (arg_idx >= argc) { usage(); return -1; }
         iterations = lexical_cast<int>(argv[arg_idx]);
      } else {
         if (width == 0.0) width = lexical_cast<double>(arg);
         else if (height == 0.0) height = lexical_cast<double>(arg);
         else {
            usage();
            return -1;
         }
      }
   }

   if (width == 0.0 || height == 0.0) {
      usage();
      return -1;
   }

   Graph g;
   NameToVertex names;

   string input_filename="MIT_connected.pairs";
   cout << "Enter name for file containing input graph edge info:" << endl;
   cin >> input_filename;
   filefunc::ReadInfile(input_filename);
   for (int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i]);
      string source=substrings[0];
      string target=substrings[1];
      add_edge(get_vertex(source, g, names), get_vertex(target, g, names), g);
   }
   
   typedef vector<simple_point<double> > PositionVec;
   PositionVec position_vec(num_vertices(g));
   typedef iterator_property_map<PositionVec::iterator, 
      property_map<Graph, vertex_index_t>::type>
      PositionMap;
   PositionMap position(position_vec.begin(), get(vertex_index, g));

   minstd_rand gen;
   random_graph_layout(
      g, position, -width/2, width/2, -height/2, height/2, gen);
   fruchterman_reingold_force_directed_layout
      (g, position, width, height,
       cooling(progress_cooling(iterations)));

   graph_traits<Graph>::vertex_iterator vi, vi_end;

   string output_filename="layout.output";
   ofstream outstream;
   filefunc::openfile(output_filename,outstream);
   for (tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) 
   {
      outstream  << get(vertex_name, g, *vi) << '\t'
                 << position[*vi].x << '\t' << position[*vi].y << endl;
   }
   filefunc::closefile(output_filename,outstream);

}
