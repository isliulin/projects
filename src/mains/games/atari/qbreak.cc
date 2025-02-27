// ==========================================================================
// Program QBREAK solves the BreakOut atari game via deep Q-learning.
// ==========================================================================
// Last updated on 12/31/16; 1/10/17; 1/18/17; 1/24/17
// ==========================================================================

// Note: On 12/17/16, we learned the hard and painful way that left
// and right actions continue to move the paddle even after it's hit a
// sidewall.  So the paddle's position can have values exceeding the
// visible part of the game board.  In these cases, the paddle appears
// to be "pinned" to the side wall.  

#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <SDL.h>
#include <ale_interface.hpp>
#include "games/breakout.h"
#include "machine_learning/environment.h"
#include "general/filefuncs.h"
#include "machine_learning/machinelearningfuncs.h"
#include "numrec/nrfuncs.h"
#include "machine_learning/reinforce.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"
#include "video/videofuncs.h"

int main(int argc, char** argv) 
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::ofstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);      

   timefunc::initialize_timeofday_clock();
   long seed = nrfunc::init_time_based_seed();
//   long seed = -11;
//   cout << "Enter negative seed:" << endl;
//   cin >> seed;
//   nrfunc::init_default_seed(seed);

// Instantiate Breakout ALE game:

   int n_screen_states = 1;
//   int n_screen_states = 2;
//   int n_screen_states = 3;
   breakout *breakout_ptr = new breakout(n_screen_states);
   int n_actions = breakout_ptr->get_n_actions();

   breakout_ptr->set_compute_difference_flag(true);
   breakout_ptr->set_compute_max_flag(false);
   
// Disable ALE's random responsiveness to input actions:

   breakout_ptr->get_ale().setFloat("repeat_action_probability",0);

// Construct environment which acts as interface between reinforcement
// agent and particular game:

   environment game_world(environment::BREAKOUT);
   game_world.set_breakout(breakout_ptr);

   bool use_big_states_flag = false;
   if(n_screen_states > 1)
   {
      use_big_states_flag = true;
   }
   game_world.set_use_big_states_flag(use_big_states_flag);
   game_world.set_frame_skip(1);
//   game_world.set_frame_skip(3);

// Set neural network architecture parameters:

   int Din = game_world.get_curr_state()->get_mdim();   // Input layer dim
   cout << "Din = " << Din << endl;
   int Dout = n_actions;

//   int H1 = 8;
//   int H1 = 16;
//   int H1 = 32;
//   int H1 = 64;
   int H1 = 128;

//   int H2 = 0;
//   int H2 = 8;
//   int H2 = 16;
   int H2 = 32;
//   int H2 = 64;
//   int H2 = 128;

   int H3 = 0;
//   int H3 = 16;
//   int H3 = 32;
//   int H3 = 64;

   vector<int> layer_dims;
   layer_dims.push_back(Din);
   layer_dims.push_back(H1);
   if(H2 > 0)
   {
      layer_dims.push_back(H2);
   }
   if(H3 > 0)
   {
      layer_dims.push_back(H3);
   }
   layer_dims.push_back(Dout);

// Construct reinforcement learning agent:

   bool include_biases = false;
   int nframes_per_epoch = 50 * 1000;
   int n_max_epochs = 3000;
   
   int replay_memory_capacity = nframes_per_epoch * 4;
//   int replay_memory_capacity = nframes_per_epoch * 8;
   int eval_memory_capacity = basic_math::min(
      int(0.1 * replay_memory_capacity), 20000);

   reinforce* reinforce_agent_ptr = new reinforce(
      include_biases, layer_dims, 1, replay_memory_capacity, 
      eval_memory_capacity, reinforce::RMSPROP);

//   reinforce_agent_ptr->set_debug_flag(true);
   reinforce_agent_ptr->set_environment(&game_world);

//   reinforce_agent_ptr->set_lambda(0.0);
   reinforce_agent_ptr->set_lambda(1E-2);
//   reinforce_agent_ptr->set_lambda(1E-3);
//   machinelearning_func::set_leaky_ReLU_small_slope(0.00); 
   machinelearning_func::set_leaky_ReLU_small_slope(0.01); 
//   machinelearning_func::set_leaky_ReLU_small_slope(0.001); 

