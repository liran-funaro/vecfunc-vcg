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
import sys
import zipfile

import msgpack
import msgpack_numpy as m
m.patch()


def write_data(obj, filepath):
    with zipfile.ZipFile(filepath, "w", zipfile.ZIP_BZIP2) as zip_f:
        with zip_f.open('data.msgpack', 'w') as f:
            msgpack.pack(obj, f, use_bin_type=True)


def read_data(filepath):
    with zipfile.ZipFile(filepath, "r", zipfile.ZIP_BZIP2) as zip_f:
        with zip_f.open('data.msgpack', 'r') as f:
            return msgpack.unpack(f, encoding='utf-8')


def print_vals(data_filepath, v1_name, v2_name, res_size=None):
    """ Prints two valuation from a data file """
    d = read_data(data_filepath)
    v1 = d[v1_name]
    v2 = d[v2_name]
    if res_size is None:
        res_size = d['res-size']

    ndim = v1.ndim
    assert v2.ndim == ndim
    assert len(res_size) == ndim
    assert v1.dtype == v2.dtype

    print(ndim)

    for r in res_size:
        print(r)

    for v in (v1, v2):
        for s in v.shape:
            print(s)
        for vv in v.flatten(order='C'):
            print(vv)


if __name__ == '__main__':
    print_vals(*sys.argv[1:])
