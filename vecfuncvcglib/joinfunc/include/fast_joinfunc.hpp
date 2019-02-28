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

/*
Fast join of two functions use the following condition

Condition for point A, B and result as R
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
           B.up <= A.down
           A.up <= B.down
  A.ind + B.ind <  R.size

We reorder the equations so that only B will be on the left side.

        B.up <= A.down
      B.down >= A.up
       B.ind <  R.size - A.ind

Finally, to normalize all the equations to the same form:
=>   -B.down <= -A.up => MAX-B.down <= MAX-A.up
=>
        B.up <= A.down
  MAX-B.down <= MAX-A.up
       B.ind <  R.size - A.ind
*/
#ifndef FAST_JOINFUNC_HPP_
#define FAST_JOINFUNC_HPP_

#include <iomanip>
#include <numeric>
#include <memory>

#include <debug.h>
#include <vcg_stats.hpp>
#include <jointvecfunc.hpp>
#include "brute_joinfunc.hpp"

//#define POINT_WITH_IND (((D) % 2) == 0)
#define POINT_WITH_IND (true)
#define POINT_DIM_MULTIPLY ((POINT_WITH_IND) ? (3) : (2))
#define POINT_DIM ((POINT_DIM_MULTIPLY)*(D))

#define MAX_VALUE (std::numeric_limits<T>::max() - 1)

#define EPS (std::numeric_limits<T>::epsilon())

template <typename T, unsigned int D,
	template<typename, typename, unsigned int> class UPPERBOUND_DS,
	unsigned int GRAD_INTERVAL = 1>
class FastJoinFunc : public BruteForceJoinFunc<T,D> {
public:
	using TDVecFunc = VecFunc<T,D>;
	using TDJoinedVecFunc = JointVecFunc<T,D>;
	using index = typename TDVecFunc::index;

	typedef struct {
        index ind;
		T val;
	} PointData;

	typedef UPPERBOUND_DS<T, PointData, POINT_DIM> join_val_ds;

	using TDPoint = typename join_val_ds::point;
	using TDPointVec = typename TDPoint::pointVec;

	typedef enum {UP=0, DOWN=1, IND=2} UpDown;

public:
	static inline T& access_point(TDPoint& p, unsigned int cur_dim, UpDown direction) {
		return access_point(p.vector, cur_dim, direction);
	}

	static inline T& access_point(TDPointVec& v, unsigned int cur_dim, UpDown direction) {
		return v[POINT_DIM_MULTIPLY*cur_dim + (unsigned int)direction];
	}

	static inline void get_up_down_val(const TDVecFunc& e, index& i, unsigned int cur_dim,
                                       T cur_val, T& up_val, T& down_val) {
		up_val = 0;
		down_val = MAX_VALUE;

		// Up
		if (i[cur_dim]+GRAD_INTERVAL < e.size[cur_dim]) {
			i[cur_dim] += GRAD_INTERVAL;
			up_val = e[i] - cur_val;
			i[cur_dim] -= GRAD_INTERVAL;
		}

		// Down
		if (i[cur_dim] > GRAD_INTERVAL-1) {
			i[cur_dim] -= GRAD_INTERVAL;
			down_val = cur_val - e[i];
			i[cur_dim] += GRAD_INTERVAL;
		}
	}

	template <bool FILTER_GRAD>
	static inline typename join_val_ds::shared_points create_points(const TDVecFunc& e,
						unsigned int res_vec_size) {
		auto ret_pts = std::shared_ptr<TDPoint>(new TDPoint[res_vec_size], std::default_delete<TDPoint[]>());
        TDPoint* pts = ret_pts.get();

        T up_val, down_val;

        unsigned int pts_count = 0;

        index i_e;
        FOR_EACH_MAT_INDEX(e, i_e) {
			auto e_ind = e.get_index(i_e);
//			if (FILTER_GRAD && e_ind == 0)
//				continue;

			auto e_val = e[e_ind];
			if (FILTER_GRAD && e_val < 0)
				continue;

			TDPoint& p = pts[pts_count++];
			p.val.ind = i_e;
			p.val.val = e_val;
			
			FOR_EACH_DIM(d) {
				get_up_down_val(e, i_e, d, e_val, up_val, down_val);
				if (FILTER_GRAD && down_val < EPS) {
					--pts_count;
					break;
				}

				access_point(p, d, UP) = up_val;
				if (std::is_signed<T>::value)
					access_point(p, d, DOWN) = -down_val;
				else
					access_point(p, d, DOWN) = MAX_VALUE - down_val;
				if (POINT_WITH_IND)
					access_point(p, d, IND) = i_e[d];
			}
		}

        DEBUG_OUTPUT("Point DIM: " << POINT_DIM);

        typename join_val_ds::shared_points pts_object(ret_pts, pts_count);
		return pts_object;
	}

