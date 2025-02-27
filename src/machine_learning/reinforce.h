// ==========================================================================
// Header file for reinforce class 
// ==========================================================================
// Last modified on 1/13/17; 1/18/17; 1/23/17; 1/24/17
// ==========================================================================

#ifndef REINFORCE_H
#define REINFORCE_H

#include <deque>
#include <map>
#include <iostream>
#include <vector>

#include "machine_learning/environment.h"

class environment;
class genmatrix;
class genvector;

class reinforce
{
   
  public:

   typedef enum{
      QLEARNING = 0,
      PLEARNING = 1,  // policy gradients
      VLEARNING = 2
   } learning_t;

   typedef enum{
      SGD = 0,
      RMSPROP = 1,
      MOMENTUM = 2,
      NESTEROV = 3,
      ADAM = 4
   } solver_t;

   typedef std::map<std::string, double > Q_MAP;
// independent string: string rep for state + action
// dependent double: Q(s,a)

// Initialization, constructor and destructor functions:

   reinforce(bool include_biases, const std::vector<int>& n_nodes_per_layer);
   reinforce(bool include_biases, const std::vector<int>& n_nodes_per_layer,
             int replay_memory_capacity, int solver_type = RMSPROP);
   reinforce(bool include_biases, const std::vector<int>& n_nodes_per_layer,
             int batch_size, int replay_memory_capacity, 
             int eval_memory_capacity, int solver_type = RMSPROP);
   reinforce(std::string snapshot_filename, int replay_memory_capacity,
             int eval_memory_capacity, int solver_type);

   ~reinforce();
   friend std::ostream& operator<< 
      (std::ostream& outstream,const reinforce& R);

// Set & get member functions:

   void set_expt_number(int n);
   int get_expt_number() const;
   void set_output_subdir(std::string subdir);   
   void set_extrainfo(std::string info);
   std::string get_params_filename() const;
   bool get_include_biases() const;
   void set_environment(environment* e_ptr);
   void set_debug_flag(bool flag);
   bool get_debug_flag() const;
   int get_episode_number() const;
   int increment_episode_number();
   void append_n_episode_turns_frac(double curr_n_turns_frac);
   void set_curr_epoch(double epoch);
   double get_curr_epoch() const;
   void set_base_learning_rate(double rate);
   double get_base_learning_rate() const;
   void push_back_learning_rate(double rate);
   double get_learning_rate() const;
   void set_batch_size(double bsize);
   int get_batch_size() const;
   void set_lambda(double lambda);
   void set_Nd(int Nd);
   void set_gamma(double gamma);
   double get_gamma() const;
   void set_rmsprop_decay_rate(double rate);
   void set_ADAM_params(double beta1, double beta2);
   int get_n_backprops() const;
   void push_back_prob_action_0(double p0);
   void clear_prob_action_0();
   genvector* get_curr_s_sample();
   const genvector* get_curr_s_sample() const;
   void set_max_mean_KL_divergence(double delta);

   void initialize_episode();
   void update_cumulative_reward(double cum_reward);
   void update_n_frames_per_episode(int n_frames);
   void update_episode_history();
   void update_epoch_history();
   void update_epsilon();

// Monitoring network training methods:

   void summarize_parameters();
   int count_weights();
   void print_biases();
   void print_weights();

   void plot_zeroth_layer_weights();
   void plot_zeroth_layer_weights(int nx, int ny);
   void compute_bias_distributions();
   void compute_weight_distributions();
   void store_quasirandom_weight_values();

   std::string init_subtitle();
   void generate_metafile_plot(
      const std::vector<double>& values,
      std::string metafile_basename, std::string title, std::string y_label, 
      bool epoch_indep_var, bool plot_smoothed_values_flag,
      bool zero_min_value_flag);
   void plot_loss_history();
   void plot_avg_discounted_eventual_reward(bool epoch_indep_var);
   void plot_maxQ_history(bool epoch_indep_var);
   void plot_reward_history(bool epoch_indep_var, bool plot_cumulative_reward);
   void plot_turns_history();
   void plot_frames_history(bool epoch_indep_var);
   void plot_episode_number_history(bool epoch_indep_var);
   void plot_epsilon_history(bool epoch_indep_var);
   void plot_lr_history(bool epoch_indep_var);
   void plot_Qmap_score_history();
   void plot_log10_loss_history(bool epoch_indep_var);
   void plot_log10_lr_mean_abs_nabla_weight_ratios(bool epoch_indep_var);
   void plot_prob_action_0();
   void plot_KL_divergence_history(bool epoch_indep_var);

   bool plot_bias_distributions(bool epoch_indep_var);
   void plot_weight_distributions(bool epoch_indep_var);
   void plot_quasirandom_weight_values(bool epoch_indep_var);
   void generate_summary_plots(bool epoch_indep_var = true);
   void generate_view_metrics_script(bool maze_flag, bool atari_flag);

