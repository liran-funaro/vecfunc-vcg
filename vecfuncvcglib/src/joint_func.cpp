/*
 * Author: Liran Funaro <liran.funaro@gmail.com>
 *
 * Copyright (C) 2006-2018 Liran Funaro
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <string>
#include <cstdint>

#include <stats.h>

#include <vecfunc_types.hpp>
#include <vcg_stats.hpp>
#include <joinfunclib.hpp>

typedef JointVecFunc<VALUE,DIM> TDJoinedVecFunc;


template<bool ... FLAGS>
VCGStats template_vcg_join(VALUE* val_a, uint32_t* size_a,
             VALUE* val_b, uint32_t* size_b,
             VALUE* val_res, uint32_t* arg_res, uint32_t* size_res,
             uint32_t method, uint32_t chunk_size) {
    TDVecFunc a(val_a, size_a);
    TDVecFunc b(val_b, size_b);
    TDJoinedVecFunc res(val_res, (TDJoinedVecFunc::index*)arg_res, size_res);
    VCGStats stats;
	join_vecfunc<VALUE, DIM, 1, FLAGS...>(a, b, res, method,
			chunk_size, &stats);
    return stats;
}


#define DEF_VCG_JOIN(N,...) \
	VCGStats vcg_join_##N(VALUE* val_a, uint32_t* size_a, \
				 VALUE* val_b, uint32_t* size_b, \
				 VALUE* val_res, uint32_t* arg_res, uint32_t* size_res, \
				 uint32_t method, uint32_t chunk_size) { \
		return template_vcg_join<__VA_ARGS__>(val_a, size_a, val_b, size_b, val_res, arg_res, size_res, \
				method, chunk_size); \
	}


// Shared library interface
extern "C" {

DEF_VCG_JOIN(nofilter,  false, false, false, false, false, false)
DEF_VCG_JOIN(filter,    false, true,  false, false, false, false)
DEF_VCG_JOIN(brute_opt, false, true,  true,  false, false, false)
DEF_VCG_JOIN(count,     false, true,  false, true,  false, false)
DEF_VCG_JOIN(buildtime, false, true,  false, true,  true,  false)
DEF_VCG_JOIN(querytime, false, true,  false, true,  true,  true )

DEF_VCG_JOIN(fg_nofilter,  true, false, false, false, false, false)
DEF_VCG_JOIN(fg_filter,    true, true,  false, false, false, false)
DEF_VCG_JOIN(fg_brute_opt, true, true,  true,  false, false, false)
DEF_VCG_JOIN(fg_count,     true, true,  false, true,  false, false)
DEF_VCG_JOIN(fg_buildtime, true, true,  false, true,  true,  false)
DEF_VCG_JOIN(fg_querytime, true, true,  false, true,  true,  true )

VCGStats vcg_test_ds_build_time(VALUE* val_v, uint32_t* size_v,
             uint32_t method, uint32_t chunk_size) {
    TDVecFunc v(val_v, size_v);
    VCGStats stats;
    test_ds_build_time(v, method, chunk_size, &stats);
    return stats;
}

} // extern "C"
