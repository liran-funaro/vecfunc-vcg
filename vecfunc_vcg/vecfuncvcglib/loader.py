"""
Loads the vecfunc module (C++).
Compiles it if necessary.

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
import subprocess
import numpy as np
import os
import sys
import ctypes
import inspect
from vecfunc_vcg.vecfuncvcglib.stats import VCGStats
from vecfunc.vecfunclib.loader import get_types as vecfunc_get_types

"""
Read/write requirements from the numpy array.
 - ‘C_CONTIGUOUS’ (‘C’) - ensure a C-contiguous array
 - ‘ALIGNED’ (‘A’)      - ensure a data-type aligned array
 - ‘OWNDATA’ (‘O’)      - ensure an array that owns its own data
 - ‘WRITEABLE’ (‘W’)    - ensure a writable array

See numpy.require documentation for more information.
"""
read_req  = ('C', 'A', 'O')
write_req = (*read_req, 'W')


module_template = "vecfuncvcg_%sd_%s.so"
subpath_options = "vecfuncvcglib/bin", "vecfuncvcglib", "bin", "."


__lib__ = {}
__force_compile__ = False


def force_compile(force=True):
    global __force_compile__
    __force_compile__ = force


def locate_lib_path(fname):
    """ Locate a file in the optional sub-folders"""
    curpath = os.path.dirname(os.path.abspath(__file__))

    while curpath != '/':
        for subpath in subpath_options:
            file_path = os.path.join(curpath, subpath, fname)
            if os.path.isfile(file_path):
                return file_path
        curpath = os.path.normpath(os.path.join(curpath, '..'))

    return None


def normalize_parameters(ndim, dtype):
    """ Return the parameters in a normalized form"""
    if dtype == float:
        dtype = 'float64'
    elif dtype == int:
        dtype = 'int64'

    dtype = dtype.lower().strip()

    return int(ndim), dtype


def locate_dll(ndim, dtype):
    """ Locate the module's DLL file """
    return locate_lib_path(module_template % (ndim, dtype))


def make_lib(ndim, dtype):
    """ Compile the module to specific parameters """
    ndim, dtype = normalize_parameters(ndim, dtype)

    make_file_path = locate_lib_path("makefile")
    print(make_file_path)
    cwd = os.path.dirname(make_file_path)

    params_str = "(dim=%s, value=%s)" % (ndim, dtype)
    cmd = "make dim=%s value=%s" % (ndim, dtype)
    caller = inspect.stack()[2]
    print(f"Building module for: {params_str}. "
          f"CMD: {cmd}. Called from: {caller[1]} ({caller[2]}). "
          f"In folder: {cwd}.", file=sys.stderr)
    ret = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ret.stderr:
        print(str(ret.stderr, 'utf-8'), file=sys.stderr)
        raise RuntimeError("Could not compile module for the parameters: %s." % params_str)


def get_types(ndim, dtype):
    return dict(
        vecfunc_get_types(ndim, dtype),
        vcg_concat_vals_type=np.ctypeslib.ndpointer(dtype=dtype, ndim=1, flags=read_req),
        vcg_val_sizes_type=np.ctypeslib.ndpointer(dtype='uint32', ndim=1, flags=read_req),
        vcg_ret_alloc_type=np.ctypeslib.ndpointer(dtype='uint32', ndim=1, flags=write_req),
        joined_vecfunc_type=np.ctypeslib.ndpointer(dtype=dtype, ndim=ndim, flags=write_req),
        joined_vecfunc_arg_type=np.ctypeslib.ndpointer(dtype='uint32', ndim=ndim + 1, flags=write_req),
    )


def load_lib(ndim, dtype):
    """ Loads and initialize a module library, or use already loaded module """
    ndim, dtype = normalize_parameters(ndim, dtype)
    key = ndim, dtype
    if key in __lib__:
        return __lib__[key]

    if __force_compile__:
        make_lib(ndim, dtype)

    dll_path = locate_dll(ndim, dtype)
    if dll_path is None:
        make_lib(ndim, dtype)
        dll_path = locate_dll(ndim, dtype)
        if dll_path is None:
            params_str = "(dim=%s, value=%s)" % (ndim, dtype)
            raise RuntimeError("No module was compiled for the parameters: %s." % params_str)
    lib = ctypes.cdll.LoadLibrary(dll_path)

    t = get_types(ndim, dtype)

    vcg_join_func = {
        (False, False, False, False, False, False): 'nofilter',
        (False, True,  False, False, False, False): 'filter',
        (False, True,  True,  False, False, False): 'brute_opt',
        (False, True,  False, True,  False, False): 'count',
        (False, True,  False, True,  True,  False): 'buildtime',
        (False, True,  False, True,  True,  True):  'querytime',

        (True, False, False, False, False, False): 'fg_nofilter',
        (True, True,  False, False, False, False): 'fg_filter',
        (True, True,  True,  False, False, False): 'fg_brute_opt',
        (True, True,  False, True,  False, False): 'fg_count',
        (True, True,  False, True,  True,  False): 'fg_buildtime',
        (True, True,  False, True,  True,  True):  'fg_querytime',

    }
    vcg_join_func = {k: getattr(lib, 'vcg_join_%s' % v) for k, v in vcg_join_func.items()}
    t['vcg_join_func'] = vcg_join_func

    vcg_maille_tuffin_func = {(True,): 'buildtime', (False,): 'main'}
    vcg_maille_tuffin_func = {k: getattr(lib, 'vcg_maille_tuffin_%s' % v) for k, v in vcg_maille_tuffin_func.items()}
    t['vcg_maille_tuffin_func'] = vcg_maille_tuffin_func

    for vcg_join in vcg_join_func.values():
        vcg_join.argtypes = (
            t['vecfunc_type'], t['vec_size_t'],
            t['vecfunc_type'], t['vec_size_t'],
            t['joined_vecfunc_type'], t['joined_vecfunc_arg_type'],
            t['vec_size_t'], ctypes.c_uint32, ctypes.c_uint32
        )
        vcg_join.restype = VCGStats

    for vcg_maille_tuffin in vcg_maille_tuffin_func.values():
        vcg_maille_tuffin.argtypes = (
            t['vcg_concat_vals_type'], t['vcg_val_sizes_type'],
            ctypes.c_uint32, ctypes.c_uint32,
            t['vcg_ret_alloc_type']
        )
        vcg_maille_tuffin.restype = VCGStats

    lib.vcg_test_ds_build_time.argtypes = (t['vecfunc_type'], t['vec_size_t'], ctypes.c_uint32, ctypes.c_uint32)
    lib.vcg_test_ds_build_time.restype = VCGStats

    ret = lib, t
    __lib__[key] = ret
    return ret
