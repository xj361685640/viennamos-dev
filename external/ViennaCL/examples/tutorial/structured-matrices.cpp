/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

/*
*
*   Tutorial:  Handling structured dense matrices (experimental, only available with OpenCL backend)
*
*/

// include necessary system headers
#include <iostream>

//
// ViennaCL includes
//
#include "viennacl/toeplitz_matrix.hpp"
#include "viennacl/circulant_matrix.hpp"
#include "viennacl/vandermonde_matrix.hpp"
#include "viennacl/hankel_matrix.hpp"
#include "viennacl/linalg/prod.hpp"

//
// Boost includes
//
#include "boost/numeric/ublas/vector.hpp"
#include "boost/numeric/ublas/matrix.hpp"
#include "boost/numeric/ublas/matrix_proxy.hpp"
#include "boost/numeric/ublas/io.hpp"


int main()
{
  typedef float      ScalarType;

  std::size_t size = 4;

  //
  // Set up ublas objects
  //
  boost::numeric::ublas::vector<ScalarType> ublas_vec(size);
  boost::numeric::ublas::matrix<ScalarType> ublas_circulant(size, size);
  boost::numeric::ublas::matrix<ScalarType> ublas_hankel(size, size);
  boost::numeric::ublas::matrix<ScalarType> ublas_toeplitz(size, size);
  boost::numeric::ublas::matrix<ScalarType> ublas_vandermonde(size, size);

  for(std::size_t i = 0; i < size; i++)
    for(std::size_t j = 0; j < size; j++)
    {
      ublas_circulant(i,j)   = static_cast<ScalarType>((i - j + size) % size);
      ublas_hankel(i,j)      = static_cast<ScalarType>((i + j) % (2 * size));
      ublas_toeplitz(i,j)    = static_cast<ScalarType>(i) - static_cast<ScalarType>(j);
      ublas_vandermonde(i,j) = pow(ScalarType(1.0 + i/1000.0), ScalarType(j));
    }

  //
  // Set up ViennaCL objects
  //
  viennacl::vector<ScalarType>             vcl_vec(size);
  viennacl::vector<ScalarType>             vcl_result(size);
  viennacl::circulant_matrix<ScalarType>   vcl_circulant(size, size);
  viennacl::hankel_matrix<ScalarType>      vcl_hankel(size, size);
  viennacl::toeplitz_matrix<ScalarType>    vcl_toeplitz(size, size);
  viennacl::vandermonde_matrix<ScalarType> vcl_vandermonde(size, size);

  // copy matrices:
  viennacl::copy(ublas_circulant, vcl_circulant);
  viennacl::copy(ublas_hankel, vcl_hankel);
  viennacl::copy(ublas_toeplitz, vcl_toeplitz);
  viennacl::copy(ublas_vandermonde, vcl_vandermonde);

  // fill vectors:
  for(std::size_t i = 0; i < size; i++)
  {
    ublas_vec[i] = ScalarType(i);
    vcl_vec[i] = ScalarType(i);
  }

  //
  // Add matrices:
  //
  std::cout << "Circulant matrix before addition: " << vcl_circulant << std::endl << std::endl;
  vcl_circulant += vcl_circulant;
  std::cout << "Circulant matrix after addition: " << vcl_circulant << std::endl << std::endl;

  //
  // Manipulate single entry
  //
  std::cout << "Hankel matrix before manipulation: " << vcl_hankel << std::endl << std::endl;
  vcl_hankel(1, 2) = ScalarType(3.14);
  std::cout << "Hankel matrix after manipulation: " << vcl_hankel << std::endl << std::endl;

  std::cout << "Vandermonde matrix before manipulation: " << vcl_vandermonde << std::endl << std::endl;
  vcl_vandermonde(1) = ScalarType(1.1); //NOTE: Write access only via row index
  std::cout << "Vandermonde matrix after manipulation: " << vcl_vandermonde << std::endl << std::endl;

  //
  // Compute matrix-vector product (FFT-accelerated)
  //
  std::cout << "Toeplitz matrix: " << vcl_toeplitz << std::endl;
  std::cout << "Vector: " << vcl_vec << std::endl << std::endl;
  vcl_result = viennacl::linalg::prod(vcl_toeplitz, vcl_vec);
  std::cout << "Result of matrix-vector product: " << vcl_result << std::endl << std::endl;

  //
  //  That's it.
  //
  std::cout << "!!!! TUTORIAL COMPLETED SUCCESSFULLY !!!!" << std::endl;

  return EXIT_SUCCESS;
}
