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
#ifndef MULTI_BINARY_SEARCH_TREE_HPP_
#define MULTI_BINARY_SEARCH_TREE_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <utility>
#include <queue>
#include <type_traits>
#include <iostream>

#include "upper_bound_ds.hpp"


namespace UpperBoundDS {


template<typename T, typename S, unsigned int D, unsigned int SD = D-1>
class MultiBinarySearchTree : public BaseUpperBoundRangeDS<T,S,D> {
	static_assert(SD > 0, "SD must be at least 1.");

public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    std::unique_ptr<T[]> sortedD;

    std::unique_ptr<unsigned int[]> splits;
    unsigned int splitCount = 0;

    unsigned int cmpDim[D];
    unsigned int cmpDimCount;

    unsigned int subD[D][SD];
    unsigned int subDimCount;

public:
    MultiBinarySearchTree(const shared_points& pts, unsigned int chunkSize,
    		const std::vector<unsigned int>& cmpDim) :
    			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize) {
        cmpDimCount = cmpDim.size();
        for (unsigned int i=0; i < cmpDimCount; i++)
        	this->cmpDim[i] = cmpDim[i];

        init();
    }

    MultiBinarySearchTree(const shared_points& pts, unsigned int chunkSize) :
				BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize) {
		cmpDimCount = D;
		for (unsigned int i=0; i < cmpDimCount; i++)
			this->cmpDim[i] = i;

		init();
	}

private:
    void init() {
    	this->res.init(this->maxDepth+2);

		this->allocHelperArrays((this->maxDepth+1) * D * SD);
		sortedD.reset(new T[this->size * D]);

		subDimCount = std::min(cmpDimCount-1, SD);

		buildTree();
    }

