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

#include <iostream>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <shogun/lib/Vector.hpp>
#include <benchmark/benchmark.h>

using namespace shogun;

double sqrt(const int& a)
{
	return std::sqrt(a);
}

Vector<double> test1(const Vector<int>& l)
{
	return Functional::evaluate(l)
//		.map([](int x)
//		{
//			std::vector<int> v(x);
//			std::iota(v.begin(), v.end(), 1);
//			return Functional::as_functor(v);
//		})
//		.mjoin()
		.map(&sqrt)
		.map([](double x)
		{
			return std::log(x);
		})
		.map([](double x)
		{
			return std::sin(x/2);
		})
		.yield();

//		std::for_each(r.begin(), r.end(), [](float s)
//		{
//			std::cout << "ahoy, " << s << std::endl;
//		});

//		std::for_each(r.begin(), r.end(), [](std::vector<int>& v)
//		{
//			std::for_each(v.begin(), v.end(), [](int s)
//			{
//				std::cout << s << " ";
//			});
//			std::cout << std::endl;
//		});
}

Vector<double> __attribute__ ((noinline)) test2(const Vector<int>& l)
{
	Vector<double> r(l.vlen);
	std::transform(l.begin(), l.end(), r.begin(), [](int x)
	{
		return std::sin(std::log(sqrt(x))/2);
	});
	return r;
}

size_t size = 1000;

static void functional(benchmark::State& state)
{
	Vector<int> l(size);
	std::iota(l.begin(), l.end(), 1);
	double c1 = 0;
	while (state.KeepRunning())
	{
		auto r = test1(l);
//		std::cout << r << std::endl;
		c1 = sqrt(std::accumulate(r.begin(), r.end(), 0.0));
//		break;
	}
//	std::cout << c1 << std::endl;
}

BENCHMARK(functional);

static void normal(benchmark::State& state)
{
	Vector<int> l(size);
	std::iota(l.begin(), l.end(), 1);
	double c2 = 0;
	while (state.KeepRunning())
	{
		auto r = test2(l);
//		std::cout << r << std::endl;
		c2 = sqrt(std::accumulate(r.begin(), r.end(), 0.0));
//		break;
	}
//	std::cout << c2 << std::endl;
}

BENCHMARK(normal);

BENCHMARK_MAIN();
