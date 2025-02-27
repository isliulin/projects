// ==========================================================================
// Header file for maze class 
// ==========================================================================
// Last modified on 11/15/16; 11/17/16; 11/18/16; 12/8/16
// ==========================================================================

#ifndef MAZE_H
#define MAZE_H

#include <stack>
#include <utility>
#include <vector>
#include "math/genmatrix.h"
#include "math/genvector.h"
#include "math/ltduple.h"
#include "image/TwoDarray.h"

class maze
{
   
  public:

   typedef std::map<std::string, double > Q_MAP;
// independent string: string rep for state + action
// dependent double: Q(s,a)

   typedef std::map<int, std::pair<double, int> > MAX_Q_MAP;
// independent int: turtle cell
// dependent pair: double = qvalue, int = action direction

   typedef std::map<int, int> PROBLEM_CELLS_MAP;
// independent int: turtle cell ID
// dependent int: -2 --> illegal action, -1 --> contradictory neighbor action

// Initialization, constructor and destructor functions:

   maze(int n_size);
   maze(const maze& C);
   ~maze();
//   maze operator= (const maze& C);
   friend std::ostream& operator<< 
      (std::ostream& outstream,const maze& C);

   int get_n_cells() const;
   int get_n_problem_cells() const;
   void set_turtle_cell(int t);
   int get_turtle_cell() const;
   int get_turtle_cell(int tx, int ty) const;
   void decompose_turtle_cell(int turtle_cell, int& tx, int& ty);

   DUPLE getDirection(int curr_dir);
   bool IsDirValid(int px, int py, int curr_dir);
   int get_neighbor(int p, int curr_dir);
   std::vector<int> get_cell_neighbors(int p);
   std::vector<int> get_unvisited_neighbors(int p);
   int n_visited_cells() const;
   void init_grid();
   void print_grid() const;
   void print_visited_cells() const;
   void print_visited_cell_stack() const;
   void set_solution_path();
   const std::vector<int>& get_solution_path() const;
   int get_solution_path_moves() const;
   void print_solution_path() const;

   void generate_maze();
   void solve_maze_backwards();
   void find_neighbor_cell_directions(int p);
   void print_soln_grid() const;

// Drawing member functions:

   void DrawLine(unsigned char* img, int x1, int y1, int x2, int y2,
                 int R, int G, int B);

   void DrawPoint(unsigned char* img, const twovector& V, int R, int G, int B);
   void DrawLine(unsigned char* img, twovector& V1, twovector& V2,
                 int R, int G, int B);
   void DrawArrow(unsigned char* img, twovector& base, twovector& tip,
                  int R, int G, int B);
   void DrawCellArrow(unsigned char* img, int px, int py, int direction,
                      int R, int G, int B);
   void DrawCellX(unsigned char* img, int px, int py, 
                  int R, int G, int B);
   void FillCell(unsigned char* img, int px, int py, int R, int G, int B);
   void RenderMaze(unsigned char* img);
   void SaveBMP(std::string FileName, const void* RawBGRImage, 
                int Width, int Height);
   void DisplayTrainedZerothLayerWeights(std::string output_subdir);
   void DrawMaze(int counter, std::string output_subdir, std::string basename, 
                 bool display_qmap_flag, twoDarray* wtwoDarray_ptr = NULL);
   void append_wtwoDarray(twoDarray* wtwoDarray_ptr);

   void initialize_occupancy_grid();
   void print_occupancy_grid() const;
   void print_occupancy_state() const;
   std::string occupancy_state_to_string();
   genvector* get_curr_legal_actions();
   genvector* get_occupancy_state();
   
   void generate_all_turtle_states();
   void reset_game(bool random_turtle_start);
   void set_game_over(bool flag);
   bool get_game_over() const;
   bool get_maze_solved() const;
   genvector* compute_legal_turtle_actions();
   bool legal_turtle_move(int curr_dir);
   bool legal_turtle_move(int tx, int ty, int curr_dir);
   int move_turtle(int curr_dir, bool erase_turtle_path);
   int get_n_soln_steps() const;
   void print_turtle_path_history() const;
   int get_n_turtle_moves() const;
   bool previously_visited_occupancy_cell() const;

   int compute_turtle_reward() const;

   std::vector<genvector*>& get_curr_maze_states();
   const std::vector<genvector*>& get_curr_maze_states() const;
   std::vector<std::string>& get_curr_maze_state_strings();
   const std::vector<std::string>& get_curr_maze_state_strings() const;

