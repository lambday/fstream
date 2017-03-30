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

template <class C>
void test_off_the_shelf_stream_methods(const C& c)
{
	cout << "off-the-shelf stream methods ----------" << endl;
	cout << "sum = " << c.stream().sum() << endl;
	cout << "prod = " << c.stream().prod() << endl;
	cout << "mean = " << c.stream().double_mean() << endl;
}

template <class C>
void test_off_the_shelf_reducers(const C& c)
{
	cout << "off-the-shelf Recuders ----------" << endl;
	cout << "sum = " << c.stream().reduce(sum<int>()) << endl;
	cout << "prod = " << c.stream().reduce(prod<int>()) << endl;
	cout << "mean = " << c.stream().reduce(double_mean<int>()).first << endl;
}

template <class C>
void test_off_the_shelf_reducers_multiple(const C& c)
{
	cout << "off-the-shelf (multiple)----------" << endl;
	auto ots_m1 = c.stream().reduce(sum<int>(), prod<int>());
	auto ots_m2 = c.stream().reduce(sum<int>(), prod<int>(), double_mean<int>());
	cout << "sum,prod = " << get<0>(ots_m1) << ","	<< get<1>(ots_m1) << endl;
	cout << "sum,prod,mean = " << get<0>(ots_m2) << ","	<< get<1>(ots_m2) << "," << get<2>(ots_m2).first << endl;
}

template <class C>
void test_custom_reducers(const C& c)
{
	auto reducer1 = custom(0, [](int r, const int& v) { return r+v; });
	auto reducer2 = custom(1, [](int r, const int& v) { return r*v; });
	auto reducer3 = custom(make_pair(0.0,1ul), [](pair<double,size_t> r, const int& v)
			{
				auto delta = v - r.first;
				r.first += delta/r.second++;
				return r;
			});

	cout << "cutom reducers ----------" << endl;
	cout << "sum = " << c.stream().reduce(reducer1) << endl;
	cout << "prod = " << c.stream().reduce(reducer2) << endl;
	cout << "mean = " << c.stream().reduce(reducer3).first << endl;
}

template <class C>
void test_custom_reducers_multiple(const C& c)
{
	auto reducer1 = custom(0, [](int r, const int& v) { return r+v; });
	auto reducer2 = custom(1, [](int r, const int& v) { return r*v; });
	auto reducer3 = custom(make_pair(0.0,1ul), [](pair<double,size_t> r, const int& v)
			{
				auto delta = v - r.first;
				r.first += delta/r.second++;
				return r;
			});

	auto result2 = c.stream().reduce(reducer1, reducer2);
	auto result3 = c.stream().reduce(reducer1, reducer2, reducer3);

	cout << "custom reducers (multiple)--------" << endl;
	cout << "sum,prd = " << get<0>(result2) << "," << get<1>(result2) << endl;
	cout << "sum,prod,mean = " << get<0>(result3) << "," << get<1>(result3)  << "," << get<2>(result3).first << endl;
}

template <class C>
void test_off_the_shelf_reducers_bind_channel(const C& c)
{
	const int&(*get_0)(const std::pair<int,int>&) = std::get<0,int,int>;
	const int&(*get_1)(const std::pair<int,int>&) = std::get<1,int,int>;

	cout << "off-the-shelf Recuders bind channel ----------" << endl;
	cout << "sum = " << c.stream().reduce(bind_channel(sum<int>(), get_0)) << endl;
	cout << "sum = " << c.stream().reduce(bind_channel(sum<int>(), get_1)) << endl;
	cout << "prod = " << c.stream().reduce(bind_channel(prod<int>(), get_0)) << endl;
	cout << "prod = " << c.stream().reduce(bind_channel(prod<int>(), get_1)) << endl;
	cout << "mean = " << c.stream().reduce(bind_channel(double_mean<int>(), get_0)).first << endl;
	cout << "mean = " << c.stream().reduce(bind_channel(double_mean<int>(), get_1)).first << endl;
}

int main()
{
	Vector<int> v({1,2,3,4,5,6,7,8,9,10});
	test_off_the_shelf_stream_methods(v);
	test_off_the_shelf_reducers(v);
	test_off_the_shelf_reducers_multiple(v);
	test_custom_reducers(v);
	test_custom_reducers_multiple(v);

	Vector<std::pair<int,int>> vp({std::make_pair(1,2),std::make_pair(3,4),std::make_pair(5,6),std::make_pair(7,8)});
	test_off_the_shelf_reducers_bind_channel(vp);
	return 0;
}