	template <bool FILTER_GRAD, bool BUILD_TIMING>
	static join_val_ds build_ds(const TDVecFunc& v, unsigned int chunkSize, VCGStats* stats) {
	    unsigned int vec_size = v.total_size();
	    STATS_INIT(stats_var);

		if (BUILD_TIMING)
			STATS_START(stats_var);
		auto pts = create_points<FILTER_GRAD>(v, vec_size);
		stats->dsPts += pts.size();
		stats->totalPts += vec_size;
		if (BUILD_TIMING)
			STATS_ADD_TIME(stats_var, stats->dsCreatePointsTime);
		join_val_ds r(pts, chunkSize);
		if(BUILD_TIMING)
			STATS_ADD_TIME(stats_var, stats->dsBuildTime);

		return r;
    }

	template <bool FILTER_GRAD, bool FILTER, bool BRUTE_OPT, bool COUNTERS, bool BUILD_TIMING, bool QUERY_TIMING>
	static void join_vecfunc(TDVecFunc& a, TDVecFunc& b, TDJoinedVecFunc& res,
			unsigned int chunkSize, VCGStats* stats __attribute__((unused))) {
		FastJoinFunc::reset_result_array(res);
		a.fix_rising();
		b.fix_rising();

		unsigned int b_vec_size = b.total_size();

		STATS_INIT(stats_var);

		DEBUG_OUTPUT("DS Build Start");
		join_val_ds r = build_ds<FILTER_GRAD, BUILD_TIMING>(b, chunkSize, stats);
		DEBUG_OUTPUT("DS Build End");

		typename join_val_ds::point_vec upper;

		auto p_resPts = std::unique_ptr<TDPoint*[]>(new TDPoint*[b_vec_size]);
		TDPoint** resPts = p_resPts.get();

		unsigned long expected = 0;
		unsigned long actual = 0;
		unsigned long actualInBound = 0;
		unsigned long actualEdge = 0;
		unsigned long bruteForce = 0;
		unsigned long bruteForceCount = 0;
		unsigned long totalCount = 0;


		index i_a, a_limit, b_limit;
		T up_val, down_val;
		a_limit = a.size;
		a_limit.min(res.size);
		FOR_EACH_INDEX(i_a, a_limit) {
			auto a_val = a[i_a];

			vec_dec(res.size, i_a, b_limit);
			b_limit.min(b.size);
			auto b_points_count = b_limit.size();
			if (BRUTE_OPT && b_points_count < 64) {
				FastJoinFunc::join_val_inner(i_a, a_val, b, b_limit, res);
				if (COUNTERS) {
					bruteForce += b_limit.size();
					bruteForceCount++;
				}
				continue;
			}

			if (QUERY_TIMING)
				STATS_START(stats_var);

			bool is_point_valid = true;
			FOR_EACH_DIM(d) {
				get_up_down_val(a, i_a, d, a_val, up_val, down_val);
				if (FILTER_GRAD && down_val < EPS) {
					is_point_valid = false;
					break;
				}

				access_point(upper, d, UP) = down_val;
				if (std::is_signed<T>::value) {
					access_point(upper, d, DOWN) = -up_val;
				} else {
					access_point(upper, d, DOWN) = MAX_VALUE - up_val;
				}
				if (POINT_WITH_IND)
					access_point(upper, d, IND) = b_limit[d]-1;
			}

			if (FILTER_GRAD && !is_point_valid)
				continue;

			upper.nextafter();

			if (COUNTERS)
				totalCount++;

	        unsigned int maxPtsCount = r.query(upper);
	        if (QUERY_TIMING)
	        	STATS_ADD_TIME(stats_var, stats->dsQueryTime);
	        if (COUNTERS)
	        	expected += maxPtsCount;

	        if (BRUTE_OPT && maxPtsCount >= b_points_count) {
				FastJoinFunc::join_val_inner(i_a, a_val, b, b_limit, res);
				if (COUNTERS) {
					bruteForce += b_points_count;
					bruteForceCount++;
				}
	        } else {
	        	if (QUERY_TIMING)
	        		STATS_START(stats_var);
				auto resCount = r.template fetchQuery<FILTER>(upper, resPts);
				if (QUERY_TIMING)
					STATS_ADD_TIME(stats_var, stats->dsQueryFetchTime);
				if (COUNTERS) {
					actual += resCount;
					actualInBound += resCount;
				}

				for (unsigned i_p=0; i_p<resCount; i_p++) {
					const TDPoint& p = *resPts[i_p];
					if (COUNTERS && b.is_edge(p.val.ind))
						actualEdge++;

					if (!p.val.ind.less(b_limit)) {
						if (COUNTERS)
							actualInBound--;
						continue;
					}

                    FastJoinFunc::join_val_check_point(i_a, a_val, p.val.ind, p.val.val, res);
				}
			}
		}

		if (COUNTERS) {
			unsigned long totalNonBruteForce = totalCount - bruteForceCount;
			stats->expectedComparedPoints += (double)expected / (double)totalCount;
			stats->comparedPoints += (double)actual / (double)totalNonBruteForce;
			stats->comparedInBoundPoints = (double)actualInBound / (double)totalNonBruteForce;
			stats->comparedEdgePoints = (double)actualEdge / (double)totalNonBruteForce;
			stats->comparedBruteForce += (double)bruteForce / (double)bruteForceCount;
			stats->bruteForceCount += (double)bruteForceCount;
			stats->totalQueries += totalCount;
		}
	}
};

#endif /* FAST_JOINFUNC_HPP_ */
