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
using Reducers::custom;
using Reducers::sum;
using Reducers::prod;
using Reducers::double_mean;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::get;

int main()
{
	Vector<int> v({1,2,3,4,5,6,7,8,9,10});

	// basic reduction
	cout << "off-the-shelf stream methods ----------" << endl;
	cout << "sum = " << v.stream().sum() << endl;
	cout << "prod = " << v.stream().prod() << endl;
	cout << "mean = " << v.stream().double_mean() << endl;

	cout << "off-the-shelf Recuders ----------" << endl;
	cout << "sum = " << v.stream().reduce(sum<int>()) << endl;
	cout << "prod = " << v.stream().reduce(prod<int>()) << endl;
	cout << "mean = " << v.stream().reduce(double_mean<int>()).first << endl;

	cout << "off-the-shelf (multiple)----------" << endl;
	auto ots_m1 = v.stream().reduce(sum<int>(), prod<int>());
	auto ots_m2 = v.stream().reduce(sum<int>(), prod<int>(), double_mean<int>());
	cout << "sum,prod = " << get<0>(ots_m1) << ","	<< get<1>(ots_m1) << endl;
	cout << "sum,prod,mean = " << get<0>(ots_m2) << ","	<< get<1>(ots_m2) << "," << get<2>(ots_m2).first << endl;

	// custom and complex reductions
	auto reducer1 = custom(0, [](int r, int v) { return r+v; });
	auto reducer2 = custom(1, [](int r, int v) { return r*v; });
	auto reducer3 = custom(make_pair(0.0,1ul), [](pair<double,size_t> r, int v)
			{
				auto delta = v - r.first;
				r.first += delta/r.second++;
				return r;
			});

	auto result1 = v.stream().reduce(reducer1);
	auto result2 = v.stream().reduce(reducer1, reducer2);
	auto result3 = v.stream().reduce(reducer1, reducer2, reducer3);

	cout << "custom reducers --------" << endl;
	cout << "sum = " << get<0>(result1) << endl;
	cout << "sum,prd = " << get<0>(result2) << "," << get<1>(result2) << endl;
	cout << "sum,prod,mean = " << get<0>(result3) << "," << get<1>(result3)  << "," << get<2>(result3).first << endl;

	return 0;
}
