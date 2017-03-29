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
#include <typeinfo>

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

template <class R, class V, size_t N, size_t I>
struct accumulate_all : accumulate_all<R,V,N,I-1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& accumulated, const A& accumulator, const V& value)
	: accumulate_all<R,V,N,I-1>(accumulated, accumulator, value)
	{
		std::get<I-1>(accumulated) = std::get<I-1>(accumulator)(std::get<I-1>(accumulated), value);
	}
};

template <class R, class V, size_t N>
struct accumulate_all<R,V,N,1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& accumulated, const A& accumulator, const V& value)
	{
		std::get<0>(accumulated) = std::get<0>(accumulator)(std::get<0>(accumulated), value);
	}
};

template <class R, class V>
struct accumulate_all<R,V,1,1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& accumulated, const A& accumulator, const V& value)
	{
		accumulated = accumulator(accumulated, value);
	}
};

template <class R, class V>
struct accumulate : accumulate_all<R,V,R::num_tasks,R::num_tasks>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate(U& accumulated, const A& accumulator, const V& value)
	: accumulate_all<R,V,R::num_tasks,R::num_tasks>(accumulated, accumulator, value)
	{
	}
};

template <class U, class A, size_t N>
struct Reducer
{
	using unitary_type = U;
	using accumulator_type = A;
	using type = Reducer<U,A,N>;
	static constexpr size_t num_tasks = N;

	Reducer(const U& _unitary, const A& _accumulator) : unitary(_unitary), accumulator(_accumulator)
	{
	}

	Reducer(const Reducer& other) = default;
	Reducer& operator=(const Reducer& other) = default;
	~Reducer() = default;

	template <class C>
	U reduce(C const * const c) const
	{
		return reduce(c->begin(), c->end(), unitary);
	}

	template <class It>
	U reduce(It begin, It end, U accumulated) const
	{
		typedef typename It::value_type V;
		for (auto it = begin; it != end; ++it)
		{
			accumulate<type,V>(accumulated, accumulator, *it);
		}
		return accumulated;
	}

	const U unitary;
	const A accumulator;
};

template <class U, class A>
auto custom(const U& unitary, const A& accumulator) -> Reducer<U,A,num_elements<A>::value>
{
	return Reducer<U,A,num_elements<A>::value>(unitary, accumulator);
}

template <typename T>
struct Accumulators
{
	struct sum { T& operator()(T& r, const T& v) const { r+=v; return r; }};
	struct prod { T& operator()(T& r, const T& v) const { r*=v; return r; }};
	struct double_mean
	{
		std::pair<double,size_t>& operator()(std::pair<double,size_t>& r, const T& v) const
		{
			auto delta = v - r.first;
			r.first += delta/r.second++;
			return r;
		};
	};
};

template <typename U>
struct sum : Reducer<U,typename Accumulators<U>::sum,1>
{
	using A = typename Accumulators<U>::sum;
	sum() : Reducer<U,A,1>(0, A()) {}
};

template <typename U>
struct prod : Reducer<U,typename Accumulators<U>::prod,1>
{
	using A = typename Accumulators<U>::prod;
	prod() : Reducer<U,A,1>(1, A()) {}
};

template <typename U>
struct double_mean : Reducer<std::pair<double,size_t>,typename Accumulators<U>::double_mean,1>
{
	using A = typename Accumulators<U>::double_mean;
	double_mean() : Reducer<std::pair<double,size_t>,A,1>(std::make_pair(0.0,1), A()) {}
};

}

}
#endif // REDUCER_HPP__
