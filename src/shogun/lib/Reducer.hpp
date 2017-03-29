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

#ifndef REDUCER_HPP__
#define REDUCER_HPP__

#include <map>
#include <iostream>
#include <functional>
#include <tuple>
#include <memory>
#include <initializer_list>
#include <utility>

namespace std
{

template<size_t I, class T>
T& get(T& t)
{
	return t;
}

}

namespace shogun
{

namespace Reducers
{

template <class T>
struct num_elements
{
	enum { value = 1 };
};

template <class A, class B>
struct num_elements<std::pair<A,B>>
{
	enum { value = 2 };
};

template <class... A>
struct num_elements<std::tuple<A...>>
{
	enum { value = std::tuple_size<std::tuple<A...>>::value };
};

template <class R, class V, size_t TotalNumReducers, size_t CurrentNumReducer>
struct accumulate_all : accumulate_all<R,V,TotalNumReducers,CurrentNumReducer-1>
{
	typedef typename R::unitary_type U;
	explicit accumulate_all(R& r, V& value, U& accumulated)
	: accumulate_all<R,V,TotalNumReducers,CurrentNumReducer-1>(r, value, accumulated)
	{
		std::get<CurrentNumReducer-1>(accumulated) = std::get<CurrentNumReducer-1>(r.accumulate)(std::get<CurrentNumReducer-1>(accumulated), value);
	}
};

template <class R, class V, size_t TotalNumReducers>
struct accumulate_all<R,V,TotalNumReducers,1>
{
	typedef typename R::unitary_type U;
	explicit accumulate_all(R& r, V& value, U& accumulated)
	{
		std::get<0>(accumulated) = std::get<0>(r.accumulate)(std::get<0>(accumulated), value);
	}
};

template <class R, class V>
struct accumulate_all<R,V,1,1>
{
	typedef typename R::unitary_type U;
	explicit accumulate_all(R& r, V& value, U& accumulated)
	{
		accumulated = r.accumulate(accumulated, value);
	}
};

template <class R, class V>
struct accumulator : accumulate_all<R,V,R::num_reducers,R::num_reducers>
{
	typedef typename R::unitary_type U;
	explicit accumulator(R& r, V& value, U& accumulated)
	: accumulate_all<R,V,R::num_reducers,R::num_reducers>(r, value, accumulated)
	{
	}
};

template <class U, class A, size_t N>
struct Reducer
{
	using unitary_type = U;
	using accumulate_type = A;
	static constexpr size_t num_reducers = N;
	using type = Reducer<U,A,N>;
	Reducer(const U& _unitary, A&& _accumulate)
		: unitary(_unitary), accumulate(std::forward<A>(_accumulate))
	{
	}
	template <class It>
	U reduce(It current, It end, U accumulated)
	{
		typedef typename It::value_type V;
		if (current == end)
			return accumulated;
		accumulator<type,V>(*this, *current, accumulated);
		return reduce(++current, end, accumulated);
	}
	template <class C>
	U reduce(C const * const c)
	{
		return reduce(c->begin(), c->end(), unitary);
	}
	U unitary;
	A accumulate;
};

template <class U, class A>
auto custom(U&& unitary, A&& accumulate) -> Reducer<U,A,num_elements<A>::value>
{
	return Reducer<U,A,num_elements<A>::value>(unitary, std::forward<A>(accumulate));
}

}

}
#endif // REDUCER_HPP__
