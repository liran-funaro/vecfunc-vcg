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
#ifndef JOINFUNC_JOIN_VECFUNC_HPP_
#define JOINFUNC_JOIN_VECFUNC_HPP_

#include <vecfunc.hpp>

template<typename T, unsigned int D>
class JointVecFunc : public VecFunc<T, D> {
public:
    typedef typename VecFunc<T,D>::index index;

public:
    index* arg;

public:
    template<typename SIZE_TYPE>
    JointVecFunc(T* val, index* arg, const SIZE_TYPE& size) :
            VecFunc<T,D>(val, size), arg(arg) {}
};


template<typename T, unsigned int D>
class JointVecFuncTest : public JointVecFunc<T, D> {
public:
    using index = typename JointVecFunc<T, D>::index;

public:
    template<typename SIZE_TYPE>
    JointVecFuncTest(const SIZE_TYPE& size) : JointVecFunc<T,D>(NULL, NULL, size) {
    	auto sz = this->total_size();
		this->m = new T[sz];
		this->arg = new index[sz];
    }

    ~JointVecFuncTest() {
    	delete[] this->m;
    	delete[] this->arg;
    }
};

#endif /* JOINFUNC_JOIN_VECFUNC_HPP_ */
