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
#include <iostream>
#include <cstdint>
#include <numeric>
#include <chrono>
#include <cstdlib>
#include <fstream>

#include <vecfunc_types.hpp>
#include <vcg_stats.hpp>
#include <vecfunc.hpp>
#include <maille_tuffin.hpp>


TDVecFuncTest::index readResSize(std::ifstream& infile) {
	TDVecFuncTest::index res_size;
	FOR_EACH_DIM_D(d, DIM)
		infile >> res_size[d];
	return res_size;
}


void readVal(TDVecFuncTest& f, std::ifstream& infile) {
	TDVecFuncTest::index sz;

	FOR_EACH_DIM_D(d, DIM)
		infile >> sz[d];

	f.reset(sz);

	TDVecFuncTest::index ind;
	FOR_EACH_MAT_INDEX(f, ind) {
		infile >> f[ind];
	}
}


int main(int argc, char **argv) {
	if (DIM != 1)
		return 1;

	if (argc < 2) {
		std::cout << "Required arguments: <input path> [<repeat>]" << std::endl;
		return 1;
	}
    std::ifstream infile(argv[1]);
    unsigned int repeat = 1;
    const unsigned int player_count = 2;

    if (argc > 4)
    	repeat = (unsigned int)strtoul(argv[2], NULL, 10);

    unsigned int input_ndim;
	infile >> input_ndim;
	if (DIM != input_ndim) {
		std::cout << "Input file dim does not match: " << input_ndim << "!=" << DIM << std::endl;
		exit(1);
	}

    TDVecFuncTest::index res_size = readResSize(infile);
    TDVecFuncTest funcs[player_count];
    readVal(funcs[0], infile);
    readVal(funcs[1], infile);
    std::cout << "SIZES: " << res_size << " " << funcs[0].size << " " << funcs[1].size << std::endl;

    std::unique_ptr<uint32_t[]> arg_res(new uint32_t[player_count]);

    VCGStats stats("TEST Maille and Tuffin");
    for (unsigned int i=0; i<repeat; i++)
    	MailleTuffin<VALUE, DIM>::maille_tuffin(funcs, player_count, res_size[0], arg_res.get(),
    			&stats);
    stats.print();

    return 0;
}
