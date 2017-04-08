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

using namespace shogun;

double sqrt(const int& a)
{
	return std::sqrt(a);
}

void test(const Vector<int>& l)
{
	auto r = Functional::evaluate(l)
//		.composite([](int x)
//		{
//			std::vector<int> v(x);
//			std::iota(v.begin(), v.end(), 1);
//			return Functional::as_functor(v);
//		})
//		.mjoin()
		.composite(&sqrt)
		.composite([](double x)
		{
			return std::to_string(x);
		})
		.composite([](const std::string& x)
		{
			return std::stof(x)/2;
		})
		.yield();

		std::for_each(r.begin(), r.end(), [](float s)
		{
			std::cout << "ahoy, " << s << std::endl;
		});

//		std::for_each(r.begin(), r.end(), [](std::vector<int>& v)
//		{
//			std::for_each(v.begin(), v.end(), [](int s)
//			{
//				std::cout << s << " ";
//			});
//			std::cout << std::endl;
//		});
}

int main()
{
	Vector<int> l(10);
	std::iota(l.begin(), l.end(), 1);
	test(l);
	return 0;
}
