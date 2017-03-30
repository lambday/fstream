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

template <typename T>
struct identity
{
	T&& operator()(T&& value) const noexcept
	{
		return std::forward<T>(value);
	}
	const T& operator()(const T& value) const noexcept
	{
		return value;
	}
};

template <class T>
struct is_pair
{
	static constexpr bool value = false;
};

template <class A, class B>
struct is_pair<std::pair<A, B>>
{
	static constexpr bool value = true;
};

// TODO try removing it
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

template <class R, class V, class F, size_t N, size_t I>
struct accumulate_all : accumulate_all<R,V,F,N,I-1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& result, const A& op, const F& of, const V& value)
	: accumulate_all<R,V,F,N,I-1>(result, op, of, value)
	{
		std::get<I-1>(result) = std::get<I-1>(op)(std::get<I-1>(result), std::get<I-1>(of)(value));
	}
};

template <class R, class V, class F, size_t N>
struct accumulate_all<R,V,F,N,1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& result, const A& op, const F& of, const V& value)
	{
		std::get<0>(result) = std::get<0>(op)(std::get<0>(result), std::get<0>(of)(value));
	}
};

template <class R, class V, class F>
struct accumulate_all<R,V,F,1,1>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate_all(U& result, const A& op, const F& of, const V& value)
	{
		result = op(result, of(value));
	}
};

template <class R, class V, class F>
struct accumulate : accumulate_all<R,V,F,R::num_tasks,R::num_tasks>
{
	typedef typename R::unitary_type U;
	typedef typename R::accumulator_type A;
	explicit accumulate(U& result, const A& op, const F& of, const V& value)
	: accumulate_all<R,V,F,R::num_tasks,R::num_tasks>(result, op, of, value)
	{
	}
};

template <class RC, class F>
struct BindConfig
{
	using unitary_type = typename RC::unitary_type;
	BindConfig(const RC& _rc, const F& _of) : rc(_rc), of(_of) {}
	const RC rc;
	const F of;
};

class Reducer
{
public:
	template <class C, class RC, class F>
	auto operator()(C const * const c, BindConfig<RC,F> rc_of) const -> typename RC::unitary_type
	{
		return reduce(c->begin(), c->end(), rc_of.rc, rc_of.of);
	}
private:
	template <class It, class RC, class F>
	auto reduce(It first, It last, const RC& rc, const F& of) const -> typename RC::unitary_type
	{
		typedef typename It::value_type V;
		auto result = rc.init;
		for (auto it = first; it != last; ++it)
		{
			accumulate<RC,V,decltype(of)>(result, rc.op, of, *it);
		}
		return result;
	}
};

template <class U, class A, size_t N>
struct Config
{
	using unitary_type = U;
	using accumulator_type = A;
	static constexpr size_t num_tasks = N;
	using type = Config<U,A,N>;

	Config(const U& _init, const A& _op) : init(_init), op(_op)
	{
	}
	Config(const Config& other) = default;
	Config& operator=(const Config& other) = default;
	~Config() = default;

	const U init;
	const A op;
};

template <class U, class A>
auto custom(const U& init, const A& op) -> Config<U,A,num_elements<A>::value>
{
	return Config<U,A,num_elements<A>::value>(init, op);
}

template <class U, class A, class F>
auto custom(const U& init, const A& op, const F& of)
	-> BindConfig<Config<U,A,num_elements<A>::value>,F>
{
	return bind_channel(Config<U,A,num_elements<A>::value>(init, op), of);
}

template <class RC, class F>
BindConfig<RC,F> bind_channel(const RC& rc, const F& of)
{
	return BindConfig<RC,F>(rc, of);
}

template <class RC, class R, class... Args>
BindConfig<RC,std::function<R(Args...)>> bind_channel(const RC& rc, R(*of)(Args...))
{
	using F = std::function<R(Args...)>;
	return BindConfig<RC,F>(rc, F(of));
}

template <typename T>
struct Accumulators
{
	// TODO move inside the Config subclasses
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
struct sum : Config<U,typename Accumulators<U>::sum,1>
{
	using A = typename Accumulators<U>::sum;
	sum() : Config<U,A,1>(0, A()) {}
};

template <typename U>
struct prod : Config<U,typename Accumulators<U>::prod,1>
{
	using A = typename Accumulators<U>::prod;
	prod() : Config<U,A,1>(1, A()) {}
};

template <typename U>
struct double_mean : Config<std::pair<double,size_t>,typename Accumulators<U>::double_mean,1>
{
	using A = typename Accumulators<U>::double_mean;
	double_mean() : Config<std::pair<double,size_t>,A,1>(std::make_pair(0.0,1ul), A()) {}
};

}

}
#endif // REDUCER_HPP__
