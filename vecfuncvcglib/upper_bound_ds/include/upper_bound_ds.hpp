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
#ifndef UPPER_BOUND_DS_HPP_
#define UPPER_BOUND_DS_HPP_

#include <cmath>
#include <memory>
#include <algorithm>

#include <vec.hpp>


namespace UpperBoundDS {


// D dim point of type T with value of type S
template<typename T, typename S, unsigned int D>
class Point {
public:
	using pointVec = vec<D,T>;
	static const unsigned int dim = D;

public:
    pointVec vector;
    S val;

    inline const S& value() const { return val; }
    inline T operator[](int index) const { return vector[index]; }
    inline bool less(const pointVec& upper) const {
        return vector.less(upper);
    }
};


template<typename T, typename S, unsigned int D>
class SharedPoints {
public:
	typedef Point<T,S,D> point;
	typedef typename point::pointVec point_vec;
	typedef point* p_point;
	typedef std::shared_ptr<point> point_shared_arr;
	typedef std::shared_ptr<p_point[]> p_point_shared_arr;
	typedef p_point* p_point_arr;

private:
	point_shared_arr p_pts;
	unsigned int _size = 0;

	p_point_shared_arr p_points_shared_arr;
	p_point_arr ptr_arr;

public:
	SharedPoints() : ptr_arr(NULL) {}

	SharedPoints(const SharedPoints& pts) :
			p_pts(pts.p_pts), _size(pts._size), p_points_shared_arr(pts.p_points_shared_arr),
			ptr_arr(pts.ptr_arr) {
	}

	SharedPoints(const point_shared_arr& pts, unsigned int size) : p_pts(pts), _size(size) {
		initPointArray();
	}

	SharedPoints(const SharedPoints& pts, p_point_shared_arr p_points_shared_arr,
			p_point_arr ptr_arr, unsigned int size) : p_pts(pts.p_pts), _size(size),
					p_points_shared_arr(p_points_shared_arr), ptr_arr(ptr_arr) {
	}

	void init(const point_shared_arr& pts, unsigned int size) {
		this->p_pts = pts;
		this->_size = size;
		initPointArray();
	}

	p_point_arr get() {
		return ptr_arr;
	}

	unsigned int size() const {
		return this->_size;
	}

private:
	void initPointArray() {
		if (!this->p_pts)
			return;
		auto pts = this->p_pts.get();
		p_points_shared_arr.reset(new p_point[this->_size], std::default_delete<p_point[]>());
		ptr_arr = p_points_shared_arr.get();
		for (unsigned int i=0; i < this->_size; i++)
			ptr_arr[i] = pts + i;
	}
};


template<typename T, typename S, unsigned int D>
class BaseUpperBoundDataStruct {
public:
	using shared_points = SharedPoints<T,S,D>;
    using point = typename shared_points::point;
    using point_vec = typename point::pointVec;
    typedef point* p_point;

protected:
    shared_points p_pts;
    unsigned int size = 0;
    unsigned int maxDepth = 0;
    unsigned int chunkSize = 0;

public:
	BaseUpperBoundDataStruct(const shared_points& pts, unsigned int chunkSize) {
		baseInit(pts, chunkSize);
	}

    BaseUpperBoundDataStruct() {}
//    ~BaseUpperBoundDataStruct() = default;

    void baseInit(const shared_points& pts, unsigned int chunkSize) {
    	this->p_pts = pts;
    	this->size = pts.size();
    	this->chunkSize = chunkSize;
    	unsigned int log_n = (unsigned int) std::log2(size);
    	unsigned int log_chunk = (unsigned int) std::log2(chunkSize);
		this->maxDepth = log_n > log_chunk ? log_n - log_chunk : 0;
    }

public:
    template <bool FILTER>
    inline unsigned int appendResultPoint(p_point* ret, unsigned int retCount,
    		p_point pt, const point_vec& upper  __attribute__((unused))) {
    	if (!FILTER || pt->less(upper))
			ret[retCount++] = pt;
    	return retCount;
    }
};


class UpperBoundRangeDSResults {
public:
    typedef struct {
        unsigned int lo;
        unsigned int hi;
        unsigned int depth;
        unsigned int sortDim;
    } Range;

public:
    unsigned int sz;
    unsigned int back_it;
    unsigned int fwd_it;
    std::unique_ptr<Range[]> ranges;

    static const unsigned int None = UINT16_MAX;

public:
    void init(unsigned int sz) {
    	this->sz = sz;
        ranges.reset(new Range[sz]);
        reset();
    }