   std::string export_snapshot();
   void import_snapshot(std::string snapshot_filename);

// General learning methods:

   void clear_delta_nablas();
   void clear_nablas();
   void update_weights_and_biases(double lr);
   void decrease_learning_rate();

// Q learning methods

   bool get_replay_memory_full() const;
   void copy_weights_onto_old_weights();
   int get_random_action() const;
   int get_random_legal_action() const;

   void set_epsilon(double eps);
   double get_epsilon() const;
   void set_min_epsilon(double min_eps);
   void set_epsilon_time_constant(double tau);
   double exponentially_decay_epsilon(double t, double tstart);
   double linearly_decay_epsilon(double t, double tstart, double tstop);

   int select_action_for_curr_state();
   int select_legal_action_for_curr_state();
   void compute_deep_Qvalues();
   void Q_forward_propagate(
      genvector* s_input, bool use_old_weights_flag = false);
   int compute_argmax_Q();
   int compute_legal_argmax_Q(double& Qstar);
   int compute_legal_argmin_Q(double& Qstar);
   int compute_legal_argextremum_Q(bool max_flag, double& Qstar);

   int store_curr_state_into_replay_memory(const genvector& curr_s);
   void store_arsprime_into_replay_memory(
      int d, int curr_a, double curr_r,
      const genvector& next_s, bool terminal_state_flag);
   void store_final_arsprime_into_replay_memory(
      int d, int curr_a, double curr_r);
   void store_ar_into_replay_memory(int d, int curr_a, double curr_r,
      bool terminal_state_flag);

   double update_Q_network(bool verbose_flag = false);
   bool get_replay_memory_entry(
      int d, genvector& curr_s, int& curr_a, double& curr_r);
   bool get_replay_memory_entry(
      int d, genvector& curr_s, int& curr_a, double& curr_r,genvector& next_s);
   bool store_curr_state_into_eval_memory(const genvector& curr_s);
   void compute_max_eval_Qvalues_distribution();

   double compute_target(double curr_r, genvector* next_s, 
                         bool terminal_state_flag);
   double L2_loss_contribution();
   double compute_curr_Q_loss(int curr_a, double target_value);
   double Q_backward_propagate(int d, int Nd, bool verbose_flag = false);
   void numerically_check_Q_derivs(int curr_a, double target_value);

   void set_Q_value(std::string state_action_str, double Qvalue);
   double get_Q_value(std::string state_action_str);
   void init_random_Qmap();
   void print_Qmap();
   void push_back_Qmap_score(double score);
   void push_back_log10_loss(double log10_loss);
   Q_MAP* get_qmap_ptr();
   const Q_MAP* get_qmap_ptr() const;

// Value function learning methods

   void set_n_actions(int n);
   double compute_value(
      genvector* curr_afterstate, bool use_old_weights_flag = false);
   int V_forward_propagate_afterstates(int player_value, double& Vstar);
   int select_legal_action_for_curr_state(int player_value, double& Vstar);
   double compute_target(
      int curr_a, int player_value, double curr_r, bool terminal_state_flag);
   double get_prev_afterstate_curr_value();

// Policy gradient learning methods

   void P_forward_propagate(genvector* s_input);
   void compute_pi_given_state(genvector* s_input, genvector *pi_output);
   void store_curr_pi_into_replay_memory(int d, genvector *curr_pi);
   void get_curr_pi_from_replay_memory(int d);
   void compute_next_pi_given_replay_index(int d);
   double compute_mean_KL_divergence_between_curr_and_next_pi();

   int get_P_action_given_pi(genvector *curr_pi);
   int get_P_action_given_pi(
      genvector *curr_pi, double ran_val, double& action_prob);
   double compute_curr_P_loss(int d, double action_prob);
   double P_backward_propagate(int d, int Nd, bool verbose_flag);
   void numerically_check_P_derivs(int d, double ran_value);
   double update_P_network(bool verbose_flag);
   int take_KL_divergence_constrained_step();

   void clear_replay_memory();
   void compute_renormalized_discounted_eventual_rewards();
   double get_advantage(int d) const;

  private:

   bool include_biases;
   bool perm_symmetrize_weights_and_biases;
   bool debug_flag;
   int expt_number;
   int learning_type;
   int solver_type;
   int n_layers, n_actions, n_weights;
   int n_backprops;
   std::string output_subdir;
   std::string extrainfo;
   std::string layer_label;
   std::string params_filename;
   std::vector<int> layer_dims;
   environment* environment_ptr;
   
