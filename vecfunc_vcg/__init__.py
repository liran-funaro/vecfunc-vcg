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
import time
import numpy as np
from vecfunc_vcg.vecfuncvcglib import join_all, vcg_maille_tuffin_multi_resource, aggregate_stats


def validate_payments(payments, private_values):
    n = len(payments)
    eps = np.finfo(np.float32).eps
    for i in range(n):
        assert payments[i] > -eps,\
            "Bad payment value for player %s: %f < 0" % (i, payments[i])
        assert payments[i] < private_values[i] + eps,\
            "Bad payment value for player %s: payment (%f) > value (%f)" % (i, payments[i], private_values[i])


###############################################################################
# Optimization
###############################################################################


def joint_func(val_funcs, max_alloc, calc_payments=True,
               join_method=None, join_chunk_size=None, join_flags=None, change_join_order=True):
    """
    Find the optimal social welfare given a list of vectorized valuations.

    Args:
        val_funcs (vecfunc/numpy-array): Vectorized valuation functions.
        max_alloc (int tuple): The maximal allocation.
        calc_payments (bool, optional): If True, will also return the
            corresponding player's payments for the optimal allocation.
            Defaults to True.
        join_method (int, optional): The joint valuation method.
        join_chunk_size (int, optional): The joint valuation chunk size.
        join_flags (str tuple, optional): One of the following flags:
            'filter': Filter compared points before.
            'count': Count compare points.
            'buildtime': Collects data structure build time statistics.
            'querytime': Collects data structure query time statistics.
        change_join_order (boo, optional): Change the join order to improve performance.

    Returns: {
        'sw': The optimal social-welfare.
        'used-resources': The sum of resource allocated.
        'allocations': The player's allocation  (if calc_allocs).
        'private-values': The player's private values (if calc_private_values).
        'payments': The player's payments (if calc_payments).
        'stats': Statistics (runtime and algorithm specific information).
    }

    Raises:
        ValueError: If given one or no valuations as input.
        ValueError: If the valuations are not of the same type and dimensions.
        AssertionError: If the optimization yields non consistent results.
        RuntimeError: If no module was compiled for the given valuations type.
    """
    start_time = time.time()
    n = len(val_funcs)
    if n < 2:
        raise ValueError("Need at least two functions")

    if change_join_order:
        s = np.argsort([np.max(v) for v in val_funcs])
        order = [s[-1], *s[:-2:2], *s[1:-2:2][::-1], s[-2]]
        orig_order = np.argsort(order)
    else:
        order = orig_order = np.arange(len(val_funcs))

    val_funcs = [val_funcs[i] for i in order]

    joined_func_lst = join_all(val_funcs, max_alloc, method=join_method, chunk_size=join_chunk_size,
                               flags=join_flags)
    joined_func = joined_func_lst[-1]
    sw_argmax = joined_func.argmax()
    sw_max = joined_func[sw_argmax]
    vcg_stats = joined_func.aggregated_stats()

    ret = {
        'sw': sw_max,
        'used-resources': sw_argmax,
        'stats': vcg_stats,
        'joined-func': joined_func.arr,
    }

    # Calculating the allocations
    allocs = list(joined_func.get_args(sw_argmax))
    ret['allocations'] = [allocs[i] for i in orig_order]

    # Validate allocation sum match total allocation
    total_alloc = np.sum(allocs, axis=0)
    assert (total_alloc == sw_argmax).all(), "Non allocated: %s" % (sw_argmax - total_alloc)

    # Calculating the private values
    private_values = [v[tuple(a)] for v, a in zip(val_funcs, allocs)]
    ret['private-values'] = [private_values[i] for i in orig_order]

    # Validating private values match social-welfare
    values_sum = np.sum(private_values)
    assert np.isclose(values_sum, sw_max), f"SW ({sw_max}) - private_values ({values_sum}) = {sw_max - values_sum}"

    # Calculating the payments
    payments = []
    if calc_payments:
        joined_func_rev_lst = join_all(val_funcs[::-1], max_alloc, method=join_method, chunk_size=join_chunk_size,
                                       flags=join_flags)
        ret['stats'] = aggregate_stats(ret['stats'], joined_func_rev_lst[-1].aggregated_stats())

        joined_func_rev = joined_func_rev_lst[-1]
        joined_func_rev_lst = joined_func_rev_lst[::-1]

        ret['joined-func-rev'] = joined_func_rev.arr
        rev_sw_max = joined_func_rev.arr.max()
        assert np.isclose(sw_max, rev_sw_max), "SW (%s) != SW-reverse (%s)" % (sw_max, rev_sw_max)

        ret['is-order-indifferent'] = np.allclose(joined_func.arr, joined_func_rev.arr)

        for i in range(n):
            if all(a == 0 for a in allocs[i]):
                payments.append(0)
                continue

            if i == 0:
                jv = joined_func_rev_lst[1]
            elif i == n-1:
                jv = joined_func_lst[-2]
            else:
                jv = join_all([joined_func_lst[i - 1], joined_func_rev_lst[i + 1]], max_alloc, method=join_method,
                              chunk_size=join_chunk_size, flags=join_flags)[-1]
                ret['stats'] = aggregate_stats(ret['stats'], jv.stats)

            payments.append(jv.max() - (sw_max - private_values[i]))
        ret['payments'] = [payments[i] for i in orig_order]

        # Validation
        validate_payments(payments, private_values)

    end_time = time.time()
    ret['stats']['optimizationRunTime'] = end_time - start_time
    return ret