// Initialize output subdirectory within an experiments folder:

   string experiments_subdir="./experiments/breakout/";
   filefunc::dircreate(experiments_subdir);

   int expt_number;
   cout << "Enter experiment number:" << endl;
   cin >> expt_number;
   string output_subdir=experiments_subdir+
      "expt"+stringfunc::integer_to_string(expt_number,3)+"/";
   filefunc::dircreate(output_subdir);
   reinforce_agent_ptr->set_output_subdir(output_subdir);

   string weights_subdir = output_subdir+"zeroth_layer_weights/";
   filefunc::dircreate(weights_subdir);
   string screen_exports_subdir = output_subdir+"screen_exports/";
   filefunc::dircreate(screen_exports_subdir);
   breakout_ptr->set_screen_exports_subdir(screen_exports_subdir);

//   reinforce_agent_ptr->set_Nd(16);  // # samples to be drawn from replay mem
   //   reinforce_agent_ptr->set_Nd(24);  // # samples to be drawn from replay mem
   reinforce_agent_ptr->set_Nd(32);  // # samples to be drawn from replay mem
   reinforce_agent_ptr->set_gamma(0.99); // discount reward factor
//   reinforce_agent_ptr->set_gamma(0.95); // discount reward factor
   reinforce_agent_ptr->set_rmsprop_decay_rate(0.90);
//   reinforce_agent_ptr->set_rmsprop_decay_rate(0.95);

//   reinforce_agent_ptr->set_base_learning_rate(1E-2);
//   reinforce_agent_ptr->set_base_learning_rate(3E-3);
   reinforce_agent_ptr->set_base_learning_rate(1E-3);
//   reinforce_agent_ptr->set_base_learning_rate(3E-4);  
//   reinforce_agent_ptr->set_base_learning_rate(2.5E-4);  

   double min_epsilon = 0.10;
   reinforce_agent_ptr->set_min_epsilon(min_epsilon);
   double starting_epoch_linear_eps_decay = 4.0;
   double stopping_epoch_linear_eps_decay = 0.35 * n_max_epochs;

   int n_lr_episodes_period = 10 * 1000;

   int nn_update_frame_period = 1;
//   int nn_update_frame_period = 10;
//   int nn_update_frame_period = 25;
   int old_weights_period_in_epochs = 2;    

// Fraction of zero-reward (S,A,R,S') states to NOT include within
// replay memory:

   const double discard_0_reward_frac = 0.0;  
//   const double discard_0_reward_frac = 0.60;  
//   const double discard_0_reward_frac = 0.75;  
//   const double discard_0_reward_frac = 0.85;  

//    int n_snapshot = 500;
   int n_episode_update = 100;

   string extrainfo="H1="+stringfunc::number_to_string(H1);
   if(H2 > 0)
   {
      extrainfo += ";H2="+stringfunc::number_to_string(H2);
   }
   if(H3 > 0)
   {
      extrainfo += ";H3="+stringfunc::number_to_string(H3);
   }

   double total_loss = -1;

   bool export_frames_flag = false;
//   bool export_frames_flag = true;

   // Set vector of minimal legal actions:

   ActionVect minimal_actions;
   if(n_actions >= 3)
   {
      minimal_actions.push_back(PLAYER_A_NOOP);  
   }
   minimal_actions.push_back(PLAYER_A_RIGHT);
   minimal_actions.push_back(PLAYER_A_LEFT);

// Generate text file summary of parameter values:

   reinforce_agent_ptr->summarize_parameters();
   ofstream params_stream;
   filefunc::appendfile(
      reinforce_agent_ptr->get_params_filename(), params_stream);

   params_stream << "n_actions = " << n_actions << endl;
   params_stream << "Leaky ReLU small slope = "
                 << machinelearning_func::get_leaky_ReLU_small_slope() << endl;
   params_stream << "Learning rate decrease period = " 
                 << n_lr_episodes_period << " episodes" << endl;
   params_stream << "Old weights period = " << old_weights_period_in_epochs 
                 << " epochs" << endl;
   params_stream << "Discard zero reward frac = " 
                 << discard_0_reward_frac << endl;
   params_stream << "Use big states flag = " << use_big_states_flag << endl;
   params_stream << "Frame skip = " << game_world.get_frame_skip() << endl;
   params_stream << "1 big state = n_screen_states = "
                 << breakout_ptr->get_n_screen_states() << endl;
   if(breakout_ptr->get_n_screen_states() == 1)
   {
      params_stream << "  Compute difference flag = "
                    << breakout_ptr->get_compute_difference_flag()
                    << endl;
      params_stream << "  Compute max flag = "
                    << breakout_ptr->get_compute_max_flag()
                    << endl;
   }

   params_stream << "Starting epoch for linear epsilon decay = "
                 << starting_epoch_linear_eps_decay << endl;
   params_stream << "Stopping epoch for linear epsilon decay = "
                 << stopping_epoch_linear_eps_decay << endl;
   params_stream << "nn_update_frame_period = "
                 << nn_update_frame_period << endl;
   params_stream << "nframes / epoch = " << nframes_per_epoch << endl;
   params_stream << "n_max_epochs = " << n_max_epochs << endl;
   params_stream << "Random seed = " << seed << endl;
   params_stream << "Process ID = " << getpid() << endl;
   filefunc::closefile(
      reinforce_agent_ptr->get_params_filename(), params_stream);

