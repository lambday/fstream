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

#ifndef VECTOR_HPP__
#define VECTOR_HPP__

#include <iostream>
#include <functional>
#include <memory>
#include <initializer_list>
#include <shogun/lib/Collection.hpp>
#include <shogun/lib/Eval.hpp>

namespace shogun
{

template <class T>
struct Vector : public Collection<T>
{
	using iterator_type = typename Collection<T>::iterator_type;
	template <class A> Vector<A> operator()(A...) const;

	virtual ~Vector() {}

	Vector(std::initializer_list<T> list)
	: vec(std::make_unique<T[]>(list.size())), vlen(list.size())
	{
		std::copy(list.begin(), list.end(), vec.get());
	}

	Vector(size_t size)
	: vec(std::make_unique<T[]>(size)), vlen(size)
	{
		std::fill(begin(), end(), static_cast<T>(0));
	}

	virtual iterator_type begin() override
	{
		return iterator_type(vec.get());
	}

	virtual iterator_type end() override
	{
		return iterator_type(vec.get() + vlen);
	}

	virtual iterator_type begin() const override
	{
		return iterator_type(vec.get());
	}

	virtual iterator_type end() const override
	{
		return iterator_type(vec.get() + vlen);
	}

	// fmap :: Functor f => (a -> b) -> f a -> f b
	template <class B>
	Vector<B>&& fmap(const std::function<B(T)>& mapper) const
	{
		Vector<B> target(vlen);
		std::transform(begin(), end(), target.begin(), mapper);
		return std::forward<Vector<B>>(target);
	}

	std::unique_ptr<T[]> vec;
	size_t vlen;
};

namespace Functional
{

template <class T>
Eval<Vector<T>,T,T> evaluate(const Vector<T>& f_a)
{
	auto identity_map = [](T&& a) { return std::forward<T>(a); };
	return Eval<Vector<T>,T,T>(identity_map, f_a);
}

}

}

#endif // VECTOR_HPP__