   std::deque<double> T_values;  // Holds latest T values
   int batch_size;  	// Perform parameter update after this many episodes
   double base_learning_rate;
   std::vector<double> learning_rate;
   double mu;	  	// Damping coeff for momentum solver type
   double lambda;	// L2 regularization coefficient
   double gamma;	// Discount factor for reward
   double rmsprop_decay_rate; // Decay factor for RMSProp leaky sum of grad**2
   double rmsprop_denom_const;  // const added to denom in RMSProp

   std::vector<genvector*> biases, old_biases;
   std::vector<genvector*> permuted_biases, sym_biases;
//	Bias STL vectors are nonzero for layers 1 thru n_layers-1
   std::vector<genvector*> nabla_biases, delta_nabla_biases;

   std::vector<genmatrix*> weights, weights_transpose;
   std::vector<genmatrix*> permuted_weights, sym_weights;
//	Weight STL vectors connect layer pairs {0,1}, {1,2}, ... , 
//      {n_layers-2, n_layers-1}
   std::vector<genmatrix*> old_weights;
   std::vector<genmatrix*> nabla_weights, delta_nabla_weights;
   std::vector<genmatrix*> velocities, prev_velocities;
   std::vector<genmatrix*> adam_m, adam_v;

   std::vector<genvector*> rmsprop_biases_cache;
   std::vector<genvector*> rmsprop_biases_denom;
   std::vector<genmatrix*> rmsprop_weights_cache;
   std::vector<genmatrix*> rmsprop_weights_denom;

// STL vector index ranges over layers l = 0, 1, ..., n_layers
// row index ranges over lth layer nodes j = 0, 1, ... n_nodes_in_lth_layer

// Node weighted inputs:

   std::vector<genvector*> Z_Prime;
   std::vector<genvector*> gammas, betas;  // Batch normalization parameters

// Node activation outputs:

   std::vector<genvector*> A_Prime;    // n_actions x 1
   int hardwired_output_action;

// Node errors:

   std::vector<genvector*> Delta_Prime; // n_actions x 1 
   
// Episode datastructures:

   std::vector<double> time_samples;
   std::vector<double> loss_values;
   std::vector<double> n_episode_turns_frac;
   std::vector<double> n_frames_per_episode;
   std::vector<double> episode_history;
   std::vector<double> epoch_history;
   std::vector<double> epsilon_values;
   std::vector<double> Qmap_scores;
   std::vector<double> log10_losses;
   std::vector<double> log10_lr_mean_abs_nabla_weight_ratios;

   std::vector<double> max_eval_Qvalues_10;
   std::vector<double> max_eval_Qvalues_25;
   std::vector<double> max_eval_Qvalues_50;
   std::vector<double> max_eval_Qvalues_75;
   std::vector<double> max_eval_Qvalues_90;

   std::vector<std::vector<double> > weight_01, weight_05, weight_10;
   std::vector<std::vector<double> > weight_25, weight_35, weight_50;
   std::vector<std::vector<double> > weight_65, weight_75, weight_90;
   std::vector<std::vector<double> > weight_95, weight_99;
   std::vector<std::vector<double> > bias_01, bias_05, bias_10, bias_25;
   std::vector<std::vector<double>  >bias_35, bias_50, bias_65, bias_75;
   std::vector<std::vector<double> > bias_90, bias_95, bias_99;

// Store histories for 9 "quasi-random" weights for each layer within
// following vectors of vectors:

   std::vector<std::vector<double> > weight_1, weight_2, weight_3;
   std::vector<std::vector<double> > weight_4, weight_5, weight_6;
   std::vector<std::vector<double> > weight_7, weight_8, weight_9;

   std::vector<double> running_reward_snapshots;
   std::vector<double> cumulative_reward_snapshots;
   std::vector<double> avg_discounted_eventual_rewards;
   double mu_R, sigma_R;
   int episode_number;
   double curr_epoch;

   std::string snapshots_subdir;

// Q learning variables:

   bool replay_memory_full_flag;
   int replay_memory_capacity;
   int replay_memory_index; // 0 <=replay_memory_index < replay_memory_capacity
   bool eval_memory_full_flag;
   int eval_memory_capacity;
   int eval_memory_index; // 0 <= eval_memory_index < eval_memory_capacity

   int Nd;  // Number of random samples to be drawn from replay memory
   double epsilon;	// Select random action with probability epsilon
   double epsilon_decay_factor;
   double min_epsilon;  // Minimal value for annealed epsilon
   double epsilon_tau; // Exponential time constant for epsilon

   genmatrix *s_eval;  // eval_memory_capacity x Din

   genmatrix *s_curr;  // replay_memory_capacity x Din
   genvector *a_curr;  // replay_memory_capacity x 1 (Holds action indices)
   genvector *r_curr;  // replay_memory_capacity x 1  (Holds rewards)
   genmatrix *s_next;  // replay_memory_capacity x Din
   genvector *terminal_state;   // replay_memory_capacity x 1