    void pushRange(unsigned int lo, unsigned int hi, unsigned int depth,
    		unsigned int sortDim=None) {
        Range& r = ranges[fwd_it];
        fwd_it = (fwd_it+1) % sz;

        r.lo = lo;
        r.hi = hi;
        r.depth = depth;
        r.sortDim = sortDim;
    }

    const Range& popRange() {
    	const Range& r = ranges[back_it];
    	back_it = (back_it+1) % sz;
    	return r;
    }

    const Range& lookupRange() const {
    	return ranges[back_it];
    }

    unsigned int lookupDepth() const {
    	return ranges[back_it].depth;
    }

    bool empty() const {
    	return back_it == fwd_it;
    }

    void reset() {
    	back_it = 0;
    	fwd_it = 0;
    }

    unsigned int getRangeCount() const {
    	if (fwd_it < back_it)
    		return (sz - back_it) + fwd_it;
    	else
    		return fwd_it - back_it;
    }

    unsigned int getPointCount() const {
    	unsigned int pointCount = 0;
    	for (unsigned int i=back_it; i != fwd_it; i = (i+1)%sz) {
    		pointCount += ranges[i].hi - ranges[i].lo;
    	}
        return pointCount;
    }
};


template<typename T, typename S, unsigned int D>
class BaseUpperBoundRangeDS : public BaseUpperBoundDataStruct<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;
    using p_point_shared_arr = typename BaseUpperBoundDataStruct<T,S,D>::shared_points::p_point_shared_arr;

protected:
    p_point_shared_arr p_helper_arr;
    UpperBoundRangeDSResults res;

public:
    BaseUpperBoundRangeDS(const shared_points& pts, unsigned int chunkSize) :
    	BaseUpperBoundDataStruct<T, S, D>(pts, chunkSize) {
	}
    BaseUpperBoundRangeDS() : BaseUpperBoundDataStruct<T, S, D>() {}

protected:
    inline void allocHelperArrays(unsigned int count) {
    	p_helper_arr.reset(new p_point[this->size * count], std::default_delete<p_point[]>());
    }

    inline p_point* helperArray(unsigned int d) {
        return p_helper_arr.get() + (this->size * d);
    }

    inline void fillPointsArray(p_point* arr) {
    	auto src_arr = this->p_pts.get();
        for (unsigned int i=0; i<this->size; i++)
            arr[i] = src_arr[i];
    }

    inline void fillHelperArray(unsigned int helperArr) {
    	auto arr = helperArray(helperArr);
    	fillPointsArray(arr);
	}

    inline void copyHelperArray(unsigned int srcHelperArr, unsigned int dstHelperArr) {
		auto srcArr = helperArray(srcHelperArr);
		auto dstArr = helperArray(dstHelperArr);
		for (unsigned int i=0; i<this->size; i++)
			dstArr[i] = srcArr[i];
	}

    inline void sortPointsByDim(p_point* s, p_point* e, unsigned int cmpDim) {
        std::sort(s, e, [cmpDim](const p_point a, const p_point b) {
            return a->vector[cmpDim] < b->vector[cmpDim];
        });
    }

	inline void sortHelperByDim(unsigned int helperArr, unsigned int cmpDim, unsigned int lo,
			unsigned int hi) {
    	auto arr = helperArray(helperArr);
    	sortPointsByDim(arr+lo, arr+hi, cmpDim);
	}

	// Partition an array such that the everything left of the
	// the item in the K position is smaller than it, and to the right is larger.
	// Returns: the point in the K position.
    inline p_point partitionHelperByDim(unsigned int helperArr, unsigned int cmpDim,
    		unsigned int k, unsigned int lo, unsigned int hi) {
    	auto arr = helperArray(helperArr);
    	return partitionPointsByDim(arr, cmpDim, k, lo, hi);
    }

    inline p_point partitionPointsByDim(p_point* arr, unsigned int cmpDim,
			unsigned int k, unsigned int lo, unsigned int hi) {
		std::nth_element(arr+lo, arr+k, arr+hi, [cmpDim](const p_point a, const p_point b) {
			return a->vector[cmpDim] < b->vector[cmpDim];
		});
		return arr[k];
	}

