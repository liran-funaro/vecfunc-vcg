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
#ifndef FRACTIONAL_CASCADING_HPP_
#define FRACTIONAL_CASCADING_HPP_

#include <cmath>
#include <memory>
#include <algorithm>
#include <utility>
#include <queue>
#include <type_traits>

#include "upper_bound_ds.hpp"


namespace UpperBoundDS {


template<typename T, typename S, unsigned int D>
class UpperBoundRangeTree2DFC : public BaseUpperBoundRangeDS<T,S,D> {
	static_assert(D >= 2, "Dim must be at least 2.");

public:
    using point = typename BaseUpperBoundDataStruct<T,S,D>::point;
    using point_vec = typename BaseUpperBoundDataStruct<T,S,D>::point_vec;
    using p_point = typename BaseUpperBoundDataStruct<T,S,D>::p_point;
    using shared_points = typename BaseUpperBoundDataStruct<T,S,D>::shared_points;

private:
    unsigned int groupsSize=0;
    unsigned int groupsCount=0;
    unsigned int d1=0, d2=0;

    std::unique_ptr<T[]> sortedD1;
    std::unique_ptr<T[]> sortedD2;
    std::unique_ptr<unsigned int[]> fractional;
    std::unique_ptr<unsigned int[]> g_ind;
    std::unique_ptr<unsigned int[]> g_end;
    unsigned int fractionalCount=0;


    unsigned int res_group=0;
    unsigned int res_ind=0;

public:
    UpperBoundRangeTree2DFC(const shared_points& pts, unsigned int chunkSize,
    		unsigned int d1=0, unsigned int d2=0) :
			BaseUpperBoundRangeDS<T, S, D>(pts, chunkSize), d1(d1), d2(d2) {
			this->init();
		}

    UpperBoundRangeTree2DFC() : BaseUpperBoundRangeDS<T, S, D>() {}

    void init(const shared_points& pts, unsigned int chunkSize,
    		unsigned int d1, unsigned int d2) {
    	this->baseInit(pts, chunkSize);
        this->d1 = d1;
        this->d2 = d2;
        this->init();
    }

private:
	void init() {
		this->allocHelperArrays(1);

		groupsCount = (this->size + (this->chunkSize - 1)) / this->chunkSize;
		groupsSize = (this->size + (groupsCount - 1)) / groupsCount;

		sortedD1.reset(new T[groupsCount]);
		sortedD2.reset(new T[this->size + 1]);
		fractional.reset(new unsigned int[(this->size + 1) * groupsCount]);
		g_ind.reset(new unsigned int[groupsCount]);
		g_end.reset(new unsigned int[groupsCount]);
		buildTree();
	}

    void buildTree() {
    	this->fillHelperArray(0);
        p_point* depth_arr = this->helperArray(0);

        for(unsigned int g=0; g<groupsCount; g++) {
            g_ind[g] = g * groupsSize;
            g_end[g] = (g == groupsCount-1) ? this->size : (g+1)*groupsSize;
        }

        this->sortHelperByDim(0, d1, 0, this->size);

        for(unsigned int g=0; g<groupsCount; g++) {
            sortedD1[g] = depth_arr[g_ind[g]]->vector[d1];
            this->sortHelperByDim(0, d2, g_ind[g], g_end[g]);
        }

        fractionalCascading();
    }

