// ==========================================================================
// Program TRAIN_TTT_NETWORK trains a neural network via V-learning.
// ==========================================================================
// Last updated on 12/13/16; 1/10/17; 1/18/17; 1/24/17
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "machine_learning/environment.h"
#include "general/filefuncs.h"
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "machine_learning/reinforce.h"
#include "general/stringfuncs.h"
#include "games/tictac3d.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

// ==========================================================================
void compute_minimax_move(
   bool AI_moves_first, tictac3d* ttt_ptr, int AI_value)
{
   int best_move = ttt_ptr->imminent_win_or_loss(AI_value);
   if(best_move < 0)
   {
      best_move = ttt_ptr->get_recursive_minimax_move(AI_value);
   }
   ttt_ptr->set_player_move(best_move, AI_value);

// Periodically diminish relative differences between intrinsic cell
// prizes as game progresses:

   int n_completed_turns = ttt_ptr->get_n_completed_turns();
   if(n_completed_turns == 1)
   {
      ttt_ptr->adjust_intrinsic_cell_prizes();
   }
   else if (n_completed_turns == 2)
   {
      ttt_ptr->adjust_intrinsic_cell_prizes();
   }
   else if (n_completed_turns == 3)
   {
      ttt_ptr->adjust_intrinsic_cell_prizes();
   }
}