def maille_tuffin(val_funcs, val_funcs_1d, max_alloc, calc_payments=True):
    """
    Find the optimal social welfare given a list of vectorized valuations.

    Args:
        val_funcs (vecfunc/numpy-array): Vectorized valuation functions.
        val_funcs_1d (vecfunc/numpy-array): Vectorized valuation functions in each dimention.
        max_alloc (int tuple): The maximal allocation.
        calc_payments (bool, optional): If True, will also return the
            corresponding player's payments for the optimal allocation.
            Defaults to True.

    Returns: {
        'sw': The optimal social-welfare.
        'used-resources': The sum of resource allocated.
        'allocations': The player's allocation.
        'private-values': The player's private values.
        'payments': The player's payments (if calc_payments).
        'stats': Statistics (runtime and algorithm specific information).
    }

    Raises:
        ValueError: If given one or no valuations as input.
        ValueError: If the valuations are not of the same type and dimensions.
        AssertionError: If the optimization yields non consistent results.
        RuntimeError: If no module was compiled for the given valuations type.
    """
    start_time = time.time()
    n = len(val_funcs)
    if n < 2:
        raise ValueError("Need at least two functions")

    vcg_ret = vcg_maille_tuffin_multi_resource(val_funcs, val_funcs_1d, max_alloc)
    allocs, vcg_stats, private_values, sw, used_resources = vcg_ret

    ret = {
        'sw': sw,
        'used-resources': used_resources,
        'allocations': allocs,
        'private-values': private_values,
        'stats': vcg_stats
    }

    # Calculating the payments
    payments = []
    if calc_payments:
        for i in range(n):
            if all(a == 0 for a in allocs[i]):
                payments.append(0)
                continue

            sub_vals = [v for val_ind, v in enumerate(val_funcs) if val_ind != i]
            sub_vals_1d = [v for val_ind, v in enumerate(val_funcs_1d) if val_ind != i]
            sub_vcg_ret = vcg_maille_tuffin_multi_resource(sub_vals, sub_vals_1d, max_alloc)
            sub_allocs, sub_vcg_stats, sub_private_values, sub_sw, sub_used_resources = sub_vcg_ret

            ret['stats'] = aggregate_stats(ret['stats'], sub_vcg_stats)
            cur_pay = sub_sw - (sw - private_values[i])
            cur_pay = min(cur_pay, private_values[i])
            payments.append(cur_pay)

        ret['payments'] = payments

        # Validation
        validate_payments(payments, private_values)

    end_time = time.time()
    ret['stats']['optimizationRunTime'] = end_time - start_time
    return ret
