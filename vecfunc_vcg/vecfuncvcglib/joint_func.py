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
import numpy as np
from vecfunc.vecfunclib import VecFunc, as_vecfunc
from vecfunc_vcg.vecfuncvcglib import loader
from vecfunc_vcg.vecfuncvcglib.stats import aggregate_stats
import numbers


class JoinedVecFunc(VecFunc):
    def __init__(self, f1, f2, size_limit, method=None, chunk_size=None, flags=None):
        self.method = 0 if method is None else method
        self.chunk_size = 64 if chunk_size is None else chunk_size
        self.f1 = as_vecfunc(f1)
        self.f2 = as_vecfunc(f2)

        if self.f1.dtype != self.f2.dtype:
            raise ValueError("Functions must have the same data type,"
                             "but %s != %s." % (self.f1.dtype, self.f2.dtype))
        if self.f1.ndim != self.f2.ndim:
            raise ValueError("Functions must have the same number of dimensions,"
                             "but %s != %s." % (self.f1.ndim, self.f2.ndim))
        dtype = f1.dtype
        ndim = f1.ndim
        shape = self.get_maximal_joined_func_size(self.f1, self.f2, np.add(size_limit, 1))

        VecFunc.__init__(self, np.empty(shape, dtype=dtype, order='C'), require_write=True)

        if flags is None:
            flags = ()
        if isinstance(flags, str):
            flags = (flags,)

        flags_bool = tuple(
            [k in flags for k in ('filter_grad', 'filter', 'brute_opt', 'count', 'buildtime', 'querytime')])
        _, data = loader.load_lib(self.ndim, self.dtype)
        vcg_join_func = data['vcg_join_func'][flags_bool]

        self.arg_shape = shape + (ndim,)
        arg_arr = np.empty(self.arg_shape, dtype='uint32', order='C')
        self.arg_arr = np.require(arg_arr, dtype='uint32', requirements=loader.write_req)

        self.stats = vcg_join_func(self.f1.arr, self.f1.ctype_arr_size, self.f2.arr, self.f2.ctype_arr_size,
                                   self.arr, self.arg_arr, self.ctype_arr_size, self.method, self.chunk_size)
        self.stats = self.stats.as_dict()

    @staticmethod
    def get_maximal_joined_func_size(f1, f2, size_limit):
        """ Returns the maximal size of the joined function """
        max_size = np.add(f1.shape, f2.shape) - 1
        max_size = np.minimum(max_size, size_limit)
        return tuple(np.maximum(max_size, 0).astype(np.uint32))

    def get_args(self, ind):
        f1_arg = tuple(self.arg_arr[ind])
        f2_arg = tuple(np.subtract(ind, f1_arg))
        res = []
        for f, a in [(self.f1, f1_arg), (self.f2, f2_arg)]:
            if hasattr(f, "get_args"):
                res.extend(f.get_args(a))
            else:
                res.append(a)

        # Validation
        total_args = np.sum(res, axis=0)
        assert (total_args == ind).all(), "Non raising functions - unused: %s" % np.subtract(ind, total_args)
        return res

    def get_funcs(self):
        res = []
        for f in [self.f1, self.f2]:
            if hasattr(f, "get_funcs"):
                res.extend(f.get_funcs())
            else:
                res.append(f)
        return res

    def aggregated_stats(self) -> dict:
        agg_stats = self.stats
        for f in [self.f1, self.f2]:
            if not hasattr(f, "aggregated_stats"):
                continue
            agg_stats = aggregate_stats(agg_stats, f.aggregated_stats())
        return agg_stats


def test_ds_build_time(v, method, chunk_size):
    v = as_vecfunc(v)
    lib = v.get_lib()
    stats = lib.vcg_test_ds_build_time(v.arr, v.ctype_arr_size, method, chunk_size)
    return stats.as_dict()


def sum_test_ds_build_time(funcs, method, chunk_size):
    ret = None
    for i in range(len(funcs)):
        stats = test_ds_build_time(funcs[i], method=method, chunk_size=chunk_size)
        if ret is None:
            ret = stats
        else:
            ret.update({k: v + ret[k] for k, v in stats.items() if isinstance(v, numbers.Number)})
    return ret


def join_all(funcs, joined_func_size_limit, method=None, chunk_size=None, flags=None):
    joined_funcs = [funcs[0]]
    for f in funcs[1:]:
        joined_funcs.append(JoinedVecFunc(joined_funcs[-1], f, joined_func_size_limit, method=method,
                                          chunk_size=chunk_size, flags=flags))
    return joined_funcs
