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
#ifndef DEBUG_H_
#define DEBUG_H_

#include <chrono>
#include <iostream>
#include <iomanip>

#include <stats.h>


#ifdef DEBUG
static auto debug_prev = std::chrono::high_resolution_clock::now();

#   define DEBUG_OUTPUT(__output__) \
    do{ \
        auto debug_now = stats_time(); \
        auto elapsed = stats_elapsed(debug_prev, debug_now); \
        std::cout << "#" \
            << "[+" << std::left << std::setw (15) << elapsed << " s] " \
            << std::setw (20) << __FILE__ << " : "    \
            << std::setw (20) << __func__ << " : "    \
            << std::setw (4)  << __LINE__ << " : "    \
            << __output__ << std::endl; \
		debug_prev = debug_now; \
    } while(0)
#   define DEBUG_STMT(__op__) __op__
#else
#   define DEBUG_OUTPUT(__output__) do {} while(0)
#   define DEBUG_STMT(__op__) do {} while(0)
#endif


#endif //DEBUG_H_
