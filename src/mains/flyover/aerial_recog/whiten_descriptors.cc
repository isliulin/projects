// ==========================================================================
// WHITEN_DESCRIPTORS imports contrast normalized 192x1 RGB-valued
// [64x1 greyscale intensity] descriptors for 8x8 pixel patches
// selected from character jpeg images generated by program
// INITIAL_DESCRIPTORS.  It computes the descriptors' covariance
// matrix along with a regularized version of its inverse square root
// covariance matrix.  Each input descriptor is whitened by
// multiplying it by the inverse square root covariance
// matrix. Whitened patch descriptors are exported to a single HDF5
// binary file.  The inverse square root of the covariance matrix is
// also exported to a text file within the dictionary subdirectory.

//			    whiten_descriptors

// ==========================================================================
// Last updated on 5/25/14; 5/26/14; 5/31/14; 6/21/14
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include <flann/flann.hpp>
#include <flann/io/hdf5.h>

#include "general/filefuncs.h"
#include "math/genmatrix.h"
#include "math/genvector.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ofstream;
using std::string;
using std::vector;


double **mat_alloc(int n, int m)
{
  int i, j;
  double **ar;

  ar = (double **)malloc(((n + 1) & ~1) * sizeof(double *) + n * m * sizeof(double));
  ar[0] = (double *)(ar + ((n + 1) & ~1));

  for(i = 0; i < n; i++){
    ar[i] = ar[0] + i * m;
    for(j = 0; j < m; j++){
      ar[i][j] = 0.0;
    }
  }
  return(ar);
}

void mat_free(double **a1)
{
  free(a1);
}

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

   const unsigned int D=64*3;     // 8 x 8 RGB patches
//   const unsigned int D=64;     // 8 x 8 greyscale patches

   string trees_subdir="./trees/";
   string dictionary_subdir=trees_subdir+"dictionary/";

// Import initial patch descriptors for all character jpeg images:

   flann::Matrix<float> patch_descriptors;
   string patches_hdf5_filename=
      dictionary_subdir+"contrast_normalized_patch_descriptors.hdf5";
   flann::load_from_file(
      patch_descriptors,patches_hdf5_filename.c_str(),
      "contrast_normalized_patch_descriptors");

   unsigned int N=patch_descriptors.rows;
   cout << "N = " << N << endl;

// Compute covariance matrix from input contrast-normalized
// descriptors:
   
   double incremental_mean[D];
   double** incremental_second_moment=mat_alloc(D,D);
   double** covar=mat_alloc(D,D);

   for (unsigned int d=0; d<D; d++)
   {
      incremental_mean[d]=0;
   }
   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         incremental_second_moment[i][j]=0;
      }
   }

   for (unsigned int n=0; n<N; n++)
   {
      float* X_ptr=patch_descriptors.ptr()+n*D;

      mathfunc::incremental_mean_vec(
         D, n, X_ptr, incremental_mean);
      mathfunc::incremental_second_vec_moment(
         D, n, X_ptr, incremental_second_moment);
   } // loop over index n labeling rows in patch_descriptors array


   mathfunc::covar_matrix(
      D, incremental_mean, incremental_second_moment, covar);

/*
// Print out mean for all N contrast-normalized descriptors 
// (should be close but not exactly equal to D-dimensional zero vector)

   for (unsigned int d=0; d<D; d++)
   {
      cout << incremental_mean[d] << " ";
   }
   cout << endl;

// Print out covariance matrix for all N contrast-normalized descriptors:
// (should be very roughly close to DxD identity matrix)

   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         cout << covar[i][j] << " ";
      }
      cout << endl;
   }
   outputfunc::enter_continue_char();
*/

   genmatrix covar_matrix(D,D);
   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         covar_matrix.put(i,j,incremental_second_moment[i][j]);
      }
   }

// Compute inverse square-root of covariance matrix via SVD.  Recall
// SVD of symmetric matrix yields eigenvector/eigenvalue
// decomposition:

   genmatrix U(D,D), W(D,D), V(D,D);
   covar_matrix.sorted_singular_value_decomposition(U,W,V);

//   cout << "U-V = " << U-V << endl;
//   cout << "W = " << W << endl;

   for(unsigned int d=0; d<D; d++)
   {
      cout << "d = " << d << " W(d,d) = " << W.get(d,d) << endl;
   }
//   outputfunc::enter_continue_char();

// Choose ZCA regularization parameter based upon smallest eigenvalues
// in W (see http://ufldl.stanford.edu/wiki/index.php/Whitening):

//   const double eps_ZCA=1E-5;
//   const double eps_ZCA=0.003;	// Nontrivially "regulates" O(10) smallest
				//  evalues in W
