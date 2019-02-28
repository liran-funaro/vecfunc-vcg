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
from vecfunc import loader
from vecfunc_vcg.vecfuncvcglib.stats import aggregate_stats


def vcg_maille_tuffin(funcs, size_limit, flags=()):
    n = len(funcs)
    ndim = 1
    dtype = funcs[0].dtype
    eps = np.finfo(np.float32).eps

    player_count = len(funcs)
    for f in funcs:
        assert f.ndim == ndim
        assert f.dtype == dtype

    bids = [np.diff(f) for f in funcs]
    for i, b in enumerate(bids):
        assert np.all(b[1:] < b[:-1]+eps), "Valuation is not concave for player %s" % i
    concat_bids = np.require(np.concatenate(bids, axis=0), dtype=dtype, requirements=loader.read_req)

    bid_sizes = np.array([len(b) for b in bids], dtype=np.uint32, order='C')
    bid_sizes = np.require(bid_sizes, dtype=np.uint32, requirements=loader.read_req)

    ret_alloc = np.zeros(n, dtype='uint32', order='C')
    ret_alloc = np.require(ret_alloc, dtype='uint32', requirements=loader.write_req)

    if flags is None:
        flags = ()
    if isinstance(flags, str):
        flags = (flags,)

    flags_bool = tuple([k in flags for k in ('buildtime',)])
    _, data = loader.load_lib(ndim, dtype)
    vcg_maille_tuffin_func = data['vcg_maille_tuffin_func'][flags_bool]

    stats = vcg_maille_tuffin_func(concat_bids, bid_sizes, player_count, size_limit, ret_alloc)
    return ret_alloc, stats.as_dict()


def vcg_maille_tuffin_multi_resource(val_funcs, val_funcs_1d, max_alloc):
    ret = [vcg_maille_tuffin(fs, ma) for ma, *fs in zip(max_alloc, *val_funcs_1d)]
    allocs, stats = zip(*ret)
    allocs = list(zip(*allocs))
    stats = aggregate_stats(*stats)
    private_values = [v[tuple(a)] for v, a in zip(val_funcs, allocs)]
    sw = np.sum(private_values)
    used_resources = np.sum(allocs, axis=0)

    assert (used_resources <= max_alloc).all(), \
        f"Allocated more than possible {tuple(max_alloc)}: {tuple(used_resources)}"

    return allocs, stats, private_values, sw, used_resources
