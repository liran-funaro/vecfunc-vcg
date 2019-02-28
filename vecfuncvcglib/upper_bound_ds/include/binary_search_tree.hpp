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
#ifndef BINARY_SEARCH_TREE_HPP_
#define BINARY_SEARCH_TREE_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <utility>
#include <queue>
#include <type_traits>

#include "upper_bound_ds.hpp"
#include "fractional_cascading.hpp"


namespace UpperBoundDS {

/*
 * Query multidimensional points across a single dimension via binary search.
 */
template<typename T, typename S, unsigned int D>
class UpperBound1DF : public BaseUpperBoundRangeDS<T,S,D> {
public:
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    unsigned int cmpDim;
    unsigned int res_h = 0;

public:
    UpperBound1DF(const shared_points& pts, unsigned int chunkSize,
			unsigned int cmpDim) :
			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize), cmpDim(cmpDim) {
		this->allocHelperArrays(1);
		this->fillHelperArray(0);
		this->sortHelperByDim(0, cmpDim, 0, this->size);
	}

    unsigned int query(const point_vec& upper) {
    	auto arr = this->helperArray(0);
		if (!(arr[0]->vector[cmpDim] < upper[cmpDim]))
			res_h = 0;
		else if (arr[this->size-1]->vector[cmpDim] < upper[cmpDim])
			res_h = this->size;
		else
			res_h = this->binarySearchUpper(arr, 0, this->size, upper, cmpDim);
        return res_h;
    }

	template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        return this->template appendMultipleResultPoint<FILTER>(0, 0, res_h, ret, 0, upper);
    }
};


/**
 * Query multidimensional points across a all dimensions, one by one via binary search, any return
 * the one with the least results.
*/
template<typename T, typename S, unsigned int D>
class UpperBound1DFMulti : public BaseUpperBoundRangeDS<T,S,D> {
public:
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    unsigned int res_h = 0;
    unsigned int res_dim =0 ;

public:
    UpperBound1DFMulti(const shared_points& pts, unsigned int chunkSize) :
		BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize) {
		this->allocHelperArrays(D);
		for (unsigned int dim=0; dim<D; dim++) {
			this->fillHelperArray(dim);
			this->sortHelperByDim(dim, dim, 0, this->size);
		}
    }

    unsigned int query(const point_vec& upper) {
        // Optimization: Check first if all the dims have value lower than upper.
        for (unsigned int i=0; i<D; i++) {
        	auto arr = this->helperArray(i);
            if (!(arr[0]->vector[i] < upper[i])){
                res_h = 0;
                res_dim = 0;
                return 0;
            }
        }

        unsigned int dimsArr1[D], dimsArr2[D];
		unsigned int * participatingDims = dimsArr1;
		unsigned int participatingDimsCount = D;
		unsigned int * nextParticipatingDims = dimsArr2;

        for (unsigned int i=0; i<participatingDimsCount; i++)
            participatingDims[i] = i;

        unsigned int l = 0;
        unsigned int h = this->size;
        // Multidimensional binary search in which we only continue with the
        // dimensions that continues left (less).
        while ((participatingDimsCount > 1) && (l < h)) {
            unsigned int mid = this->calcMid(l, h);

            unsigned nextParticipatingDimsCount = 0;
            for (unsigned int i=0; i<participatingDimsCount; i++) {
                unsigned int curDim = participatingDims[i];
                auto arr = this->helperArray(curDim);
                if (!(arr[mid]->vector[curDim] < upper[curDim])) {
                	nextParticipatingDims[nextParticipatingDimsCount++] = curDim;
                }
            }

            if (nextParticipatingDimsCount > 0) {
            	std::swap(participatingDims, nextParticipatingDims);
            	participatingDimsCount = nextParticipatingDimsCount;

                h = mid; // Go left
            } else
                l = mid+1; // Go right
        }

        res_dim = participatingDims[0];
        if (l < h)
        	res_h = this->binarySearchUpperHelperByDim(res_dim, l, h, upper, res_dim);
		else
			res_h = h;

        return res_h;
    }

	template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        return this->template appendMultipleResultPoint<FILTER>(res_dim, 0, res_h,
        		ret, 0, upper);
    }
};


template<typename T, typename S, unsigned int D>
class UpperBoundBinarySearchTree2DF : public BaseUpperBoundRangeDS<T,S,D> {
	static_assert(D >= 2, "Dim must be at least 2.");

public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    unsigned int d1 = 0, d2 = 1;
    std::unique_ptr<T[]> sortedD1;

public:
    UpperBoundBinarySearchTree2DF(const shared_points& pts, unsigned int chunkSize,
    		unsigned int d1=0, unsigned int d2=1) :
    		BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize), d1(d1), d2(d2) {
    		this->init();
        }

    UpperBoundBinarySearchTree2DF() : BaseUpperBoundRangeDS<T, S, D>() {}

	void init(const shared_points& pts, unsigned int chunkSize,
			unsigned int d1, unsigned int d2) {
		this->baseInit(pts, chunkSize);
        this->d1 = d1;
        this->d2 = d2;
        this->init();
    }

