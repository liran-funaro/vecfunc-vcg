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
#ifndef STATS_H_
#define STATS_H_

#include <chrono>
#include <iostream>
#include <iomanip>


static inline std::chrono::high_resolution_clock::time_point stats_time() {
	return std::chrono::high_resolution_clock::now();
}

static inline double stats_elapsed(std::chrono::high_resolution_clock::time_point& start_time) {
	std::chrono::duration<double> elapsed = stats_time() - start_time;
	return elapsed.count();
}

static inline double stats_elapsed(std::chrono::high_resolution_clock::time_point& start_time,
		std::chrono::high_resolution_clock::time_point& end_time) {
	std::chrono::duration<double> elapsed = end_time - start_time;
	return elapsed.count();
}


#define STATS_INIT(__start_time_var__) std::chrono::high_resolution_clock::time_point __start_time_var__
#define STATS_START(__start_time_var__) __start_time_var__ = stats_time()
#define STATS_ADD_TIME(__start_time_var__, __var__) __var__ += stats_elapsed(__start_time_var__)


#endif //STATS_H_
