/*
 *  Copyright 2008-2014 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include <cusp/detail/config.h>

#include <cusp/coo_matrix.h>
#include <cusp/csr_matrix.h>
#include <cusp/hyb_matrix.h>
#include <cusp/multiply.h>
#include <cusp/linear_operator.h>

#include <cusp/eigen/spectral_radius.h>

#include <cusp/precond/diagonal.h>

namespace cusp
{
namespace precond
{
namespace aggregation
{
namespace detail
{

template <typename MatrixType>
struct Dinv_A : public cusp::linear_operator<typename MatrixType::value_type, typename MatrixType::memory_space>
{
    typedef typename MatrixType::value_type   ValueType;
    typedef typename MatrixType::memory_space MemorySpace;

    const MatrixType& A;
    const cusp::precond::diagonal<ValueType,MemorySpace> Dinv;

    Dinv_A(const MatrixType& A)
        : cusp::linear_operator<ValueType,MemorySpace>(A.num_rows, A.num_cols, A.num_entries + A.num_rows),
          A(A), Dinv(A)
    {}

    template <typename Array1, typename Array2>
    void operator()(const Array1& x, Array2& y) const
    {
        cusp::multiply(A,x,y);
        cusp::multiply(Dinv,y,y);
    }
};

template <typename MatrixType>
double estimate_rho_Dinv_A(const MatrixType& A)
{
    detail::Dinv_A<MatrixType> Dinv_A(A);

    return cusp::eigen::ritz_spectral_radius(Dinv_A, 8);
}

} // end namespace detail

template <typename IndexType, typename ValueType, typename MemorySpace>
struct select_sa_matrix_type
{
  typedef cusp::csr_matrix<IndexType,ValueType,MemorySpace> CSRType;
  typedef cusp::coo_matrix<IndexType,ValueType,MemorySpace> COOType;

  typedef typename thrust::detail::eval_if<
        thrust::detail::is_convertible<MemorySpace, cusp::host_memory>::value
      , thrust::detail::identity_<CSRType>
      , thrust::detail::identity_<COOType>
    >::type type;
};

template <typename MatrixType>
struct select_sa_matrix_view
{
  typedef typename MatrixType::memory_space MemorySpace;
  typedef typename MatrixType::format       Format;

  typedef typename thrust::detail::eval_if<
        thrust::detail::is_convertible<MemorySpace, cusp::host_memory>::value
      , typename thrust::detail::eval_if<
          thrust::detail::is_same<Format, cusp::csr_format>::value
          , thrust::detail::identity_<typename MatrixType::const_view>
          , cusp::detail::as_csr_type<MatrixType>
          >
      , thrust::detail::identity_<typename MatrixType::const_coo_view_type>
    >::type type;
};

template<typename MatrixType>
struct sa_level
{
    public:

    typedef typename MatrixType::index_type IndexType;
    typedef typename MatrixType::value_type ValueType;
    typedef typename MatrixType::memory_space MemorySpace;

    MatrixType A_; 					                              // matrix
    MatrixType T; 					                              // matrix
    cusp::array1d<IndexType,MemorySpace> aggregates;      // aggregates
    cusp::array1d<ValueType,MemorySpace> B;               // near-nullspace candidates

    size_t    num_iters;
    ValueType rho_DinvA;

    sa_level(void) : num_iters(1), rho_DinvA(0) {}

    template<typename SALevelType>
    sa_level(const SALevelType& L)
      : A_(L.A_),
        aggregates(L.aggregates), B(L.B),
        num_iters(L.num_iters), rho_DinvA(L.rho_DinvA)
    {}
};

} // end namespace aggregation
} // end namespace precond
} // end namespace cusp

