// =================================================================
// Bayesian belief network subroutines written in C (not C++ !)
// =================================================================
// Last updated on 7/4/01
// =================================================================

#include <math.h>
#include "bayesian_c.h"

// ----------------------------------------------------------------------
// Subroutine evaluate_bayesian_network acts as a C++ interface to Tim
// Wallace's Bayesian belief network algorithms which are written in
// C.  As of 7/4/01, we model each node within the network as having 2
// states: satellite is stationary vs satellite is maneuvering.  The
// total number of nodes within the network is input in parameter
// n_nodes.  The time in seconds associated with each node since the
// start of the satellite pass is contained in node_time.  Conditional
// probabilistic evidence based upon image processing is passed within
// arrays pstationary_given_evidence and pmaneuvering_given_evidence.
// After this subroutine sets up, initializes and updates the Bayesian
// belief network, it returns the belief probability that the
// satellite is maneuvering within image i in maneuvering_belief[i].
// The belief probability that the satellite is stationary simply
// equals 1 - maneuvering_belief[i].

void evaluate_bayesian_network(
   int n_nodes,double node_time[],
   double pstationary_given_evidence[],
   double pmaneuvering_given_evidence[],
   double maneuvering_belief[])
{
   char name[80];
   int i;
   NETNODE **status;
   NETVECT evidence;

   status = (NETNODE **)calloc(n_nodes,sizeof(NETNODE *));
   init_netvect(&evidence,2);

   initialize_bayesian_network(n_nodes,node_time,status);

// Now we have network in its initial state, reflecting prior beliefs
// only.  We next need to admit evidence, and then update network.

   for (i=0; i<n_nodes; i++)
   {
      update_bayesian_network(i,status,&evidence,
                              pstationary_given_evidence[i],
                              pmaneuvering_given_evidence[i]);
   }

   dump_network(status[0]);

   for (i=0; i<n_nodes; i++)
   {
//      stationary_belief[i]=status[i]->belief.dat[0];
      maneuvering_belief[i]=status[i]->belief.dat[1];
   }
}

// =================================================================
// Subroutine initialize_bayesian_network sets up and initializes a
// 2-state Bayesian belief network containing n_nodes nodes.  The
// state of each node corresponds to satellite stationary vs satellite
// maneuvering.  The array node_time containing image times since
// start of the pass is used to set conditional transition
// probabilities between each of the nodes.  If the time differential
// dt between 2 nodes is small compared to some time constant tau, the
// probability that the satellite switches its status between time i
// and time i+1 is exponentially small.  On the other hand, if dt >>
// tau, the probability that the satellite switches its status
// approaches 0.5.

void initialize_bayesian_network(int n_nodes,double node_time[],
                                 NETNODE **status)
{
   char name[80];
   int i;
   double dt,tau;
   double apriori_stationary_prob=0.9;
   double apriori_maneuvering_prob=0.1;
   double pstatic,pchange;

// Initialize root node:

   status[0] = create_node(2,0,"0");
   add_statename(status[0],0,"stationary");
   add_statename(status[0],1,"maneuvering");
   status[0]->pi.dat[0]=apriori_stationary_prob;
   status[0]->pi.dat[1]=apriori_maneuvering_prob;
   revise_belief(status[0]);
 
// Initialize remaining nodes within chain:

   for (i=1; i<n_nodes; i++)
   {
      sprintf(name,"%d",i);
      status[i] = create_node(2,2,name);
      add_statename(status[i],0,"stationary");
      add_statename(status[i],1,"maneuvering");
      status[i]->pi.dat[0]=apriori_stationary_prob;
      status[i]->pi.dat[1]=apriori_maneuvering_prob;
      status[i-1]->first_child=status[i];
      status[i]->parent=status[i-1];
      nvcopy(&(status[i]->incoming_pi),&(status[i-1]->belief));

// Model conditional transition probabilities as decaying exponential
// functions of time:

      tau=5.0;	// Time constant in secs for satellite to change status

      dt=node_time[i]-node_time[i-1];
      pstatic=0.5*(1+exp(-dt/tau));
      pchange=1-pstatic;

      printf("i = %d  node_time[i] = %f \n",i,node_time[i]);
      printf("dt/tau = %f  exp(-dt/tau) = %f \n",dt/tau,exp(-dt/tau));
      printf("i = %d  dt = %f   pstatic = %f  pchange = %f \n",
             i,dt,pstatic,pchange);
      
      status[i]->CP.mat[0][0] = pstatic;
      status[i]->CP.mat[0][1] = pchange;
      status[i]->CP.mat[1][0] = pchange;
      status[i]->CP.mat[1][1] = pstatic;
      build_transpose(status[i]);
      revise_belief(status[i]);
   }
}

// =================================================================
// Subroutine update_bayesian_network takes in conditional
// probabilities that the satellite is stationary or maneuvering
// depending upon some image processing criterion within the NETVECT
// *evidence_ptr.  It then calls Wallace's submit_evidence subroutine
// which updates all probability values within the entire Bayesian
// network.

void update_bayesian_network(
   int currnode,NETNODE **status,NETVECT *evidence_ptr,
   double pstationary_given_evidence,double pmaneuvering_given_evidence)
{
   evidence_ptr->dat[0] = pstationary_given_evidence;
   evidence_ptr->dat[1] = pmaneuvering_given_evidence;
   submit_evidence(status[currnode],evidence_ptr);
}


