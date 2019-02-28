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
#ifndef VCG_STATS_HPP_
#define VCG_STATS_HPP_

#include <cstring>

#include <debug.h>
#include <jointvecfunc.hpp>


class VCGStats {
public:
	const char* method;
	double totalRuntime = 0;
	double dsCreatePointsTime = 0;
	double dsBuildTime = 0;
	double dsQueryTime = 0;
	double dsQueryFetchTime = 0;

	double expectedComparedPoints = 0;
	double comparedPoints = 0;
	double comparedInBoundPoints = 0;
	double comparedEdgePoints = 0;
	double comparedBruteForce = 0;

	unsigned int dsPts = 0;
	unsigned int totalPts = 0;
	unsigned int totalQueries = 0;

	unsigned int joinedFuncCount = 0;
	unsigned int bruteForceCount = 0;

	VCGStats(const char* method="default") : method(method) {}

public:
	void print() {
		if (joinedFuncCount > 0) {
	        std::cout
	        << "====================================================================" << std::endl
	        << "Runs Statistics"                                                      << std::endl
	        << "====================================================================" << std::endl
			<< "Method:                           " << method                         << std::endl
			<< "Run count:                        " << joinedFuncCount                << std::endl
	        << "Average Compared Point:           "
	        << (comparedPoints / (double)joinedFuncCount)                             << std::endl
	        << "Average Expected Compare Point:   "
	        << (expectedComparedPoints / (double)joinedFuncCount)                     << std::endl
	        << "DS PTS count:                     " << dsPts                          << std::endl
	        << "Total PTS count:                  " << totalPts                       << std::endl
	        << "Total Queries:                    " << totalQueries                   << std::endl;
		}

        std::cout
        << "====================================================================" << std::endl
        << "Time Statistics"                                                      << std::endl
        << "====================================================================" << std::endl
        << "Total runtime (seconds):               " << totalRuntime              << std::endl
		<< "Total DS create points time (seconds): " << dsBuildTime               << std::endl
		<< "Total DS build time (seconds):         " << dsBuildTime               << std::endl
        << "Total query time (seconds):            " << dsQueryTime               << std::endl
        << "Total query fetch time (seconds):      " << dsQueryFetchTime          << std::endl;
	}
};


#endif /* VCG_STATS_HPP_ */
