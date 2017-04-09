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

#ifndef EVAL_HPP__
#define EVAL_HPP__

#include <functional>

using std::declval;

namespace shogun
{

template <template <class> class Functor, class A, class B>
struct Eval
{
	Eval(std::function<B(A)>&& map, const Functor<A>& _f_a) : mapper(map), f_a(_f_a)
	{
	}

	template <class Mapper>
	Eval<Functor,A,decltype((declval<Mapper>())(declval<B>()))> map(const Mapper& _mapper) const
	{
		using C = decltype((declval<Mapper>())(declval<B>()));
		auto composite_mapper = [this, &_mapper](const A& a)
		{
			return std::forward<C>(_mapper(std::forward<B>(mapper(a))));
		};
		return Eval<Functor,A,C>(composite_mapper, f_a);
	}

//	template <class Mapper>
//	auto bind(const Mapper& mapper) const
//	{
//		using NewFunctor = decltype((declval<Mapper>())(declval<B>()));
//		//
//	}

	template <class C>
	Eval<Functor,A,C> map(C(* const _mapper)(const B&)) const
	{
		auto composite_mapper = [this, &_mapper](const A& a)
		{
			return std::forward<C>(_mapper(std::forward<B>(mapper(a))));
		};
		return Eval<Functor,A,C>(composite_mapper, f_a);
	}

	Functor<B> yield() const
	{
		return f_a.fmap(mapper);
	}

	const std::function<B(A)> mapper;
	const Functor<A>& f_a;
};

}

#endif // EVAL_HPP__
