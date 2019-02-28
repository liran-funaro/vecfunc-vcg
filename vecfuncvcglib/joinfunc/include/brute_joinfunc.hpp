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
#ifndef BRUTE_JOINFUNC_HPP_
#define BRUTE_JOINFUNC_HPP_

#include <cstring>

#include <debug.h>
#include <vcg_stats.hpp>
#include <jointvecfunc.hpp>


template <typename T, unsigned int D>
class BruteForceJoinFunc {
public:
	static const unsigned int dim = D;

	using TDVecFunc = VecFunc<T,D>;
	using TDJoinedVecFunc = JointVecFunc<T,D>;
	using index = typename TDVecFunc::index;

	static inline void reset_result_array(TDJoinedVecFunc& res) {
	    auto res_vec_size = res.size.size();
	    std::memset((void*)res.m,   0, sizeof(*res.m)*res_vec_size);
	    std::memset((void*)res.arg, 0, sizeof(*res.arg)*res_vec_size);
	}

	static inline void join_val_check_point(const index& i_a, T a_val, const index& i_b, T b_val,
	                					    TDJoinedVecFunc& res) {
	    index i_res;
	    vec_add(i_a, i_b, i_res);
	    auto res_ind = res.get_index(i_res);
	    auto val = a_val + b_val;

	    if (res[res_ind] < val) {
	        res[res_ind] = val;
	        res.arg[res_ind] = i_a;
	    }
	}

	static inline void join_val_inner(const index& i_a, T a_val,
	                    const TDVecFunc& b, const index& b_limit,
	                    TDJoinedVecFunc& res) {
	    index i_b;
	    FOR_EACH_INDEX(i_b, b_limit) {
	        join_val_check_point(i_a, a_val, i_b, b[i_b], res);
	    }
	}

	template<bool COUNTERS>
	static void join_vecfunc(const TDVecFunc& a, const TDVecFunc& b, TDJoinedVecFunc& res,
			VCGStats* stats __attribute__((unused))) {
		reset_result_array(res);

		unsigned long combinationCount = 0;

		index i_a, a_limit, b_limit;
		a_limit = a.size;
		a_limit.min(res.size);
		FOR_EACH_INDEX(i_a, a_limit) {
			auto a_val = a[i_a];

			vec_dec(res.size, i_a, b_limit);
			b_limit.min(b.size);

			join_val_inner(i_a, a_val, b, b_limit, res);
			if (COUNTERS)
				combinationCount += b_limit.size();
		}

		if (COUNTERS)
			stats->comparedBruteForce += (double)combinationCount / (double)a.total_size();
	}
};


#endif /* BRUTE_JOINFUNC_HPP_ */