    // Merge sort iteration
    inline void mergePointsByDim(p_point* arrLeft, p_point* arrLeftTop,
    		p_point* arrRight, p_point* arrRightTop, p_point* dst, unsigned int d) {
//        std::merge(sa, ea, sb, eb, st, [cmpDim](const p_point a, const p_point b) {
//            return a->vector[cmpDim] < b->vector[cmpDim];
//        });
        while(arrLeft < arrLeftTop && arrRight < arrRightTop) {
				if ((*arrLeft)->vector[d] < (*arrRight)->vector[d]) {
				*dst = *arrLeft;
				arrLeft++;
			} else {
				*dst = *arrRight;
				arrRight++;
			}
			dst++;
		}

		while (arrLeft < arrLeftTop) {
			*dst = *arrLeft;
			arrLeft++;
			dst++;
		}

		while (arrRight < arrRightTop) {
			*dst = *arrRight;
			arrRight++;
			dst++;
		}
    }

	inline void mergeHelperArrayByDim(unsigned int helperArr1, unsigned int lo1, unsigned int hi1,
			unsigned int helperArr2, unsigned int lo2, unsigned int hi2, unsigned int dstHelperArr,
			unsigned int dstLo, unsigned int cmpDim) {
		auto arr1 = helperArray(helperArr1);
		auto arr2 = helperArray(helperArr2);
		auto dstArr = helperArray(dstHelperArr);
		mergePointsByDim(arr1+lo1, arr1+hi1, arr2+lo2, arr2+hi2, dstArr+dstLo, cmpDim);
	}

    inline unsigned int calcMid(unsigned int l, unsigned int h) {
        return ((h-l-1) / 2) + l;
    }

    inline unsigned int binarySearchUpper(const p_point* arr,
                unsigned int lo, unsigned int hi,
                const vec<D,T>& upper, unsigned int cmpDim) {
    	T pivot = upper[cmpDim];
    	auto lower = std::lower_bound(arr+lo, arr+hi, pivot, [cmpDim](const p_point a, const T p) {
            return a->vector[cmpDim] < p;
        });
    	return lower - arr;
    }

    inline unsigned int binarySearchUpperHelperByDim(unsigned int helperArr,
                    unsigned int lo, unsigned int hi,
                    const vec<D,T>& upper, unsigned int cmpDim) {
    	auto arr = helperArray(helperArr);
    	return binarySearchUpper(arr, lo, hi, upper, cmpDim);
    }

    // Create an array of splits to be merged
	// VERIFIED: O(N)
	void buildSplits(std::unique_ptr<unsigned int[]>& splits, unsigned int& splitCount) {
		splits.reset(new unsigned int[this->size]);
		std::unique_ptr<unsigned int[]> newSplits(new unsigned int[this->size]);
		splitCount = 1;
		splits[0] = 0;
		splits[1] = this->size;

		for (unsigned depth=0; depth < this->maxDepth; depth++) {
			unsigned int newSplitCount = 0;
			for (unsigned int i=0; i < splitCount; i++) {
				newSplits[newSplitCount++] = splits[i];
				newSplits[newSplitCount++] = this->calcMid(splits[i], splits[i+1])+1;
			}

			newSplits[newSplitCount] = splits[splitCount];
			splits.swap(newSplits);
			splitCount = newSplitCount;
		}
	}

    template <bool FILTER>
	inline unsigned int appendMultipleResultPoint(unsigned int helperArrayIdx,
			unsigned int lo, unsigned int hi,
			p_point* ret, unsigned int retCount, const point_vec& upper) {
    	auto arr = this->helperArray(helperArrayIdx);
		for (unsigned int i = lo; i < hi; i++)
			retCount = this->template appendResultPoint<FILTER>(ret, retCount, arr[i], upper);
		return retCount;
	}
};


template<typename T, typename S, unsigned int D>
class SimpleUpperBoundDataStruct : public BaseUpperBoundDataStruct<T,S,D>{
public:
	using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
	using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
	using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
	using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

public:
	SimpleUpperBoundDataStruct(const shared_points& pts, unsigned int chunkSize) :
			BaseUpperBoundDataStruct<T, S, D>(pts, chunkSize) {}
    ~SimpleUpperBoundDataStruct() = default;


    unsigned int query(const point_vec& upper __attribute__((unused))) {
		return 0;
	}

    template <bool FILTER>
	unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
		unsigned retCount = 0;

		auto arr = this->p_pts.get();
		for (unsigned int i=0; i<this->size; i++)
			retCount = this->template appendResultPoint<FILTER>(ret, retCount, arr[i], upper);

		return retCount;
	}
};


} // UpperBoundDS

#endif //UPPER_BOUND_DS_HPP_
