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

#include <map>
#include <iostream>
#include <functional>
#include <tuple>
#include <memory>
#include <initializer_list>
#include <utility>
#include <shogun/lib/Vector.hpp>

using namespace shogun;

int main()
{
	Vector<int> v({1,2,3,4,5,6,7,8,9,10});

	// basic reduction
	std::cout << "off-the-shelf ----------" << std::endl;
	std::cout << "sum = " << v.stream().sum() << std::endl;
	std::cout << "prod = " << v.stream().prod() << std::endl;
	std::cout << "mean = " << v.stream().double_mean() << std::endl;

	// custom and complex reductions
	auto r1 = custom(0, [](int r, int v) { return r+v; });
	auto r2 = custom(1, [](int r, int v) { return r*v; });
	auto r3 = custom(std::make_pair(0.0,1ul), [](std::pair<double,size_t> r, int v)
			{
				auto delta = v - r.first;
				r.first += delta/r.second++;
				return r;
			});

	auto result = v.stream().reduce(r1);
	auto result2 = v.stream().reduce(r1, r2);
	auto result3 = v.stream().reduce(r1, r2, r3);

	std::cout << "custom reducers --------" << std::endl;
	std::cout << "sum = " << std::get<0>(result) << std::endl;
	std::cout << "sum, prd = " << std::get<0>(result2) << ", " << std::get<1>(result2) << std::endl;
	std::cout << "sum, prod, mean = " << std::get<0>(result3) << ", " << std::get<1>(result3)  << ", " << std::get<2>(result3).first << std::endl;

	return 0;
}