//   const double eps_ZCA=0.01;	// Coates recommended value for 16x16 descrips
   const double eps_ZCA=0.1;	// Coates recommended value for 8x8 descriptors

// Note: Adopting eps_ZCA = 0.1 leads to a covariance matrix for
// whitened descriptors which is quite far from the identity.  But as
// Coates indicates, it does appear to yield a better dictionary with
// fewer (though still sizable) number of "noisy" elements.

   genmatrix ZCA(D,D);
   ZCA.clear_values();
   for (unsigned int d=0; d<D; d++)
   {
      ZCA.put(d, d, 1/sqrt(W.get(d,d) + eps_ZCA) );
   }
   
   genmatrix inverse_sqrt_covar(D,D);
   inverse_sqrt_covar = V * ZCA * V.transpose();

//   cout << "inverse_sqrt_covar = " << endl;
//   cout << inverse_sqrt_covar << endl;
//   outputfunc::enter_continue_char();

// Export inverse square root of covariance matrix to a text file so
// that it does not need to be computed more than once:

   string inverse_covar_sqrt_filename=
      dictionary_subdir+"inverse_sqrt_covar_matrix.dat";
   ofstream inverse_outstream;
   filefunc::openfile(inverse_covar_sqrt_filename,inverse_outstream);

   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         inverse_outstream << inverse_sqrt_covar.get(i,j) << endl;
      }
   }
   filefunc::closefile(inverse_covar_sqrt_filename,inverse_outstream);   

   string banner="Exported "+inverse_covar_sqrt_filename;
   banner += " to "+dictionary_subdir;
   outputfunc::write_big_banner(banner);

// Whiten all descriptors by multiplying them with inverse_sqrt_covar:

   flann::Matrix<float> *whitened_patch_descriptors
      =new flann::Matrix<float>(new float[N*D],N,D);

   for (unsigned int n=0; n<N; n++)
   {
      float* X_ptr=patch_descriptors.ptr()+n*D;

//      double X_sum = 0;
//      double whitened_X_sum = 0;
      for (unsigned int d=0; d<D; d++)
      {
         double whitened_X_component=0;
         for (unsigned int j=0; j<D; j++)
         {
            whitened_X_component += inverse_sqrt_covar.get(d,j) * X_ptr[j];
         }
         (*whitened_patch_descriptors)[n][d]=whitened_X_component;

//         X_sum += X_ptr[d];
//         whitened_X_sum += whitened_X_component;

      } // loop over index d labeling components of whitened_X descriptor
//      cout << "n = " << n << " X_sum = " << X_sum << " whitened_X_sum = "
//           << whitened_X_sum << endl;
   } // loop over index n labeling rows in patch_descriptors array

// Recompute covariance matrix for whitened descriptors:

   for (unsigned int d=0; d<D; d++)
   {
      incremental_mean[d]=0;
   }
   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         incremental_second_moment[i][j]=0;
      }
   }

   for (unsigned int n=0; n<N; n++)
   {
      float* X_ptr=whitened_patch_descriptors->ptr()+n*D;
      mathfunc::incremental_mean_vec(
         D, n, X_ptr, incremental_mean);
      mathfunc::incremental_second_vec_moment(
         D, n, X_ptr, incremental_second_moment);
   } // loop over index n labeling rows in patch_descriptors array
   mathfunc::covar_matrix(
      D, incremental_mean, incremental_second_moment, covar);

// Print out mean for all N whitened descriptors 
// (should be close to D-dimensional zero vector)

   cout << "Mean for N whitened descriptors" << endl;
   for (unsigned int d=0; d<D; d++)
   {
      cout << incremental_mean[d] << " ";
   }
   cout << endl;

// Print out covariance matrix for all N whitened descriptors:
// (should be quasi-close to DxD identity matrix)

   cout << "Covar matrix for N whitened descriptors" << endl;
   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         cout << covar[i][j] << " ";
      }
      cout << endl;
   }

   cout << "Saving whitened patch descriptors" << endl;
   string whitened_hdf5_filename=dictionary_subdir+
      "whitened_patch_descriptors.hdf5";
   filefunc::deletefile(whitened_hdf5_filename);
   flann::save_to_file(
      *whitened_patch_descriptors,whitened_hdf5_filename,
      "whitened_patch_descriptors");

   banner="Wrote N = "+stringfunc::number_to_string(N)
      +" whitened patch descriptors to "+whitened_hdf5_filename;
   outputfunc::write_big_banner(banner);

   mat_free(incremental_second_moment);
   mat_free(covar);
   delete [] patch_descriptors.ptr();
   delete [] whitened_patch_descriptors->ptr();
   delete whitened_patch_descriptors;
}

   
