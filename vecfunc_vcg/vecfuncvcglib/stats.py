"""
Author: Liran Funaro <liran.funaro@gmail.com>

Copyright (C) 2006-2018 Liran Funaro

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""
import ctypes
import pprint
import numbers
from functools import reduce
from typing import List, Optional


class VCGStats(ctypes.Structure):
    _fields_ = [
        ("method", ctypes.c_char_p),
        ("totalRuntime", ctypes.c_double),
        ("dsCreatePointsTime", ctypes.c_double),
        ("dsBuildTime", ctypes.c_double),
        ("dsQueryTime", ctypes.c_double),
        ("dsQueryFetchTime", ctypes.c_double),

        ("expectedComparedPoints", ctypes.c_double),
        ("comparedPoints", ctypes.c_double),
        ("comparedInBoundPoints", ctypes.c_double),
        ("comparedEdgePoints", ctypes.c_double),
        ("comparedBruteForce", ctypes.c_double),

        ("dsPts", ctypes.c_uint),
        ("totalPts", ctypes.c_uint),
        ("totalQueries", ctypes.c_uint),

        ("joinedFuncCount", ctypes.c_uint),
        ("bruteForceCount", ctypes.c_uint),
    ]

    def as_dict(self):
        ret_dict = {}
        for f, t in self._fields_:
            if t == ctypes.c_char_p:
                ret_dict[f] = str(getattr(self, f), 'utf-8')
            else:
                ret_dict[f] = getattr(self, f)
        return ret_dict

    def __repr__(self):
        return pprint.pformat(self.as_dict())


def aggregate_stats(*stats: dict) -> dict:
    stats_count = len(stats)
    if stats_count < 1:
        return {}
    elif stats_count == 1:
        return stats[0]
    elif stats_count == 2:
        stats1 = stats[0]
        stats2 = stats[1]
        agg_stats = {}
        keys = set(stats1.keys())
        for k, v in stats1.items():
            if isinstance(v, numbers.Number):
                agg_stats[k] = [v]
            elif type(v) in (list, tuple):
                agg_stats[k] = list(v)
            else:
                keys.remove(k)
                agg_stats[k] = v
        for k in keys:
            v = stats2[k]
            if isinstance(v, numbers.Number):
                agg_stats[k].append(v)
            else:
                agg_stats[k].extend(v)
        return agg_stats
    else:
        return reduce(aggregate_stats, stats)