// ==========================================================================
// Reinforcement training loop starts here

   const int n_fire_ball_frames = 2;
   int cum_framenumber = 0;
   bool eval_memory_full_flag = false;

   while(reinforce_agent_ptr->get_curr_epoch() < n_max_epochs)
   {
      double curr_epoch = cum_framenumber / nframes_per_epoch;
      reinforce_agent_ptr->set_curr_epoch(curr_epoch);
      int curr_episode_number = reinforce_agent_ptr->get_episode_number();

      bool random_start = false;
      game_world.start_new_episode(random_start);
      reinforce_agent_ptr->initialize_episode();

// Periodically decrease learning rate:

      if(curr_episode_number > 0 && 
         curr_episode_number%n_lr_episodes_period == 0)
      {
         reinforce_agent_ptr->decrease_learning_rate();
      }

// -----------------------------------------------------------------------
// Current episode starts here:

      cout << "************  Start of episode " << curr_episode_number
           << " for expt " << expt_number << " ***********" << endl;

      int d = -1, n_state_updates = 0;
      int prev_a = 0;
      int curr_a = -1;
      Action a;
      int n_prev_lives = -1;
      double cum_reward = 0;

// As of 12/17/16, we reset paddle's starting position to be at the
// gameboard's horizontal center:

      breakout_ptr->set_paddle_x(
         breakout_ptr->get_default_starting_paddle_x());
      int n_recenter_paddle_frames = 
         breakout_ptr->get_default_starting_paddle_x()  
         - breakout_ptr->get_center_paddle_x();

// On 12/16/16, we discovered the hard way that the Arcade Learning
// Environment's getEpisodeFrameNumber() method does NOT always return
// contiguous increasing integers!  So we no longer use the following
// line:

//  int curr_frame_number = game_world.get_episode_framenumber();

      int curr_episode_framenumber = 0;  // since start of current episode
      int curr_life_framenumber = 0;  // n_frames since start of current life

      while(!game_world.get_game_over())
      {
         int n_curr_lives = breakout_ptr->get_ale().lives();
         bool state_updated_flag = false;
         bool zero_input_state = false;

         curr_episode_framenumber++;
         curr_life_framenumber++;
         cum_framenumber++;
         curr_epoch = double(cum_framenumber) / nframes_per_epoch;
         reinforce_agent_ptr->set_curr_epoch(curr_epoch);

         if(cum_framenumber % game_world.get_frame_skip() == 0)
         {
            if(use_big_states_flag)
            {
               if(breakout_ptr->crop_pool_curr_frame(export_frames_flag))
               {
                  state_updated_flag = true;
                  n_state_updates++;
                  breakout_ptr->update_curr_big_state();
               }
            }
            else
            {
               if(breakout_ptr->get_compute_difference_flag())
               {
                  if(breakout_ptr->crop_pool_difference_curr_frame(
                        export_frames_flag))
                  {
                     state_updated_flag = true;
                     n_state_updates++;
                  }
               }
               else if (breakout_ptr->get_compute_max_flag())
               {
                  if(breakout_ptr->crop_pool_sum_curr_frame(
                        export_frames_flag))
                  {
                     state_updated_flag = true;
                     n_state_updates++;
                  }
               }
            }
         } // cum_framenumber % frame_skip == 0 conditional

         if(state_updated_flag && n_state_updates > 2)
         {
            genvector* curr_s = NULL;
            if(use_big_states_flag)
            {
               curr_s = breakout_ptr->get_curr_big_state();
            }
            else
            {
               curr_s = game_world.get_curr_state();
            }

// If curr_s == 0, do NOT store it into the replay memory:

            if(curr_s->magnitude() <= 0)
            {
               zero_input_state = true;
//               cout << " Zero input state detected" << endl;
            }
            else
            {

// Fill evaluation memory with randomly generated initial states:

               if(nrfunc::ran1() < 0.2)
               {
                  eval_memory_full_flag = 
                     reinforce_agent_ptr->store_curr_state_into_eval_memory(
                        *curr_s);
               }
               
               if(eval_memory_full_flag)
               {
                  d = reinforce_agent_ptr->store_curr_state_into_replay_memory(
                     *curr_s);
                  if(d%1000 == 0) 
                     cout << "Replay memory index d = " << d << endl;
               }
            }            
         } // state_updated_flag && n_state_updates > 2 conditional

// First reposition paddle so that it starts at screen's horizontal center:

         if(curr_life_framenumber < n_recenter_paddle_frames)
         {
            a = PLAYER_A_LEFT;  // move paddle towards center
         }

// Next fire ball:

         else if(curr_life_framenumber < 
                 n_recenter_paddle_frames + n_fire_ball_frames)
         {
            a = PLAYER_A_FIRE;  // fire ball
         }

// Now start playing game:

         else
         {
            curr_a = prev_a;
            if(state_updated_flag)
            {
               curr_a = reinforce_agent_ptr->select_action_for_curr_state();
               prev_a = curr_a;
            }
            a = minimal_actions[curr_a];
         }

// As of 12/18/16 we do not not allow the paddle to move beyond the
// right or left walls:
         if(a == PLAYER_A_RIGHT)
         {
            if(!breakout_ptr->increment_paddle_x())
            {
               a = PLAYER_A_NOOP;
            }
         }
         else if (a == PLAYER_A_LEFT)
         {
            if(!breakout_ptr->decrement_paddle_x())
            {
               a = PLAYER_A_NOOP;
            }
         }
         breakout_ptr->push_back_paddle_x();

         double curr_reward = breakout_ptr->get_ale().act(a);
         cum_reward += curr_reward;

         double renorm_reward = 0;
         if(curr_reward > 0)
         {
            renorm_reward = 1;
         }

         if(n_curr_lives != n_prev_lives)
         {

/*
// As of 6 pm on Mon Dec 19, 2016, we experiment with treating
// end-of-life as end-of-episode:

            if(n_prev_lives > 0)
            {
               game_world.set_game_over(true);
            }
*/

/*

// As of noontime on Mon Dec 19, 2016 we experiment with NOT
// penalizing the agent when it misses the ball.  Q should then only
// increase.


// Penalize agent whenever it misses the ball:

            if(n_prev_lives > 0)
            {
               renorm_reward = -1;
            }
*/

            n_prev_lives = n_curr_lives;
            curr_life_framenumber = 0;
//            cout << "n_curr_lives = " << n_curr_lives
//                 << " n_prev_lives = " << n_prev_lives
//                 << " renorm_reward = " << renorm_reward << endl;
         }

         if(d >= 0 && game_world.get_game_over())
         {
            reinforce_agent_ptr->store_final_arsprime_into_replay_memory(
               d, curr_a, renorm_reward);
         }
         else if (d >= 0 && state_updated_flag && n_state_updates > 2 && 
                  curr_a >= 0 && !zero_input_state)
         {
            genvector* next_s = game_world.compute_next_state(a);

// As of 6:45 am on Mon Dec 5, we experiment with discarding some
// fraction of zero reward states in order to increase chances of
// agent seeing non-zero reward states without having to perform a
// huge number of expensive backpropagations:

            bool ignore_curr_state = false;
            if(nearly_equal(renorm_reward, 0))
            {
               if(nrfunc::ran1() < discard_0_reward_frac)
               {
                  ignore_curr_state = true;
               }
            }

            if(!ignore_curr_state)
            {
               reinforce_agent_ptr->store_arsprime_into_replay_memory(
                  d, curr_a, renorm_reward, *next_s, 
                  game_world.get_game_over());
            }
         }

         if(reinforce_agent_ptr->get_replay_memory_full() &&
            cum_framenumber % nn_update_frame_period == 0)
         {
            bool verbose_flag = false;
            if(curr_episode_number % 100 == 0)
            {
               verbose_flag = true;
            }
            total_loss = reinforce_agent_ptr->update_Q_network(
               verbose_flag);
         }

// Periodically save an episode's worth of screens to output
// subdirectory:

         bool export_RGB_screens_flag = false;
         if(curr_episode_number % 1000 == 0) export_RGB_screens_flag = true;

         if(export_RGB_screens_flag)
         {
            string curr_screen_filename="screen_"+
               stringfunc::integer_to_string(curr_episode_number,5)+"_"+
               stringfunc::integer_to_string(curr_episode_framenumber,5)+
               ".png";
            breakout_ptr->save_screen(
               curr_episode_number, curr_screen_filename);

/*
            string caption;
            if(a == PLAYER_A_NOOP)
            {
               caption = "No OP";
            }
            else if(a == PLAYER_A_RIGHT)
            {
               caption = "Move RIGHT";
            }
            else if(a == PLAYER_A_LEFT)
            {
               caption = "Move LEFT";
            }
            else if (a == PLAYER_A_FIRE)
            {
               caption = "Fire ball";
            }
            caption += "; paddle X = "+stringfunc::number_to_string(
               breakout_ptr->get_paddle_x());
            breakout_ptr->save_screen(
               curr_episode_number, curr_screen_filename, caption);
*/

         }
      } // game_over while loop

// -----------------------------------------------------------------------

//      breakout_ptr->set_forced_game_over(false);
//      breakout_ptr->get_ale().reset_game();

      cout << "Episode finished" << endl;
      cout << "  epoch = " << curr_epoch 
           << "  cum_frame = " << cum_framenumber << endl;
      cout << "  cum_reward = " << cum_reward 
           << "  epsilon = " << reinforce_agent_ptr->get_epsilon() 
           << "  n_backprops = " 
           << reinforce_agent_ptr->get_n_backprops() << endl;

      reinforce_agent_ptr->update_episode_history();
      reinforce_agent_ptr->update_epoch_history();
      reinforce_agent_ptr->update_n_frames_per_episode(
         curr_episode_framenumber);
      reinforce_agent_ptr->update_cumulative_reward(cum_reward);
      reinforce_agent_ptr->update_epsilon();

      if(cum_framenumber % 10 == 0)
      {
         reinforce_agent_ptr->compute_max_eval_Qvalues_distribution();
      }

      if(total_loss > 0)
      {
         reinforce_agent_ptr->push_back_log10_loss(log10(total_loss));
      }

// Periodically copy current weights into old weights:

      int n_oldweight_frames = old_weights_period_in_epochs * 
         nframes_per_epoch;
      if(cum_framenumber % n_oldweight_frames == 0)
      {
         reinforce_agent_ptr->copy_weights_onto_old_weights();
      }

/*
      if(reinforce_agent_ptr->get_replay_memory_full())
      {
         bool verbose_flag = false;
         if(curr_episode_number % 10 == 0)
         {
            verbose_flag = true;
         }
         total_loss = reinforce_agent_ptr->update_Q_network(
            verbose_flag);
      }
*/

// Slowly decay epsilon over time:

      reinforce_agent_ptr->linearly_decay_epsilon(
         curr_epoch, starting_epoch_linear_eps_decay,
         stopping_epoch_linear_eps_decay);
      
// Periodically export status info:

      if(curr_episode_number >= n_episode_update - 1 && 
         curr_episode_number % n_episode_update == 0)
      {
         outputfunc::print_elapsed_time();
         if(reinforce_agent_ptr->get_include_biases()){
            reinforce_agent_ptr->compute_bias_distributions();
         }
         reinforce_agent_ptr->push_back_learning_rate(
            reinforce_agent_ptr->get_learning_rate());
         reinforce_agent_ptr->compute_weight_distributions();
         reinforce_agent_ptr->store_quasirandom_weight_values();
         reinforce_agent_ptr->generate_summary_plots();
         reinforce_agent_ptr->generate_view_metrics_script(false, true);
         breakout_ptr->plot_paddle_x_dist(output_subdir, extrainfo);

// Export trained weights in neural network's zeroth layer as
// colored images to output_subdir

         int n_reduced_xdim = breakout_ptr->get_n_reduced_xdim();
         int n_reduced_ydim = breakout_ptr->get_n_reduced_ydim();
         if(use_big_states_flag)
         {
            n_reduced_ydim *= n_screen_states;
         }
         reinforce_agent_ptr->plot_zeroth_layer_weights(
            n_reduced_xdim, n_reduced_ydim);
      }

/*
      if(curr_episode_number > 0 && curr_episode_number % n_snapshot == 0)
      {
         n_snapshot *= 2;
         reinforce_agent_ptr->export_snapshot();
      }
*/

   } // curr_epoch < n_max_epochs while loop

// Reinforcement training loop ends here
// ==========================================================================

   delete reinforce_agent_ptr;
   delete breakout_ptr;
}

