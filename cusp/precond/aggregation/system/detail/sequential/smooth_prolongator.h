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

#include <cusp/array1d.h>
#include <cusp/elementwise.h>
#include <cusp/execution_policy.h>
#include <cusp/format_utils.h>

#include <cusp/blas/blas.h>

namespace cusp
{
namespace precond
{
namespace aggregation
{
namespace detail
{

template <typename DerivedPolicy,
          typename MatrixType1,
          typename MatrixType2,
          typename MatrixType3,
          typename ValueType>
void smooth_prolongator(thrust::system::detail::sequential::execution_policy<DerivedPolicy> &exec,
                        const MatrixType1& S,
                        const MatrixType2& T,
                        MatrixType3& P,
                        const ValueType rho_Dinv_S,
                        const ValueType omega,
                        cusp::csr_format)
{
    typedef typename MatrixType3::index_type IndexType;

    cusp::array1d<ValueType, cusp::host_memory> D(S.num_rows);
    cusp::extract_diagonal(S, D);

    // create D_inv_S by copying S then scaling
    MatrixType3 D_inv_S(S);

    // scale the rows of D_inv_S by D^-1
    for ( size_t row = 0; row < D_inv_S.num_rows; row++ )
    {
        const IndexType row_start = D_inv_S.row_offsets[row];
        const IndexType row_end   = D_inv_S.row_offsets[row+1];
        const ValueType diagonal  = D[row];

        for ( IndexType index = row_start; index < row_end; index++ )
            D_inv_S.values[index] /= diagonal;
    }

    const ValueType lambda = omega / rho_Dinv_S;
    cusp::blas::scal( D_inv_S.values, lambda );

    MatrixType3 temp;
    cusp::multiply( D_inv_S, T, temp );
    cusp::subtract( T, temp, P );
}

template <typename DerivedPolicy,
          typename MatrixType1,
          typename MatrixType2,
          typename MatrixType3,
          typename ValueType>
void smooth_prolongator(thrust::system::detail::sequential::execution_policy<DerivedPolicy> &exec,
                        const MatrixType1& S,
                        const MatrixType2& T,
                        MatrixType3& P,
                        const ValueType rho_Dinv_S,
                        const ValueType omega,
                        cusp::known_format)
{
    typedef typename MatrixType1::index_type    IndexType;
    typedef typename MatrixType1::memory_space  MemorySpace;

    typedef typename MatrixType1::const_coo_view_type             CooViewType1;
    typedef typename MatrixType2::const_coo_view_type             CooViewType2;
    typedef typename cusp::detail::as_csr_type<MatrixType3>::type CsrType;

    CooViewType1 S_(S);
    CooViewType2 T_(T);
    CsrType P_;

    cusp::array1d<IndexType,MemorySpace> S_row_offsets(S.num_rows + 1);
    cusp::array1d<IndexType,MemorySpace> T_row_offsets(T.num_rows + 1);

    cusp::indices_to_offsets(S_.row_indices, S_row_offsets);
    cusp::indices_to_offsets(T_.row_indices, T_row_offsets);

    smooth_prolongator(exec,
                       cusp::make_csr_matrix_view(S.num_rows, S.num_cols, S.num_entries, S_row_offsets, S_.column_indices, S_.values),
                       cusp::make_csr_matrix_view(T.num_rows, T.num_cols, T.num_entries, T_row_offsets, T_.column_indices, T_.values),
                       P_,
                       rho_Dinv_S, omega, cusp::csr_format());

    cusp::convert(P_, P);
}

template <typename DerivedPolicy,
          typename MatrixType1,
          typename MatrixType2,
          typename MatrixType3,
          typename ValueType>
void smooth_prolongator(thrust::system::detail::sequential::execution_policy<DerivedPolicy> &exec,
                        const MatrixType1& S,
                        const MatrixType2& T,
                        MatrixType3& P,
                        const ValueType rho_Dinv_S,
                        const ValueType omega)
{
    typedef typename MatrixType1::format Format;

    Format format;

    smooth_prolongator(exec, S, T, P, rho_Dinv_S, omega, format);
}

} // end namespace detail
} // end namespace aggregation
} // end namespace precond
} // end namespace cusp