private:
	void init() {
		this->res.init(this->maxDepth+2);

		this->allocHelperArrays(this->maxDepth+1);
		sortedD1.reset(new T[this->size]);
		buildTree();
	}

    void buildTree() {
    	std::unique_ptr<unsigned int[]> splits;
		unsigned int splitCount = 0;
    	this->buildSplits(splits, splitCount);

    	this->fillHelperArray(this->maxDepth);
		this->sortHelperByDim(this->maxDepth, d1, 0, this->size);

		p_point* helper_arr = this->helperArray(this->maxDepth);
		for (unsigned int i=0; i<this->size; i++)
			sortedD1[i] = helper_arr[i]->vector[d1];

    	auto arr = this->helperArray(this->maxDepth);
    	for (unsigned int i=0; i < splitCount; i++)
    		this->sortPointsByDim(arr+splits[i], arr+splits[i+1], d2);

    	unsigned int splitJump = 1;
    	for (unsigned int depth=this->maxDepth; depth > 0; depth--) {
    		auto arrSrc = this->helperArray(depth);
			auto arrDst = this->helperArray(depth-1);
    		for (unsigned int i=0; i < splitCount; i += 2*splitJump) {
    			auto left = arrSrc + splits[i];
				auto mid = arrSrc + splits[i + splitJump];
				auto top = arrSrc + splits[i + 2 * splitJump];
				auto dst = arrDst + splits[i];
				this->mergePointsByDim(left, mid, mid, top, dst, d2);
    		}

    		splitJump *= 2;
    	}
	}

	void addResultRange(const point_vec& upper, unsigned int lo, unsigned int hi,
			unsigned int depth) {
		hi = this->binarySearchUpperHelperByDim(depth, lo, hi, upper, d2);
		if (lo < hi)
			this->res.pushRange(lo, hi, depth);
	}

public:
    unsigned int query(const point_vec& upper) {
    	this->res.reset();
        unsigned int l = 0;
        unsigned int h = this->size;
        unsigned int depth = 0;

        T d1_pivot = upper[d1];
        auto sortedD1_raw = sortedD1.get();

        while (depth < this->maxDepth) {
            // Optimization
            if (sortedD1_raw[h-1] < d1_pivot)
            	// If the right most is smaller, than we include all the points in the range.
                break;
            if (!(sortedD1_raw[l] < d1_pivot)) {
            	// If the left most is bigger, than we don't include anything from that range.
                l = h;
                break;
            }

            unsigned int mid = this->calcMid(l, h);

            /*
			 * If the the middle point is smaller than the upper limit,
			 * than there might be some points on the right part that
			 * might also be smaller.
			 * We need to add the right range as well.
			 */
            if(sortedD1_raw[mid] < d1_pivot) {
                addResultRange(upper, l, mid+1, depth+1); // Add left
                l = mid+1; // Go right
            } else
                h = mid+1; // Go left

            depth++;
        }

        if (l < h)
            addResultRange(upper, l, h, depth);

        return this->res.getPointCount();
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        unsigned int retCount = 0;

        while (!this->res.empty()) {
			auto& r = this->res.popRange();
			retCount = this->template appendMultipleResultPoint<FILTER>(r.depth, r.lo, r.hi, ret,
					retCount, upper);
        }


        return retCount;
    }
};


template<typename T, typename S, unsigned int D, class RANGETREE_2D, unsigned int J=2>
class UpperBoundBinarySearchTree2DFMuti : public BaseUpperBoundDataStruct<T,S,D> {

public:
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

protected:
    RANGETREE_2D q[D];
    unsigned int qCount = 0;
    unsigned int bestResult = 0;

public:
    UpperBoundBinarySearchTree2DFMuti(const shared_points& pts, unsigned int chunkSize,
    		std::vector<unsigned int> cmpDim) :
			BaseUpperBoundDataStruct<T, S, D>(pts, chunkSize) {
		for (unsigned int i = 0; i < cmpDim.size(); i += J)
			q[qCount++].init(pts, chunkSize, cmpDim[i], cmpDim[(i+1)%cmpDim.size()]);
    }

    UpperBoundBinarySearchTree2DFMuti(const shared_points& pts, unsigned int chunkSize) :
			BaseUpperBoundDataStruct<T, S, D>(pts, chunkSize) {
		for (unsigned int i = 0; i < D; i += J) {
			unsigned int j = 0;
			auto m = i%3;
			switch(m) {
			case 0:
				j = (i + 1) % D;
				break;
			default:
				j = (i-m) % D;
				break;
			}
			q[qCount++].init(pts, chunkSize, i, j);
		}
	}

    unsigned int query(const point_vec& upper) {
        unsigned int count = this->size+1;
        bestResult = 0;

		for (unsigned int i = 0; i < qCount; i++) {
            auto c = q[i].query(upper);
            if (c < count) {
                count = c;
                bestResult = i;
            }
        }

        return count;
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        return q[bestResult].template fetchQuery<FILTER>(upper, ret);
    }
};


template<typename T, typename S, unsigned int D>
using UpperBoundRangeTreeF2Partial = class UpperBoundBinarySearchTree2DFMuti<T,S,D,UpperBoundBinarySearchTree2DF<T,S,D>, 2>;

template<typename T, typename S, unsigned int D>
using UpperBoundRangeTreeF2Conseq = class UpperBoundBinarySearchTree2DFMuti<T,S,D,UpperBoundBinarySearchTree2DF<T,S,D>, 1>;


template<typename T, typename S, unsigned int D>
using UpperBoundRangeTreeF2FCPartial = class UpperBoundBinarySearchTree2DFMuti<T,S,D,UpperBoundRangeTree2DFC<T,S,D>, 2>;

template<typename T, typename S, unsigned int D>
using UpperBoundRangeTreeF2FCConseq = class UpperBoundBinarySearchTree2DFMuti<T,S,D,UpperBoundRangeTree2DFC<T,S,D>, 1>;

} // UpperBoundDS

#endif //BINARY_SEARCH_TREE_HPP_