    void shortFractionalCascading() {
        p_point* depth_arr = this->helperArray(0);

        unsigned int gc = groupsCount;
        auto in_g = std::unique_ptr<unsigned int []>(new unsigned int[groupsCount]);
        for(unsigned int g=0; g<groupsCount; g++)
            in_g[g] = g;

        T min_v = std::numeric_limits<T>::max();
        unsigned int min_g = groupsCount;

        for(unsigned int g=0; g<groupsCount; g++) {
            T cur_v = depth_arr[g_ind[g]]->vector[d2];
            if (cur_v < min_v) {
                min_v = cur_v;
                min_g = g;
            }
        }

        std::swap(in_g[min_g], in_g[gc-1]);

        unsigned int i=0;
        while(gc > 0) {
            auto g1 = in_g[0];

            if (gc == 1)
                min_v = depth_arr[g_ind[g1]]->vector[d2];
            else {
                auto g2 = in_g[1 + std::rand() % (gc-1)];
                T v1 = depth_arr[g_ind[g1]]->vector[d2];
                T v2 = depth_arr[g_ind[g2]]->vector[d2];
                min_v = v1 < v2 ? v1 : v2;
                min_g = v1 < v2 ? g1 : g2;
                std::swap(in_g[min_g], in_g[0]);
            }

            sortedD2[i] = min_v;
            for(unsigned int ig=0; ig<gc; ) {
                auto g = in_g[ig];
                while(g_ind[g] < g_end[g] && depth_arr[g_ind[g]]->vector[d2] <= min_v)
                    g_ind[g]++;

                if (g_ind[g] < g_end[g])
                    ig++;
                else
                    in_g[ig] = in_g[--gc];
            }

            for(unsigned int g=0; g<groupsCount; g++)
                fractional[i*groupsCount + g] = g_ind[g];
            i++;
        }

        fractionalCount = i;

        for(unsigned int g=0; g<groupsCount; g++) {
            g_ind[g] = g * groupsSize;
            fractional[i*groupsCount + g] = g_end[g];
        }
    }

    void fractionalCascading() {
        p_point* depth_arr = this->helperArray(0);

        T min_v = std::numeric_limits<T>::max();
        for(unsigned int g=0; g<groupsCount; g++) {
            T cur_v = depth_arr[g_ind[g]]->vector[d2];
            if (cur_v < min_v)
                min_v = cur_v;
        }

        unsigned int i=0;
        while (min_v != std::numeric_limits<T>::max()) {
            T new_min_v = std::numeric_limits<T>::max();

            sortedD2[i] = min_v;
            for(unsigned int g=0; g<groupsCount; g++) {
                T cur_v = std::numeric_limits<T>::max();
                while(g_ind[g] < g_end[g]) {
                    cur_v = depth_arr[g_ind[g]]->vector[d2];
                    if (cur_v > min_v)
                        break;
                    else
                        g_ind[g]++;
                }
                if (cur_v < new_min_v)
                    new_min_v = cur_v;
            }

            for(unsigned int g=0; g<groupsCount; g++)
                fractional[i*groupsCount + g] = g_ind[g];
            i++;
            min_v = new_min_v;
        }

        for(unsigned int g=0; g<groupsCount; g++) {
            g_ind[g] = g * groupsSize;
            fractional[i*groupsCount + g] = g_end[g];
        }

        fractionalCount = i;
    }

public:
    unsigned int query(const point_vec& upper) {
        auto sortedD1_raw = sortedD1.get();
        res_group = std::upper_bound(sortedD1_raw, sortedD1_raw+groupsCount,
                                 upper[d1]) - sortedD1_raw;

        auto sortedD2_raw = sortedD2.get();
        res_ind = std::upper_bound(sortedD2_raw, sortedD2_raw+fractionalCount,
                                 upper[d2]) - sortedD2_raw;

        unsigned int retCount = 0;
        for(unsigned int g=0; g<res_group; g++)
            retCount += fractional[res_ind*groupsCount + g] - g_ind[g];

        return retCount;
    }

    template <bool FILTER>
    unsigned int fetchQuery(const point_vec& upper, p_point* ret) {
        p_point* depth_arr = this->helperArray(0);
        unsigned int retCount = 0;

        for(unsigned int g=0; g<res_group; g++) {
            auto l = g_ind[g];
            auto h = fractional[res_ind*groupsCount + g];
            for (auto i=l; i<h; i++)
            	retCount = this->template appendResultPoint<FILTER>(ret, retCount, depth_arr[i], upper);
        }

        return retCount;
    }
};

} // UpperBoundDS

#endif //FRACTIONAL_CASCADING_HPP_