   genvector *curr_s_sample, *next_s_sample;  // Din x 1 

   Q_MAP qmap;
   Q_MAP::iterator qmap_iter;

// P learning variables:

   double max_mean_KL_divergence;
   std::vector<double> prob_action_0;
   std::vector<double> log10_mean_KL_divergences;
   genvector *curr_pi_sample; // Dout x 1
   genvector *next_pi_sample; // Dout x 1
   genmatrix *pi_curr; // replay_memory_capacity * Dout

// V learning variables:

   genvector *prev_afterstate_ptr; // Din x 1

// ADAM solver variables:

   double beta1, beta2;
   double curr_beta1_pow, curr_beta2_pow;

   void compute_cumulative_action_dist();
   void update_rmsprop_biases_cache(double decay_rate);
   void update_rmsprop_weights_cache(double decay_rate);

   void allocate_member_objects();
   void initialize_member_objects(const std::vector<int>& n_nodes_per_layer);
   void instantiate_weights_and_biases();
   void instantiate_training_variables();
   void initialize_weights_and_biases();
   void delete_weights_and_biases();
};

// ==========================================================================
// Inlined methods:
// ==========================================================================

// Set and get member functions:

inline void reinforce::set_expt_number(int n)
{
   expt_number = n;
}

inline int reinforce::get_expt_number() const
{
   return expt_number;
}

inline void reinforce::set_extrainfo(std::string info)
{
   extrainfo = info;
}

inline std::string reinforce::get_params_filename() const
{
   return params_filename;
}

inline bool reinforce::get_include_biases() const
{
   return include_biases;
}

inline void reinforce::set_environment(environment* e_ptr)
{
   environment_ptr = e_ptr;
}

inline void reinforce::set_debug_flag(bool flag)
{
   debug_flag = flag;
}

inline bool reinforce::get_debug_flag() const
{
   return debug_flag;
}

inline int reinforce::get_episode_number() const
{
   return episode_number;
}

inline int reinforce::increment_episode_number() 
{
   episode_number++;
   return episode_number;
}

inline void reinforce::set_curr_epoch(double epoch)
{
   curr_epoch = epoch;
}

inline double reinforce::get_curr_epoch() const
{
   return curr_epoch;
}

inline void reinforce::append_n_episode_turns_frac(double frac)
{
   n_episode_turns_frac.push_back(frac);
}

inline void reinforce::set_base_learning_rate(double rate)
{
   base_learning_rate = rate;
   learning_rate.push_back(base_learning_rate);
}

inline double reinforce::get_base_learning_rate() const
{
   return base_learning_rate;
}

inline void reinforce::push_back_learning_rate(double rate)
{
   learning_rate.push_back(rate);
}

inline double reinforce::get_learning_rate() const
{
   return learning_rate.back();
}

inline void reinforce::set_batch_size(double bsize)
{
   batch_size = bsize;
}

inline int reinforce::get_batch_size() const
{
   return batch_size;
}

inline void reinforce::set_lambda(double lambda)
{
   this->lambda = lambda;
}

inline void reinforce::set_Nd(int Nd)
{
   this->Nd = Nd;
}

inline void reinforce::set_gamma(double gamma)
{
   this->gamma=gamma;
}

inline double reinforce::get_gamma() const
{
   return gamma;
}

inline void reinforce::set_rmsprop_decay_rate(double rate)
{
   rmsprop_decay_rate = rate;
}

inline reinforce::Q_MAP* reinforce::get_qmap_ptr()
{
   return &qmap;
}

inline const reinforce::Q_MAP* reinforce::get_qmap_ptr() const
{
   return &qmap;
}

inline void reinforce::push_back_Qmap_score(double score)
{
   Qmap_scores.push_back(score);
}

inline void reinforce::push_back_log10_loss(double log10_loss)
{
   log10_losses.push_back(log10_loss);
}

inline bool reinforce::get_replay_memory_full() const
{
   return replay_memory_full_flag;
}

inline void reinforce::set_ADAM_params(double beta1, double beta2)
{
   this->beta1 = beta1;
   this->beta2 = beta2;
}

inline int reinforce::get_n_backprops() const
{
   return n_backprops;
}

inline void reinforce::push_back_prob_action_0(double p0)
{
   prob_action_0.push_back(p0);
}

inline void reinforce::clear_prob_action_0()
{
   prob_action_0.clear();
}

inline genvector* reinforce::get_curr_s_sample()
{
   return curr_s_sample;
}

inline const genvector* reinforce::get_curr_s_sample() const
{
   return curr_s_sample;
}

inline void reinforce::set_max_mean_KL_divergence(double delta)
{
   max_mean_KL_divergence = delta;
}


#endif  // reinforce.h