private:
	inline unsigned int dimHelperArray(unsigned int depth, unsigned dim, unsigned int subDim) {
		return ((D*SD) * depth) + (SD * dim) + subDim;
	}

	inline T* getSortedArray(unsigned int d) {
		return sortedD.get() + (this->size * d);
	}

	// Initiate the sub dim for each sub dim index.
	// VERIFIED: (D*SD)
	void buildSubD() {
		for(unsigned int i=0; i < cmpDimCount; i++) {
			auto d = cmpDim[i];
			auto m = d % (subDimCount+1);
			unsigned int j = (i-m) % cmpDimCount;
			for (unsigned int sdIdx = 0; sdIdx < subDimCount; j = (j + 1) % cmpDimCount) {
				if (i == j)
					continue;
				subD[d][sdIdx++] = cmpDim[j];
			}
		}
	}

    void buildTree() {
    	buildSubD();
    	this->buildSplits(splits, splitCount);

        // O(D) * buildTreeNoRec()
    	for(unsigned int i=0; i < cmpDimCount; i++)
			buildTreeNoRec(cmpDim[i]);

    	splits.release();
    }

    inline void pointArrMergeSort(unsigned int mainD) {
		unsigned int helperInd = dimHelperArray(0, mainD, 0);
		auto arr = this->helperArray(helperInd);
		for (unsigned int i=0; i < splitCount; i++)
			this->sortPointsByDim(arr+splits[i], arr+splits[i+1], mainD);

		unsigned int splitJump = 1;
		for (unsigned int depth=0; depth < this->maxDepth; depth++) {
			helperInd = dimHelperArray(depth, mainD, 0);
			auto arrSrc = this->helperArray(helperInd);
			helperInd = dimHelperArray(depth+1, mainD, 0);
			auto arrDst = this->helperArray(helperInd);
			for (unsigned int i = 0; i < splitCount; i += 2 * splitJump) {
				auto left = arrSrc + splits[i];
				auto mid = arrSrc + splits[i + splitJump];
				auto top = arrSrc + splits[i + 2 * splitJump];
				auto dst = arrDst + splits[i];
				this->mergePointsByDim(left, mid, mid, top, dst, mainD);
			}

			splitJump *= 2;
		}
    }

    void buildTreeNoRec(unsigned int mainD) {
		auto helperInd = dimHelperArray(0, mainD, 0);
		this->fillHelperArray(helperInd);
		pointArrMergeSort(mainD);
//		this->sortPointsByDim(helper_arr, helper_arr+this->size, d);

		helperInd = dimHelperArray(this->maxDepth, mainD, 0);
		auto arr = this->helperArray(helperInd);
		T* sorted_arr = getSortedArray(mainD);
		for (unsigned int i=0; i<this->size; i++)
			sorted_arr[i] = arr[i]->vector[mainD];

		for (unsigned int sdInd=1; sdInd < subDimCount; sdInd++) {
			auto subHelperInd = dimHelperArray(this->maxDepth, mainD, sdInd);
			this->copyHelperArray(helperInd, subHelperInd);
		}

		for (unsigned int sdInd=0; sdInd < subDimCount; sdInd++) {
			unsigned int sd = subD[mainD][sdInd];
			helperInd = dimHelperArray(this->maxDepth, mainD, sdInd);
			arr = this->helperArray(helperInd);

			for (unsigned int i=0; i < splitCount; i++)
				this->sortPointsByDim(arr+splits[i], arr+splits[i+1], sd);

			unsigned int splitJump = 1;
			for (unsigned int depth=this->maxDepth; depth > 0; depth--) {
				helperInd = dimHelperArray(depth, mainD, sdInd);
				auto arr = this->helperArray(helperInd);
				helperInd = dimHelperArray(depth-1, mainD, sdInd);
				auto arrDst = this->helperArray(helperInd);
				for (unsigned int i=0; i < splitCount; i += 2*splitJump) {
					auto left = arr + splits[i];
					auto mid = arr + splits[i+splitJump];
					auto top = arr + splits[i+2*splitJump];
					auto dst = arrDst + splits[i];
					this->mergePointsByDim(left, mid, mid, top, dst, sd);
				}

				splitJump *= 2;
			}
		}
	}

    inline unsigned int findLeftMostBinarySearch(unsigned int mainD, unsigned int depth,
			unsigned int lo, unsigned int & hi,
			const point_vec& upper) {
    	if (SD == 1 || subDimCount == 1) {
    		auto sd = subD[mainD][0];
			auto helperInd = dimHelperArray(depth, mainD, 0);
    		hi = this->binarySearchUpperHelperByDim(helperInd, lo, hi, upper, sd);
    		return 0;
    	}

		// Optimization:
		// Check first if all the dims have value lower than upper.
		for (unsigned int sdIdx = 0; sdIdx < subDimCount; sdIdx++) {
			auto sd = subD[mainD][sdIdx];
			auto helperInd = dimHelperArray(depth, mainD, sdIdx);
			auto arr = this->helperArray(helperInd);
			if (!(arr[lo]->vector[sd] < upper[sd])){
				hi = lo;
				return sdIdx;
			}
		}

		unsigned int participatingDims1[SD];
		unsigned int participatingDims2[SD];
		unsigned int * oldParticipatingDims = participatingDims1;
		unsigned int * newParticipatingDims = participatingDims2;
		unsigned int oldParticipatingDimsCount = subDimCount;

		for (unsigned int sdIdx = 0; sdIdx < oldParticipatingDimsCount; sdIdx++)
			oldParticipatingDims[sdIdx] = sdIdx;

		// Multidimensional binary search in which we only continue with the
		// dimensions that continues left (less).
		while ((oldParticipatingDimsCount > 1) && (lo < hi)) {
			unsigned int mid = this->calcMid(lo, hi);

			unsigned int newParticipatingDimsCount = 0;
			for (unsigned int i = 0; i < oldParticipatingDimsCount; i++) {
				auto sdIdx = oldParticipatingDims[i];
				auto sd = subD[mainD][sdIdx];
				auto helperInd = dimHelperArray(depth, mainD, sdIdx);
				auto arr = this->helperArray(helperInd);
				if (!(arr[mid]->vector[sd] < upper[sd]))
					newParticipatingDims[newParticipatingDimsCount++] = sdIdx;
			}

			if (newParticipatingDimsCount > 0) {
				std::swap(oldParticipatingDims, newParticipatingDims);
				oldParticipatingDimsCount = newParticipatingDimsCount;
				hi = mid; // Go left
			} else
				// If all dims went right, then continue right
				lo = mid+1;
		}

		auto sdIdx = oldParticipatingDims[0];
		if (lo < hi) {
			auto sd = subD[mainD][sdIdx];
			auto helperInd = dimHelperArray(depth, mainD, sdIdx);
			hi = this->binarySearchUpperHelperByDim(helperInd, lo, hi, upper, sd);
		}

		return sdIdx;
	}

