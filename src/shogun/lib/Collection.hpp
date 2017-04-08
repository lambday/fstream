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

#ifndef COLLECTION_HPP__
#define COLLECTION_HPP__

#include <map>
#include <iostream>
#include <functional>
#include <tuple>
#include <memory>
#include <initializer_list>
#include <utility>
#include <shogun/lib/Monad.hpp>

namespace shogun
{

template <class T>
struct Collection : public Monad<T>
{
	virtual ~Collection() {}

	template <class V>
	struct iterator : std::iterator<std::bidirectional_iterator_tag,V>
	{
		using value_type = V;
		explicit iterator(T* _ptr) : ptr(_ptr) {}
		iterator& operator++() { ptr++; return *this; }
		iterator operator++(int) { auto ret = *this; ++(*this); return ret; }
		iterator& operator--() { ptr--; return *this; }
		iterator operator--(int) { auto ret = *this; --(*this); return ret; }
		friend bool operator==(iterator& first, iterator& second)
		{
			return first.ptr == second.ptr;
		}
		friend bool operator!=(iterator& first, iterator& second)
		{
			return !(first == second);
		}
		V& operator*() { return *ptr; }
		V* ptr;
	};

	using value_type = T;
	using iterator_type = iterator<T>;

	virtual iterator<T> begin() = 0;
	virtual iterator<T> end() = 0;
	virtual iterator<T> begin() const = 0;
	virtual iterator<T> end() const = 0;
};

}
#endif // COLLECTION_HPP__
