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
#ifndef UPPER_BOUND_SCALAR_HPP_
#define UPPER_BOUND_SCALAR_HPP_

#include <cmath>
#include <memory>
#include <algorithm>

#include "upper_bound_ds.hpp"


namespace UpperBoundDS {


template<typename T, typename S, unsigned int D, double (*vecToScalar)(
		const typename BaseUpperBoundRangeDS<T, S, D>::point_vec&)>
class BaseUpperBoundScalar : public BaseUpperBoundRangeDS<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using p_point_arr = typename BaseUpperBoundDataStruct<T,S,D>::p_point_arr;

private:
    unsigned int res_h;

    class PointScalar {
    public:
        double s;
        p_point p;
    };

    std::unique_ptr<unsigned long[]> p_sortedScalar;

public:
    void build() {
        auto pts = this->pointsArray();
        std::unique_ptr<PointScalar[]> p_ps(new PointScalar[this->size]);
        auto ps = p_ps.get();

        for (unsigned int i=0; i<this->size; i++) {
            ps[i].p = pts + i;
            ps[i].s = vecToScalar(ps[i].p->vector);
        }

        std::sort(ps, ps+this->size,
            [](const PointScalar& a, const PointScalar& b) {
            return a.s < b.s;
        });

        auto sortedScalar = p_sortedScalar.get();
        auto dim_arr = this->helperArray(0);
        for (unsigned int i=0; i<this->size; i++) {
            dim_arr[i] = ps[i].p;
            sortedScalar[i] = ps[i].s;
        }
    }

public:
    BaseUpperBoundScalar(p_point_arr pts, unsigned int size, unsigned int chunkSize) :
    		BaseUpperBoundRangeDS<T,S,D>(pts, size, chunkSize), res_h(0) {
    	this->allocHelperArrays(1);
		p_sortedScalar.reset(new unsigned long[this->size]);
		this->build();
    }

    unsigned int query(const point_vec& upper __attribute__((unused))) {
        auto upperScaler = vecToScalar(upper);
        auto sortedScalar = p_sortedScalar.get();

        if (!(sortedScalar[0] < upperScaler))
            res_h = 0;
        else if (sortedScalar[this->size-1] < upperScaler)
            res_h = this->size;
        else
        	res_h = std::upper_bound(sortedScalar, sortedScalar+this->size,
        	                                 upperScaler) - sortedScalar;
        return res_h;
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        auto dim_arr = this->helperArray(0);
        unsigned int retCount = 0;
        for (unsigned int i=0; i<res_h; i++)
        	retCount = this->template appendResultPoint<FILTER>(ret, retCount, dim_arr[i], upper);

        return retCount;
    }
};


template<typename T, typename S, unsigned int D>
double vecToL1(const typename BaseUpperBoundRangeDS<T,S,D>::point_vec& v) {
	return v.L1Scalar();
}

template<typename T, typename S, unsigned int D>
double vecToL2(const typename BaseUpperBoundRangeDS<T,S,D>::point_vec& v) {
	return v.squareScalar();
}

template<typename T, typename S, unsigned int D>
double vecToMax(const typename BaseUpperBoundRangeDS<T,S,D>::point_vec& v) {
	return v.maximum();
}

template<typename T, typename S, unsigned int D>
using UpperBoundL1 = class BaseUpperBoundScalar<T,S,D, vecToL1<T,S,D> >;

template<typename T, typename S, unsigned int D>
using UpperBoundL2 = class BaseUpperBoundScalar<T,S,D, vecToL2<T,S,D> >;

template<typename T, typename S, unsigned int D>
using UpperBoundMax = class BaseUpperBoundScalar<T,S,D, vecToMax<T,S,D> >;


} // UpperBoundDS

#endif //UPPER_BOUND_SCALAR_HPP_