// ==========================================================================
int main (int argc, char* argv[])
{
   timefunc::initialize_timeofday_clock();
//   nrfunc::init_time_based_seed();
   long s = -11;
//   cout << "Enter negative seed:" << endl;
//   cin >> s;
   nrfunc::init_default_seed(s);

   int nsize = 4;
   int n_zlevels = 4;
   tictac3d* ttt_ptr = new tictac3d(nsize, n_zlevels);
   int n_actions = nsize * nsize * n_zlevels;
   int n_max_turns = n_actions;

// Construct environment which acts as interface between reinforcement
// agent and particular game:

   environment game_world(environment::TTT);
   game_world.set_tictac3d(ttt_ptr);

   int Din = nsize * nsize * n_zlevels;	// Input dimensionality
   int Dout = 1;			// Output dimensionality

//   int H1 = 3 * 64;	//  
   int H1 = 5 * 64;	//  = 320
//   int H1 = 7 * 64;	//  

//   int H2 = 0;
   int H2 = 32;
//   int H2 = 1 * 64;

   int H3 = 0;
//   int H3 = 32;

   string extrainfo="H1="+stringfunc::number_to_string(H1);
   if(H2 > 0)
   {
      extrainfo += "; H2="+stringfunc::number_to_string(H2);
   }
   if(H3 > 0)
   {
      extrainfo += "; H3="+stringfunc::number_to_string(H3);
   }
   extrainfo += "; zlevels="+stringfunc::number_to_string(n_zlevels);
   extrainfo += "; V-learning";
   
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

   int batch_size = 1;
//   int batch_size = 3;
   int replay_memory_capacity = 10 * batch_size * n_max_turns;
   bool include_biases = true;
   reinforce* reinforce_agent_ptr = new reinforce(
      include_biases, layer_dims, batch_size, replay_memory_capacity,
      reinforce::SGD);
   reinforce_agent_ptr->set_environment(&game_world);
   reinforce_agent_ptr->set_n_actions(n_actions);

// Initialize output subdirectory within an experiments folder:

   string experiments_subdir="./experiments/TTT/";
   filefunc::dircreate(experiments_subdir);

   int expt_number;
   cout << "Enter experiment number:" << endl;
   cin >> expt_number;
   string output_subdir=experiments_subdir+
      "expt"+stringfunc::integer_to_string(expt_number,3)+"/";
   filefunc::dircreate(output_subdir);
   reinforce_agent_ptr->set_output_subdir(output_subdir);

   reinforce_agent_ptr->set_Nd(32);
   reinforce_agent_ptr->set_gamma(0.95);  // reward discount factor
   reinforce_agent_ptr->set_rmsprop_decay_rate(0.90);
//   reinforce_agent_ptr->set_base_learning_rate(1E-4);
   reinforce_agent_ptr->set_base_learning_rate(3E-5);

   int n_max_episodes = 2 * 1000 * 1000;
//   int n_max_episodes = 20 * 1000 * 1000;

   int n_update = 100;
   int n_summarize = 1000;
   int n_snapshot = 20000;

   int n_losses = 0;
   int n_stalemates = 0;
   int n_wins = 0;
   int n_recent_losses = 0;
   int n_recent_stalemates = 0;
   int n_recent_wins = 0;

   double curr_reward = -999;
   double win_reward = 1;
   double stalemate_reward = 0.25;
   double lose_reward = -1;

// Periodically decrease learning rate down to some minimal floor
// value:

   int n_lr_episodes_period = 100 * 1000;

   int old_weights_period = 10; 
//   int old_weights_period = 32;  

   reinforce_agent_ptr->set_epsilon_time_constant(1000);
   double min_epsilon = 0.10;
   reinforce_agent_ptr->set_min_epsilon(min_epsilon);

   int AI_value = -1;     // "X" pieces
   int agent_value = 1;   // "O" pieces
//   bool AI_moves_first = true;
   bool AI_moves_first = false;
   bool periodically_switch_starting_player = false;
//   bool periodically_switch_starting_player = true;

   int update_old_weights_counter = 0;
   double total_loss = -1;

// Import previously trained TTT network to guide AI play:

   reinforce* reinforce_AI_ptr = NULL;
//   reinforce* reinforce_AI_ptr = new reinforce();

// ==========================================================================
// Reinforcement training loop begins here

   while(reinforce_agent_ptr->get_episode_number() < n_max_episodes)
   {
      int curr_episode_number = reinforce_agent_ptr->get_episode_number();
      outputfunc::update_progress_and_remaining_time(
         curr_episode_number, n_summarize, n_max_episodes);

      bool random_start = false;
      game_world.start_new_episode(random_start);
      reinforce_agent_ptr->initialize_episode();

// Decrease learning rate as training proceeds:

      if(curr_episode_number > 0 && 
         curr_episode_number%n_lr_episodes_period == 0)
      {
         reinforce_agent_ptr->decrease_learning_rate();
         if(periodically_switch_starting_player)
         {
            AI_moves_first = !AI_moves_first;
         }
      }

      if(AI_moves_first)
      {
         ttt_ptr->set_recursive_depth(0);   // AI plays stupidly
//         ttt_ptr->set_recursive_depth(2);   // AI plays offensively
      }
      else
      {
         ttt_ptr->set_recursive_depth(0);   // AI plays stupidly
//         ttt_ptr->set_recursive_depth(1);   // AI plays defensively
      }

// -----------------------------------------------------------------------
// Current episode starts here:

//      cout << "************  Start of Game " << curr_episode_number
//           << " ***********" << endl;

      genvector* next_s;
      while(!game_world.get_game_over())
      {
//         int curr_timestep = reinforce_agent_ptr->get_curr_timestep();
         int curr_timestep = -1;

// AI move:

         if((AI_moves_first && curr_timestep == 0) || curr_timestep > 0)
         {
            ttt_ptr->get_random_legal_player_move(AI_value);
//            compute_minimax_move(AI_moves_first, ttt_ptr, AI_value);
            ttt_ptr->increment_n_AI_turns();
//             ttt_ptr->display_board_state();

            if(ttt_ptr->check_player_win(AI_value) > 0)
            {
               game_world.set_game_over(true);
               curr_reward = lose_reward; // Agent loses!
//                reinforce_agent_ptr->record_reward_for_action(curr_reward);
               break;
            }
         
            if(ttt_ptr->check_filled_board())
            {
               game_world.set_game_over(true);
               curr_reward = stalemate_reward; // Entire board filled
//                reinforce_agent_ptr->record_reward_for_action(curr_reward);
               break;
            }
         } // AI_moves_first || timestep > 0 conditional

// Agent move:

         genvector *curr_s = game_world.get_curr_state();
         int d = reinforce_agent_ptr->store_curr_state_into_replay_memory(
            *curr_s);

         double Vstar;
         int curr_a = reinforce_agent_ptr->
            select_legal_action_for_curr_state(agent_value, Vstar);
         next_s = game_world.compute_next_state(curr_a, agent_value);

         ttt_ptr->increment_n_agent_turns();

// Step the environment and then retrieve new reward measurements:

         curr_reward = 0;
         if(ttt_ptr->check_player_win(agent_value) > 0)
         {
            game_world.set_game_over(true);
            curr_reward = win_reward;	 // Agent wins!
         }
         else if (ttt_ptr->check_filled_board())
         {
            game_world.set_game_over(true);
            curr_reward = stalemate_reward;
         }

//          reinforce_agent_ptr->record_reward_for_action(curr_reward);
//          reinforce_agent_ptr->increment_time_counters();

         if(game_world.get_game_over())
         {
            reinforce_agent_ptr->store_arsprime_into_replay_memory(
               d, curr_a, curr_reward, *curr_s, game_world.get_game_over());
         }
         else
         {
            reinforce_agent_ptr->store_arsprime_into_replay_memory(
               d, curr_a, curr_reward, *next_s, game_world.get_game_over());
         }

//         ttt_ptr->display_board_state();        

      } // !game_over while loop
// -----------------------------------------------------------------------

      reinforce_agent_ptr->increment_episode_number();
//      reinforce_agent_ptr->update_T_values();
//      reinforce_agent_ptr->update_running_reward(n_update);

// Periodically copy current weights into old weights:

      update_old_weights_counter++;
      if(update_old_weights_counter%old_weights_period == 0)
      {
         reinforce_agent_ptr->copy_weights_onto_old_weights();
         update_old_weights_counter = 1;
      }

      if(reinforce_agent_ptr->get_replay_memory_full() && 
         curr_episode_number % reinforce_agent_ptr->get_batch_size() == 0)
      {
         total_loss = reinforce_agent_ptr->update_Q_network();
      }
      
// Exponentially decay epsilon:

      reinforce_agent_ptr->exponentially_decay_epsilon(
         curr_episode_number, 0);

      if(nearly_equal(curr_reward, lose_reward))
      {
         n_losses++;
         n_recent_losses++;
      }

      else if (nearly_equal(curr_reward, win_reward))
      {
         n_wins++;
         n_recent_wins++;
      }
      else if(ttt_ptr->get_n_empty_cells() == 0)
      {
         n_stalemates++;
         n_recent_stalemates++;
      }

      if(curr_episode_number >= n_update - 1 && 
         curr_episode_number % n_update == 0)
      {
         ttt_ptr->display_board_state();
         cout << "V-learning" << endl;
         if(AI_value == -1)
         {
            cout << "AI = X    agent = O" << endl;
         }
         if(AI_moves_first)
         {
            cout << "AI moves first " << endl;
         }
         else
         {
            cout << "Agent moves first" << endl;
         }

         int n_episodes = curr_episode_number + 1;         
         double recent_loss_frac = double(n_recent_losses) / n_update;
         double recent_stalemate_frac = double(n_recent_stalemates) / n_update;
         double recent_win_frac = double(n_recent_wins) / n_update;
         double loss_frac = double(n_losses) / n_episodes;
         double stalemate_frac = double(n_stalemates) / n_episodes;
         double win_frac = double(n_wins) / n_episodes;
         cout << "n_episodes = " << n_episodes 
              << " n_recent_losses = " << n_recent_losses
              << " n_recent_stalemates = " << n_recent_stalemates
              << " n_recent_wins = " << n_recent_wins
              << endl;
         cout << " recent loss frac = " << recent_loss_frac
              << " recent stalemate frac = " << recent_stalemate_frac 
              << " recent win frac = " << recent_win_frac << endl;
         cout << " loss_frac = " << loss_frac
              << " stalemate_frac = " << stalemate_frac
              << " win_frac = " << win_frac
              << endl;
         n_recent_losses = n_recent_stalemates = n_recent_wins = 0;
         cout << "  Total loss = " << total_loss << endl;

         if(total_loss > 0)
         {
            cout << " log10(total_loss) = " << log10(total_loss) << endl;
            reinforce_agent_ptr->push_back_log10_loss(log10(total_loss));
         }

         double curr_n_turns_frac = double(
            ttt_ptr->get_n_AI_turns() + ttt_ptr->get_n_agent_turns()) / 
            n_max_turns;
         reinforce_agent_ptr->append_n_episode_turns_frac(curr_n_turns_frac);
//          reinforce_agent_ptr->snapshot_running_reward();

         ttt_ptr->append_game_loss_frac(loss_frac);
         ttt_ptr->append_game_stalemate_frac(stalemate_frac);
         ttt_ptr->append_game_win_frac(win_frac);
      }

      if(curr_episode_number >= n_summarize - 1 && 
         curr_episode_number % n_summarize == 0)
      {
         reinforce_agent_ptr->compute_weight_distributions();
         reinforce_agent_ptr->store_quasirandom_weight_values();
         reinforce_agent_ptr->generate_summary_plots();
         ttt_ptr->plot_game_frac_histories(
            output_subdir, curr_episode_number, extrainfo);
      }

      if(curr_episode_number > 0 && curr_episode_number % n_snapshot == 0)
      {
         reinforce_agent_ptr->export_snapshot();
      }
      
   } // n_episodes < n_max_episodes while loop

// Reinforcement training loop ends here
// ==========================================================================

   outputfunc::print_elapsed_time();
   cout << "Final episode number = "
        << reinforce_agent_ptr->get_episode_number() << endl;
   cout << "N_weights = " << reinforce_agent_ptr->count_weights()
        << endl;

// Export trained weights in neural network's zeroth layer as
// greyscale images to output_subdir

   reinforce_agent_ptr->plot_zeroth_layer_weights();

   delete ttt_ptr;
   delete reinforce_agent_ptr;
   delete reinforce_AI_ptr;
}
