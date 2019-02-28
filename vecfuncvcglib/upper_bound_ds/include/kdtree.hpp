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
#ifndef UPPER_BOUND_KDTREE_HPP_
#define UPPER_BOUND_KDTREE_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <vector>

#include "upper_bound_ds.hpp"


namespace UpperBoundDS {


template<typename T, typename S, unsigned int D, bool PARTIAL=false>
class KDTree : public BaseUpperBoundRangeDS<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    std::unique_ptr<T[]> medianArr;
    std::vector<unsigned int> cmpDim;

public:
    KDTree(const shared_points& pts, unsigned int chunkSize) :
			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize) {
    	if (PARTIAL) {
    		for (unsigned int d=0; d < D; d++)
    			this->cmpDim.push_back(d);
    	}
		this->res.init((1 << this->maxDepth) + 2);

		this->allocHelperArrays(1);
		medianArr.reset(new T[this->size]);
		buildTree();
	}

	KDTree(const shared_points& pts, unsigned int chunkSize, std::vector<unsigned int> cmpDim) :
			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize), cmpDim(cmpDim) {
        this->res.init((1 << this->maxDepth) + 2);

        this->allocHelperArrays(1);
        medianArr.reset(new T[this->size]);
        buildTree();
    }

private:
	inline unsigned int sortAxis(unsigned int depth) {
		if (PARTIAL)
			return cmpDim[depth % cmpDim.size()];
		else
			return depth % D;
	}

    void buildTree() {
    	this->fillHelperArray(0);
        for (unsigned int i=0; i<this->size; i++)
        	medianArr[i] = 0;

        buildTree(0, this->size, 0);
    }

    void buildTree(unsigned int l, unsigned int h, unsigned int depth) {
        if (h-l <= 1)
            return;

        auto axis = sortAxis(depth);
        if (depth == this->maxDepth) {
            this->sortHelperByDim(0, axis, l, h);
            return;
        }

        unsigned int mid = this->calcMid(l, h);
        p_point midPoint = this->partitionHelperByDim(0, axis, mid, l, h);
        medianArr[mid] = (*midPoint)[axis];

		// Left tree include mid point
        buildTree(l,     mid+1, depth+1); // Build left tree
        buildTree(mid+1, h,     depth+1); // Build right tree
    }

public:
    unsigned int query(const point_vec& upper) {
        this->res.reset();
        this->res.pushRange(0, this->size, 0);

        p_point* arr = this->helperArray(0);
        while (!this->res.empty() && this->res.lookupDepth() <= this->maxDepth) {
        	auto& r = this->res.popRange();
        	auto axis = sortAxis(r.depth);
        	if (r.depth == this->maxDepth) {
        		auto h = this->binarySearchUpper(arr, r.lo, r.hi, upper, axis);
        		if (h-r.lo > 0)
        			this->res.pushRange(r.lo, h, r.depth+1);
        	} else {
        		unsigned int mid = this->calcMid(r.lo, r.hi);

        		/*
				 * If the the middle point is smaller than the upper limit,
				 * than there might be some points on the right part that
				 * might also be smaller.
				 * We need to add the right range as well.
				 */
				if(medianArr[mid] < upper[axis])
					this->res.pushRange(mid+1, r.hi, r.depth+1);

				/*
				 * In any way, we need to add the left range as there might be smaller items
				 */
				this->res.pushRange(r.lo, mid+1, r.depth+1);
        	}
        }

        return this->res.getPointCount();
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        unsigned int retCount = 0;

        while (!this->res.empty()) {
        	auto& r = this->res.popRange();
			retCount = this->template appendMultipleResultPoint<FILTER>(0, r.lo, r.hi, ret,
					retCount, upper);
        }

        return retCount;
    }
};


template<typename T, typename S, unsigned int D>
using KDTreeFull = class KDTree<T,S,D,false>;


} // UpperBoundDS

#endif //UPPER_BOUND_KDTREE_HPP_
