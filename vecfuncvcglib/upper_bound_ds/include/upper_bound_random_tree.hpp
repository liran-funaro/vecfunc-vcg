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
#ifndef UPPER_BOUND_RANDOM_TREE_HPP_
#define UPPER_BOUND_RANDOM_TREE_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#include "upper_bound_ds.hpp"


namespace UpperBoundDS {

template<typename T, typename S, unsigned int D>
class UpperBoundNode {
public: 
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using p_point_arr = typename BaseUpperBoundDataStruct<T,S,D>::p_point_arr;

public:
    p_point pt;
    UpperBoundNode<T,S,D>* left;
    UpperBoundNode<T,S,D>* right;

    point_vec upper;
    point_vec lower;
    double ptToUpperDist;
    // double upperLowerDist;

    void init(p_point new_pt) {
        pt = new_pt;
        left = NULL;
        right = NULL;

        upper = new_pt->vector;
        lower = new_pt->vector;
        ptToUpperDist = 0;
        // upperLowerDist = 0;
    }

    void updateUpperLower(p_point new_pt) {
        upper.max(new_pt->vector);
        lower.min(new_pt->vector);
        ptToUpperDist = pt->vector.squareDist(upper);
        // upperLowerDist = upper.squareDist(lower);
    }

    void insert(p_point new_pt, UpperBoundNode<T,S,D>* newNode) {
        insert(this, new_pt, newNode);
    }

    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        if (!this->lower.less(upper)) {
            return 0;
        }

        if (this->upper.less(upper)) {
            return getTree(ret);
        }

        unsigned int retCount = 0;
        if (this->pt->less(upper))
            ret[retCount++] = this->pt;

        if (left)
            retCount += left->fetchQuery(upper, ret+retCount);

        if (right)
            retCount += right->fetchQuery(upper, ret+retCount);

        return retCount;
    }

    unsigned int getTree(p_point* ret) {
        unsigned int retCount = 0;
        ret[retCount++] = this->pt;
        if (left)
            retCount += left->getTree(ret+retCount);

        if (right)
            retCount += right->getTree(ret+retCount);

        return retCount;
    }

private:
    UpperBoundNode<T,S,D>*& testDirection(p_point new_pt) {
        bool leftMatch = false;
        if (left)
            leftMatch = left->isPointInRange(new_pt);

        bool rightMatch = false;
        if (right)
            rightMatch = right->isPointInRange(new_pt);

        if (leftMatch && !rightMatch)
            return left;

        if (rightMatch && !leftMatch)
            return right;

        return std::rand() % 2 ? left : right;
    }

private:
    bool isPointInRange(p_point new_pt) {
        return new_pt->vector.lessEq(upper) && new_pt->vector.moreEq(lower);
    }

    static void insert(UpperBoundNode<T,S,D>* curNode, p_point new_pt, UpperBoundNode<T,S,D>* newNode) {
        while (true) {
            curNode->updateUpperLower(new_pt);

            double new_ptToUpperDist = new_pt->vector.squareDist(curNode->upper);
            if (new_ptToUpperDist < curNode->ptToUpperDist) {
                curNode->ptToUpperDist = new_ptToUpperDist;
                std::swap(new_pt, curNode->pt);
            }

            auto& ptr = curNode->testDirection(new_pt);
            if (!ptr) {
                newNode->init(new_pt);
                ptr = newNode;
                break;
            } else {
                curNode = ptr;
            }
        }
    }
};


template<typename T, typename S, unsigned int D>
class UpperBoundRandTree : public BaseUpperBoundDataStruct<T,S,D> {
public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using p_point_arr = typename BaseUpperBoundDataStruct<T,S,D>::p_point_arr;

private:
    UpperBoundNode<T,S,D> root;
    std::unique_ptr<UpperBoundNode<T,S,D>[]> p_nodesPool;

private:
    void buildTree() {
        std::srand(std::time(0));
        auto pts = this->p_pts.get();
        root.init(pts);

        auto nodesPool = p_nodesPool.get();

        for (unsigned int i=1; i<this->size; i++)
            root.insert(pts + i, &nodesPool[i-1]);
    }

public:
	UpperBoundRandTree(p_point_arr pts, unsigned int size, unsigned int chunkSize) :
			BaseUpperBoundDataStruct<T, S, D>(pts, size, chunkSize) {
    	p_nodesPool.reset(new UpperBoundNode<T,S,D>[size-1]);
		buildTree();
    }

    unsigned int query(const point_vec& upper __attribute__((unused))) {
        return 0;
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        return root.fetchQuery(upper, ret);
    }
};


} // UpperBoundDS

#endif //UPPER_BOUND_RANDOM_TREE_HPP_
