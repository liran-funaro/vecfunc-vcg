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
#ifndef CATEGORY_TREE_HPP_
#define CATEGORY_TREE_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <queue>
#include <type_traits>
#include <vector>
#include <map>

#include <bitset>
#include <iostream>
#include <climits>


#include "upper_bound_ds.hpp"
#include "binary_search_tree.hpp"
#include "multi_binary_search_tree.hpp"


namespace UpperBoundDS {

std::vector<unsigned int> popcount(unsigned int n) {
    std::bitset<sizeof(unsigned int) * CHAR_BIT> b(n);
    std::vector<unsigned int> ret;
    for(unsigned int i=0; ret.size() < b.count(); i++) {
    	if (b.test(i))
    		ret.push_back(i);
    }

    return ret;
}


template<typename T, typename S, unsigned int D>
class CategoryTree : public BaseUpperBoundRangeDS<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    using f1_ds = UpperBound1DF<T,S,D>;
    using f2_ds = UpperBoundBinarySearchTree2DF<T,S,D>;
//    using f_all_ds = UpperBoundRangeTreeF2Partial<T,S,D>;
    using f_all_ds = MultiBinarySearchTree<T,S,D, 2>;

    point_vec minimum;

    std::vector<f1_ds> f1;
    std::vector<f2_ds> f2;
    std::vector<f_all_ds> f_all;
    std::vector<unsigned int> take_all;

    std::map<unsigned int, std::vector<p_point>> m;

public:
    CategoryTree(const shared_points& pts, unsigned int chunkSize) :
    			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize) {
    	findPointsMinimum();
    	allocateToCategories();

    	for (auto& it : m) {
    		auto r = it.first;
    		auto count = it.second.size();
    		if (count == 0) {
    			continue;
    		}
			if (count <= chunkSize || r == 0) {
				take_all.push_back(r);
				continue;
			}

    		auto idx = popcount(r);

#ifdef DEBUG
    		std::cout << "IDX(" << r << ", " << count << "): ";
    		for (unsigned int t=0; t < idx.size(); t++) {
    			std::cout << idx[t] << " ";
    		}
    		std::cout << std::endl;
#endif

    		shared_points sr(this->p_pts, this->p_helper_arr, it.second.data(), count);
    		switch(idx.size()) {
    		case 0:
    			take_all.push_back(r);
    			break;
    		case 1:
				f1.push_back(f1_ds(sr, chunkSize, idx[0]));
    			break;
    		case 2:
				f2.push_back(f2_ds(sr, chunkSize, idx[0], idx[1]));
				break;
    		default:
//    			std::random_shuffle(idx.begin(), idx.end());
//    			idx.resize(6);
				f_all.push_back(f_all_ds(sr, chunkSize, idx));
				break;
    		}
    	}
    }

private:
    void findPointsMinimum() {
    	for (unsigned int d=0; d<D; d++) {
    		switch(d%3) {
    		case 1:
				minimum[d] = -MAX_VALUE;
				break;
    		case 0:
    		case 2:
    		default:
    			minimum[d] = 0;
    			break;
    		}
    	}

//    	auto pts = this->p_pts.get();
//    	minimum = pts[0]->vector;
//		for (unsigned int i = 1; i < this->size; i++)
//			minimum.min(pts[i]->vector);

//    	auto pts = this->p_pts.get();
//    	unsigned int mid = (unsigned int)(0.9 * this->size);
//		for(unsigned int d=0; d < D; d++) {
//			auto p = this->partitionPointsByDim(pts, d, mid, 0, this->size);
//			minimum[d] = p->vector[d];
//		}

		minimum.nextafter();
    }

    void allocateToCategories() {
		auto pts = this->p_pts.get();
		for (unsigned int i = 0; i < this->size; i++) {
			unsigned int r = getPointIndex(pts[i]->vector);
			m[r].push_back(pts[i]);
		}
	}

    unsigned int getPointIndex(const point_vec& p) {
    	unsigned int r = 0;

    	for (unsigned int d=0; d < D; d++) {
    		if (p[d] > minimum[d])
    			r |= 1<<d;
    	}

    	return r;
    }

public:
    unsigned int query(const point_vec& upper) {
		unsigned int resultCount = 0;
		for (auto& it : take_all)
			resultCount += m[it].size();
		for (auto& it : f1)
			resultCount += it.query(upper);
		for (auto& it : f2)
			resultCount += it.query(upper);
		for (auto& it : f_all)
			resultCount += it.query(upper);
		return resultCount;
	}

	template <bool FILTER>
	unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
		unsigned retCount = 0;

		for (auto& it : take_all) {
			for (auto& vecIt : m[it])
				retCount = this->template appendResultPoint<FILTER>(ret, retCount, vecIt, upper);
		}

		for (auto& it : f1)
			retCount += it.template fetchQuery<FILTER>(upper, ret+retCount);
		for (auto& it : f2)
			retCount += it.template fetchQuery<FILTER>(upper, ret+retCount);
		for (auto& it : f_all)
			retCount += it.template fetchQuery<FILTER>(upper, ret+retCount);

		return retCount;
	}
};

} // UpperBoundDS

#endif //CATEGORY_TREE_HPP_
