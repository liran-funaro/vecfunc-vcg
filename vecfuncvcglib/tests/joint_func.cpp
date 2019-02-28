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
#include <joinfunclib.hpp>
#include <jointvecfunc.hpp>

typedef VecFuncTest<VALUE,DIM> TDVecFuncTest;
typedef JointVecFuncTest<VALUE,DIM> TDJointVecFuncTest;

TDVecFuncTest::index readResSize(std::ifstream& infile) {
	TDVecFuncTest::index res_size;
	FOR_EACH_DIM_D(d, DIM)
		infile >> res_size[d];
	return res_size;
}


TDVecFuncTest readVal(std::ifstream& infile) {
	TDVecFuncTest::index sz;

	FOR_EACH_DIM_D(d, DIM)
		infile >> sz[d];

	TDVecFuncTest ret(sz);

	TDVecFuncTest::index ind;
	FOR_EACH_MAT_INDEX(ret, ind) {
		infile >> ret[ind];
	}

	return ret;
}


int main(int argc, char **argv) {
	if (argc < 2) {
		std::cout << "Required arguments: <input path> [<repeat>, <method>, <chunk size>]" << std::endl;
		return 1;
	}
    std::ifstream infile(argv[1]);
    unsigned int repeat = 1;
    unsigned int chunkSize = 512;
    unsigned int method = 0;

    if (argc > 2)
        repeat = (unsigned int)strtoul(argv[2], NULL, 10);
    if (argc > 3)
    	method = (unsigned int)strtoul(argv[3], NULL, 10);
    if (argc > 4)
    	chunkSize = (unsigned int)strtoul(argv[4], NULL, 10);

    unsigned int input_ndim;
	infile >> input_ndim;
	if (DIM != input_ndim) {
		std::cout << "Input file dim does not match: " << input_ndim << "!=" << DIM << std::endl;
		exit(1);
	}

    TDVecFuncTest::index res_size = readResSize(infile);
    TDJointVecFuncTest res(res_size);
    TDVecFuncTest a = readVal(infile);
    TDVecFuncTest b = readVal(infile);
    std::cout << "SIZES: " << res_size << " " << a.size << " " << b.size << std::endl;

    VCGStats stats("TEST");
    for (unsigned int i=0; i<repeat; i++)
    	join_vecfunc<VALUE, DIM, 1, true, true, false, true, true, true>(a, b, res,
    			(unsigned int)method, chunkSize, &stats);
    stats.print();

    double total_sum = res.sum<double>();
    TDJointVecFuncTest::index ind;
    FOR_EACH_MAT_INDEX(res, ind) {
    	total_sum += res.arg[res.get_index(ind)].L1Scalar();
    }
    std::cout << "A SUM: " << a.sum<double>() << " - B SUM: " << b.sum<double>() << " - RES SUM: " << total_sum << std::endl;

    double s = 0;
    FOR_EACH_MAT_INDEX(b, ind) {
    	FOR_EACH_DIM_D(d, DIM)
    		s += b[ind] * ind[d];
	}

    std::cout << "A IND SUM: " << s << std::endl;

    return 0;
}
