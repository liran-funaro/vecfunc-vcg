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
#ifndef UPPER_BOUND_TRANSFORMED_HPP_
#define UPPER_BOUND_TRANSFORMED_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <queue>

#include "upper_bound_ds.hpp"
#include "binary_search_tree.hpp"


namespace UpperBoundDS {

template<typename T, typename S, unsigned int D>
class TransformedBinarySearchTree : public BaseUpperBoundDataStruct<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using p_point_arr = typename BaseUpperBoundDataStruct<T,S,D>::p_point_arr;

    typedef UpperBoundRangeTree2D<double,p_point,2> TransformedTree;
    using TransformedPoint = typename TransformedTree::point;
    using p_TransformedPoint = typename TransformedTree::p_point;
    using TransformedVec = typename TransformedTree::point_vec;

private:
    std::shared_ptr<TransformedPoint> p_transformedPts;
    TransformedTree transformedTree;
    std::unique_ptr<p_TransformedPoint[]> p_transformedResPts;

private:
    TransformedVec getTransformedVec(const point_vec& v) {
        TransformedVec ret;
        // ret[0] = v.squareScalar();
        // ret[1] = v.area();
        ret[0] = 0;
        ret[1] = 0;
        for (unsigned int i=0; i<D; i++)
            ret[i%2] += v[i];
        return ret;
    }

public:
    TransformedBinarySearchTree(p_point_arr pts, unsigned int size, unsigned int chunkSize) :
			BaseUpperBoundDataStruct<T, S, D>(pts, size, chunkSize) {
        p_transformedPts = std::shared_ptr<TransformedPoint>(new TransformedPoint[size],
                                        std::default_delete<TransformedPoint[]>());

        p_transformedResPts.reset(new p_TransformedPoint[size]);

        auto transformedPts = p_transformedPts.get();

        auto l_pts = this->pointsArray();
        for (unsigned int i=0; i<size; i++) {
            transformedPts[i].val = l_pts + i;
            transformedPts[i].vector = getTransformedVec(l_pts[i].vector);
        }

        transformedTree.init(p_transformedPts, size, chunkSize, 0, 1);
    }

    unsigned int query(const point_vec& upper) {
        auto transformedUpper = getTransformedVec(upper);
        return transformedTree.query(transformedUpper);
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        auto transformedUpper = getTransformedVec(upper);
        auto transformedResPts = p_transformedResPts.get();
        auto transformedCount = transformedTree.template fetchQuery<false>(transformedUpper, transformedResPts);

        unsigned int retCount = 0;
        for (unsigned int i=0; i<transformedCount; i++) {
        	retCount = this->template appendResultPoint<FILTER>(ret, retCount, transformedResPts[i]->val, upper);
        }
        return retCount;
    }
};


} // UpperBoundDS

#endif //UPPER_BOUND_TRANSFORMED_HPP_