public:
    inline void findLeftMost(const point_vec& upper,
    		unsigned int & retL, unsigned int & retH,
			unsigned int & retDim, unsigned int & retDepth) {
    	unsigned int depth = 0;
    	unsigned int lo = 0;
    	unsigned int hi = this->size;

    	// Default return values
    	retL = 0;
    	retH = 0;
    	retDim = cmpDim[0];
		retDepth = 0;

    	// Optimization:
		// Check first if all the dims have value lower than upper.
		for(unsigned int i=0; i < cmpDimCount; i++) {
			auto d = cmpDim[i];
			auto s = this->getSortedArray(d);
			if (!(s[0] < upper[d]))
				return;
		}

		unsigned int participatingDims1[D];
		unsigned int participatingDims2[D];
		unsigned int * oldParticipatingDims = participatingDims1;
		unsigned int * newParticipatingDims = participatingDims2;
		unsigned int oldParticipatingDimsCount = cmpDimCount;

		for (unsigned int i=0; i<oldParticipatingDimsCount; i++)
			oldParticipatingDims[i] = cmpDim[i];

		// Multidimensional binary search in which we only continue with the
		// dimensions that continues left (less).
		while ((depth < this->maxDepth) && (oldParticipatingDimsCount > 1) && (lo != hi)) {
			unsigned int mid = this->calcMid(lo, hi);

			unsigned int newParticipatingDimsCount = 0;
			for (unsigned int i = 0; i < oldParticipatingDimsCount; i++) {
				auto d = oldParticipatingDims[i];
				auto s = this->getSortedArray(d);
				if (!(s[mid] < upper[d]))
					newParticipatingDims[newParticipatingDimsCount++] = d;
			}

			if (newParticipatingDimsCount > 0) {
				std::swap(oldParticipatingDims, newParticipatingDims);
				oldParticipatingDimsCount = newParticipatingDimsCount;
				hi = mid+1; // Go left
			} else {
				this->res.pushRange(lo, mid+1, depth+1);
				lo = mid + 1; // Go right
			}

			depth++;
		}

		retL = lo;
		retH = hi;
		retDim = oldParticipatingDims[0];
		retDepth = depth;
	}

    unsigned int query(const point_vec& upper) {
    	this->res.reset();
    	unsigned int l, h, d, depth;
    	findLeftMost(upper, l, h, d, depth);

        T pivot = upper[d];

        auto curSortedD = getSortedArray(d);
        // Multidimensional binary search in which we only continue with the
        // dimensions that continues left (less).
        while (depth < this->maxDepth && l != h) {
            // Optimization
            if (curSortedD[h-1] < pivot)
            	// If the right most is smaller, than we include all the points in the range.
                break;
            if (curSortedD[l] >= pivot) {
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
            if(curSortedD[mid] < pivot) {
            	this->res.pushRange(l, mid+1, depth+1); // Add left
                l = mid+1; // Go right
            } else
                h = mid+1; // Go left

            depth++;
        }

        if (l != h)
        	this->res.pushRange(l, h, depth);

        const unsigned int c = this->res.getRangeCount();
        for (unsigned int i=0; i < c; i++) {
        	const auto& r = this->res.popRange();
        	unsigned int hi = r.hi;

        	unsigned int sdIdx = findLeftMostBinarySearch(d, r.depth, r.lo, hi, upper);
			if (hi > r.lo) {
				auto helperInd = dimHelperArray(r.depth, d, sdIdx);
				auto sd = subD[d][sdIdx];
				this->res.pushRange(r.lo, hi, helperInd, sd);
			}
        }

        return this->res.getPointCount();
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        unsigned int retCount = 0;

        while (!this->res.empty()) {
			auto& r = this->res.popRange();
			retCount = this->template appendMultipleResultPoint<FILTER>(r.depth, r.lo, r.hi, ret, retCount, upper);
        }

        return retCount;
    }
};


template<typename T, typename S, unsigned int D>
using MultiBinarySearchTreeFull = MultiBinarySearchTree<T,S,D>;

template<typename T, typename S, unsigned int D>
using MultiBinarySearchTreeSingle = MultiBinarySearchTree<T,S,D,1>;

template<typename T, typename S, unsigned int D>
using MultiBinarySearchTreeDouble = MultiBinarySearchTree<T,S,D,2>;


} // UpperBoundDS

#endif //MULTI_BINARY_SEARCH_TREE_HPP_
