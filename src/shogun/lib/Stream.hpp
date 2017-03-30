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
#include <shogun/lib/Reducer.hpp>

namespace shogun
{

using Reducers::Reducer;
using Reducers::custom;
using Reducers::BindConfig;
using Reducers::bind_channel;

template <class C>
class Stream
{
public:
	typedef typename C::iterator_type::value_type T;
	Stream(C const * const _c) : c(_c), reducer() {}
	template <class RC>
	auto reduce(const RC& rc) -> typename RC::unitary_type
	{
		return reduce(c, bind_channel_if_not(rc));
	}
	template <class RC1, class RC2>
	auto reduce(const RC1& rc1, const RC2& rc2) -> std::pair<typename RC1::unitary_type,typename RC2::unitary_type>
	{
		return reduce(c, bind_channel_if_not(rc1), bind_channel_if_not(rc2));
	}
	template <class...RCs>
	auto reduce(const RCs&...rcs) -> std::tuple<typename RCs::unitary_type...>
	{
		return reduce(c, bind_channel_if_not(rcs)...);
	}
	auto sum() -> typename C::iterator_type::value_type
	{
		return reduce(c, bind_channel(Reducers::sum<T>(), std::identity<T>()));
	}
	auto prod() -> typename C::iterator_type::value_type
	{
		return reduce(c, bind_channel(Reducers::prod<T>(), std::identity<T>()));
	}
	double double_mean()
	{
		return reduce(c, bind_channel(Reducers::double_mean<T>(), std::identity<T>())).first;
	}
private:
	template <class RC>
	BindConfig<RC, std::identity<T>> bind_channel_if_not(const RC& rc) const
	{
		return bind_channel(rc, std::identity<T>());
	}
	template <class RC, class F>
	const BindConfig<RC, F>& bind_channel_if_not(const BindConfig<RC, F>& bc) const
	{
		return bc;
	}
	template <class BC>
	auto reduce(C const * const c, const BC& bc) -> typename BC::unitary_type
	{
		return reducer(c, bc);
	}
	template <class BC1, class BC2>
	auto reduce(C const * const c, const BC1& bc1, const BC2& bc2) -> std::pair<typename BC1::unitary_type,typename BC2::unitary_type>
	{
		return reducer(c, custom(std::make_pair(bc1.rc.init, bc2.rc.init),
					std::make_pair(bc1.rc.op, bc2.rc.op),
					std::make_pair(bc1.of, bc2.of)));
	}
	template <class... BCs>
	auto reduce(C const * const c, const BCs&...bcs) -> std::tuple<typename BCs::unitary_type...>
	{
		return reducer(c, custom(std::make_tuple(bcs.rc.init...),
					std::make_tuple(bcs.rc.op...), std::make_tuple(bcs.of...)));
	}
	C const * const c;
	const Reducer reducer;
};

}
#endif // STREAM_HPP__
