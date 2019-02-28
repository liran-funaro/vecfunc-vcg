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
#ifndef JOINFUNC_HPP
#define JOINFUNC_HPP

#include <debug.h>

#include <jointvecfunc.hpp>
#include <vcg_stats.hpp>
#include "brute_joinfunc.hpp"
#include "fast_joinfunc.hpp"

#include <upper_bound_ds.hpp>
#include <binary_search_tree.hpp>
#include <kdtree.hpp>
#include <multi_binary_search_tree.hpp>
#include <category_tree.hpp>

//#include <upper_bound_transformed.hpp>
//#include <upper_bound_scalar.hpp>
//#include <upper_bound_randtree.hpp>


#define JOIN_VECFUNC_CASE(id, DS, DESC) \
    case (id): \
        DEBUG_OUTPUT("USING: " << #DS); \
        stats->method = #DESC; \
        FastJoinFunc<T,D,DS,G>::template join_vecfunc<FLAGS...>(a, b, res, chunkSize, stats); \
        break


#define JOIN_VECFUNC_ALL_VALID_CASES \
		JOIN_VECFUNC_CASE(1, SimpleUpperBoundDataStruct, Simple); \
		JOIN_VECFUNC_CASE(2, UpperBound1DFMulti, 1D Binary Search); \
		JOIN_VECFUNC_CASE(3, UpperBoundRangeTreeF2Partial, 2D Binary Search Tree); \
		JOIN_VECFUNC_CASE(4, UpperBoundRangeTreeF2FCPartial, 2D Binary Search Tree (FC)); \
		JOIN_VECFUNC_CASE(5, CategoryTree, Category Tree); \
        JOIN_VECFUNC_CASE(6, KDTreeFull, K-D Tree); \
        JOIN_VECFUNC_CASE(7, MultiBinarySearchTreeFull, Multi 2D Binary Search Tree (Full)); \
        JOIN_VECFUNC_CASE(8, MultiBinarySearchTreeSingle, Multi 2D Binary Search Tree (Single));

#if DIM > 1 || POINT_WITH_IND
#define JOIN_VECFUNC_ALL_CASES \
	JOIN_VECFUNC_ALL_VALID_CASES \
	JOIN_VECFUNC_CASE(9, MultiBinarySearchTreeDouble, Multi 2D Binary Search Tree (Double));
#else
#define JOIN_VECFUNC_ALL_CASES \
	JOIN_VECFUNC_ALL_VALID_CASES \
    JOIN_VECFUNC_CASE(9, MultiBinarySearchTreeSingle, Multi 2D Binary Search Tree (Single));
#endif



template<typename T, unsigned int D, unsigned int G = 1, bool ... FLAGS>
static void join_vecfunc(VecFunc<T, D>& a, VecFunc<T, D>& b, JointVecFunc<T, D>& res,
		unsigned int method __attribute__((unused)), unsigned int chunkSize,
		VCGStats* stats __attribute__((unused))) {
	using namespace UpperBoundDS;

	STATS_INIT(start_time);
	STATS_START(start_time);

    switch(method) {
    	JOIN_VECFUNC_ALL_CASES

        case 0:
        default:
        	DEBUG_OUTPUT("USING default: BruteForceJoinFunc");
        	stats->method = "Brute Force";
			BruteForceJoinFunc<T,D>::template join_vecfunc<true>(a, b, res, stats);
            break;
    }

    STATS_ADD_TIME(start_time, stats->totalRuntime);
	stats->joinedFuncCount++;
}


#undef JOIN_VECFUNC_CASE
#define JOIN_VECFUNC_CASE(id, DS, DESC) \
    case (id): \
        DEBUG_OUTPUT("USING: " << #DS); \
        stats->method = #DESC; \
        FastJoinFunc<T,D,DS,G>::template build_ds<false, true>(v, chunkSize, stats); \
        break


template<typename T, unsigned int D, unsigned int G = 1>
static void test_ds_build_time(const VecFunc<T, D>& v, unsigned int method, unsigned int chunkSize, VCGStats* stats) {
	using namespace UpperBoundDS;

	STATS_INIT(start_time);
	STATS_START(start_time);

    switch(method) {
    	JOIN_VECFUNC_ALL_CASES

        case 0:
        default:
        	DEBUG_OUTPUT("No output");
            break;
    }

    STATS_ADD_TIME(start_time, stats->totalRuntime);
    stats->joinedFuncCount++;
}


#endif //JOINFUNC_HPP
