/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2017, Soumyajit De
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STREAM_HPP__
#define STREAM_HPP__

#include <map>
#include <iostream>
#include <functional>
#include <tuple>
#include <memory>
#include <initializer_list>
#include <utility>
#include <fstream/lib/Reducer.hpp>

namespace shogun
{

using Reducers::Reducer;
using Reducers::custom;

template <class C>
struct Stream
{
	Stream(C const * const _c) : c(_c) {}
	template <class R>
	auto reduce(R& r) -> typename R::unitary_type
	{
		return r.reduce(c);
	}
	template <class R1, class R2>
	auto reduce(R1& r1, R2& r2) -> std::pair<typename R1::unitary_type,typename R2::unitary_type>
	{
		return custom(std::make_pair(r1.unitary, r2.unitary), std::make_pair(r1.accumulate, r2.accumulate)).reduce(c);
	}
	template <class...Rs>
	auto reduce(Rs...rs) -> std::tuple<typename Rs::unitary_type...>
	{
		return custom(std::make_tuple(rs.unitary...), std::make_tuple(rs.accumulate...)).reduce(c);
	}
	auto sum() -> typename C::value_type
	{
		typedef typename C::value_type T;
		return custom(0, [](T& r, T& v) { return r+v; }).reduce(c);
	}
	auto prod() -> typename C::value_type
	{
		typedef typename C::value_type T;
		return custom(1, [](T& r, T& v) { return r*v; }).reduce(c);
	}
	double double_mean()
	{
		typedef typename C::value_type T;
		auto result = custom(std::make_pair(0.0,1ul), [](std::pair<double,size_t> r, T& v)
			{
				auto delta = v - r.first;
				r.first += delta/r.second++;
				return r;
			}).reduce(c);
		return result.first;
	}
	C const * const c;
};

}
#endif // STREAM_HPP__
