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
#include <string>
#include <cstdint>

#include <cmath>
#include <memory>
#include <algorithm>

#include <stats.h>
#include <vecfunc_types.hpp>
#include <vcg_stats.hpp>
#include <vecfunc.hpp>
#include <maille_tuffin.hpp>


template<bool BUILD_TIMING>
VCGStats template_vcg_maille_tuffin(VALUE* concat_bids, uint32* bid_sizes, uint32 player_count,
		uint32 total, uint32* arg_res) {
	// Assume arg_res[:] == 0

	VCGStats stats;
	stats.method = "Maille Tuffin";
	if (DIM != 1)
		return stats;

	std::unique_ptr<TDVecFunc[]> funcs(new TDVecFunc[player_count]);

	for (unsigned int i=0; i<player_count; i++) {
		unsigned int val_sz = bid_sizes[i];
		funcs[i].reset(concat_bids, val_sz);
		concat_bids += val_sz;
	}

	MailleTuffin<VALUE, DIM>::template maille_tuffin<BUILD_TIMING>(funcs.get(), player_count, total,
			arg_res, &stats);

	return stats;
}

#define DEF_VCG_MAILLE_TUFFIN(N, BUILD_TIMING) \
	VCGStats vcg_maille_tuffin_##N(VALUE* concat_bids, uint32* bid_sizes, uint32 player_count, \
			uint32 total, uint32* arg_res) { \
		return template_vcg_maille_tuffin<BUILD_TIMING>(concat_bids, bid_sizes, player_count, \
				total, arg_res); \
	}


// Shared library interface
extern "C" {

DEF_VCG_MAILLE_TUFFIN(buildtime, true)
DEF_VCG_MAILLE_TUFFIN(main, false)

} // extern "C"
