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
#ifndef MAILLE_TUFFIN_HPP_
#define MAILLE_TUFFIN_HPP_

#include <string>
#include <cstdint>

#include <cmath>
#include <memory>
#include <algorithm>

#include <stats.h>
#include <vcg_stats.hpp>
#include <vecfunc.hpp>


template<typename T, unsigned int D>
class MailleTuffin {
public:
	template<bool BUILD_TIMING>
	static void maille_tuffin(VecFunc<T,D>* vals __attribute__((unused)),
			unsigned int player_count __attribute__((unused)),
			unsigned int total __attribute__((unused)),
			uint32_t* arg_res __attribute__((unused)),
			VCGStats* stats __attribute__((unused))) {
		stats->totalRuntime = nan("");
	}
};


template<typename T>
class MailleTuffin<T, 1> {
	class ValPoint {
	public:
		T v;
		unsigned int p;
	};

private:
	static unsigned int merge_bid_point(ValPoint* prevRes, const unsigned int prevResSz,
			VecFunc<T,1>& val, const unsigned int valPlayer,
			ValPoint* res, const unsigned int resSz) {
		unsigned int res_i=0, prev_i=0, val_i=0;
		unsigned int valSz = val.size[0];

		while(res_i<resSz && prev_i<prevResSz && val_i<valSz) {
			if (prevRes[prev_i].v > val[val_i])
				res[res_i++] = prevRes[prev_i++];
			else {
				res[res_i].v = val[val_i++];
				res[res_i++].p = valPlayer;
			}
		}

		while(res_i<resSz && prev_i<prevResSz) {
			res[res_i++] = prevRes[prev_i++];
		}

		while(res_i<resSz && val_i<valSz) {
			res[res_i].v = val[val_i++];
			res[res_i++].p = valPlayer;
		}

		return res_i;
	}

public:
	template<bool BUILD_TIMING>
	static void maille_tuffin(VecFunc<T,1>* bids, unsigned int player_count,
			unsigned int total, uint32_t* arg_res, VCGStats* stats __attribute__((unused))) {
		// Assume arg_res[:] == 0

		STATS_INIT(start_time);
		STATS_START(start_time);

		std::unique_ptr<ValPoint[]> merged_val_set[2];
		merged_val_set[0].reset(new ValPoint[total]);
		merged_val_set[1].reset(new ValPoint[total]);
		ValPoint* merged_val = merged_val_set[0].get();
		ValPoint* res_val = merged_val_set[1].get();
		unsigned int merged_val_size = 0;

		for (unsigned int i=0; i<player_count; i++) {
			merged_val_size = merge_bid_point(merged_val, merged_val_size,
					bids[i], i, res_val, total);
			std::swap(merged_val, res_val);
		}

		if (BUILD_TIMING)
			STATS_ADD_TIME(start_time, stats->dsBuildTime);

		for(unsigned int i=0; i<merged_val_size; i++) {
			arg_res[merged_val[i].p]++;
		}

		STATS_ADD_TIME(start_time, stats->totalRuntime);
	}
};


#endif /* MAILLE_TUFFIN_HPP_ */