   void set_qmap_ptr(Q_MAP *qmap_ptr);
   void compute_max_Qmap();
   double score_max_Qmap();
   void identify_max_Qmap_problems();
   void draw_max_Qmap(unsigned char* img);
   void color_cells(unsigned char* img, twoDarray* wtwoDarray_ptr);

   int is_problem_cell(int turtle_p);


  private: 

   bool random_turtle_start;
   int n_size, n_cells;
   int n_directions, nbits;
   int ImageSize;
   int turtle_cell;
   int turtle_p_start, turtle_px_start, turtle_py_start;
   double turtle_value, wall_value;
   bool game_over;
   std::vector<int> direction;
   genmatrix *grid_ptr;
   genmatrix *grid_problems_ptr;
   genmatrix *soln_grid_ptr;

   std::vector<bool> visited_cell;
	// independent int = cell ID; dependent bool = visited flag

   std::vector<int> visited_cell_stack;

   std::vector<DUPLE> cell_decomposition;
// independent int: p
// dependent triple: px, py

   std::vector<int> soln_path;

   genvector *curr_legal_actions;
   genmatrix *occupancy_grid;
   genvector *occupancy_state;
   genmatrix *visited_turtle_cells;

   std::vector<DUPLE> occupancy_cell_decomposition;
// independent int: p
// dependent triple: px, py

   std::vector<int> turtle_path_history;

// curr_maze_states holds all possible turtle states for current maze
// as genvectors.  

   std::vector<genvector*> curr_maze_states;

// curr_maze_state_strings holds all possible turtle
// states as strings.

   std::vector<std::string> curr_maze_state_strings;

   Q_MAP *qmap_ptr;
   Q_MAP::iterator qmap_iter;

   MAX_Q_MAP max_qmap;
   MAX_Q_MAP::iterator max_qmap_iter;

// Store IDs for cells containing actions which are either illegal or
// pairwise contradictory:

   std::vector<int> current_problem_cells;
   PROBLEM_CELLS_MAP problem_cells_map;
   PROBLEM_CELLS_MAP::iterator problem_cells_iter;

// wtwoDarray_ptrs holds pointers to twoDarrays which contain
// renormalized trained weight values:

   std::vector<twoDarray*> wtwoDarray_ptrs;

   void allocate_member_objects();
   void initialize_member_objects();
   void docopy(const maze& T);

   int get_cell(int px, int py) const;
   int get_occupancy_cell(int px, int py) const;
   std::string get_cell_bitstr(int px, int py);
   int get_direction_from_p_to_q(int p, int q);
   void remove_wall(int p, int curr_dir);
   bool wall_exists(int p, int curr_dir);
   void initialize_occupancy_state();   
};

// ==========================================================================
// Inlined methods:
// ==========================================================================

// Set and get member functions:

inline int maze::get_n_cells() const
{
   return n_cells;
}

inline int maze::get_n_problem_cells() const
{
   return problem_cells_map.size();
}

inline int maze::get_cell(int px, int py) const
{
   return n_size * py + px;
}

inline int maze::get_occupancy_cell(int px, int py) const
{
   return occupancy_grid->get_ndim() * py + px;
}

inline genvector* maze::get_curr_legal_actions()
{
   return curr_legal_actions;
}

inline genvector* maze::get_occupancy_state()
{
   return occupancy_state;
}

inline void maze::set_turtle_cell(int t)
{
   turtle_cell = t;
}

inline int maze::get_turtle_cell() const
{
   return turtle_cell;
}

inline int maze::get_turtle_cell(int tx, int ty) const
{
   return ty * (2 * n_size - 1) + tx;
}

inline void maze::decompose_turtle_cell(int turtle_cell, int& tx, int& ty)
{
   ty = turtle_cell / (2*n_size - 1);
   tx = turtle_cell % (2*n_size - 1);
}

inline std::vector<genvector*>& maze::get_curr_maze_states() 
{
   return curr_maze_states;
}

inline const std::vector<genvector*>& maze::get_curr_maze_states() const
{
   return curr_maze_states;
}

inline std::vector<std::string>& maze::get_curr_maze_state_strings() 
{
   return curr_maze_state_strings;
}

inline const std::vector<std::string>& maze::get_curr_maze_state_strings() const
{
   return curr_maze_state_strings;
}

inline void maze::set_qmap_ptr(Q_MAP *qmap_ptr)
{
   this->qmap_ptr = qmap_ptr;
}

inline void maze::append_wtwoDarray(twoDarray* wtwoDarray_ptr)
{
   wtwoDarray_ptrs.push_back(wtwoDarray_ptr);
}

#endif  // maze.h


