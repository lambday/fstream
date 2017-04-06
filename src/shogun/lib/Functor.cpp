#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <type_traits>
#include <cmath>

using std::declval;

template <template <class...> class Function>
struct Functor
{
		template <class T>
		using f_applied_type = decltype((declval<Function<T>>())(declval<T>()));

		// fmap :: Functor f => (a -> b) -> f a -> f b
		// takes a function that maps a -> b
		// takes a f(a) type instance
		// returns a f(b) type instance
		template <class S, class T>
		static f_applied_type<T> fmap(const std::function<T(S)>& map, f_applied_type<S>&& f_s)
		{
				return f_s.apply(map);
		}
};

template <class S>
struct EagerFunctor : Functor<EagerFunctor>
{
		EagerFunctor<S> operator()(S...) const;

		template <class T> using data_type = std::vector<T>;

		EagerFunctor() : data(data_type<S>()) {}
		EagerFunctor(const data_type<S>& _data) : data(_data)
		{
				std::cout << "constructed from vector" << std::endl;
		}

		template <class T>
		EagerFunctor<T> apply(const std::function<T(S)>& map) const
		{
				data_type<T> target(data.size());
				std::transform(data.begin(), data.end(), target.begin(), map);
				return EagerFunctor<T>(target);
		}

		const data_type<S>& get() const
		{
				return data;
		}

		const data_type<S>& data;
};

struct Vector
{
		template <class T> std::vector<T> operator()(T...) const;
		template <class T, class S>
		static std::vector<T> apply(const std::function<T(S)>& mapper, const std::vector<S>& source)
		{
				std::vector<T> target(source.size());
				std::transform(source.begin(), source.end(), target.begin(), mapper);
				return target;
		}
};

template <class Function, class S, class T>
struct LazyFunctor
{
		template <class A>
		using f_applied_type = decltype((declval<Function>())(declval<A>()));

		template <class Mapper>
		LazyFunctor(const Mapper& map, const f_applied_type<S>& _f_s)
		: mapper(map), f_s(_f_s)
		{
				std::cout << "constructed from vector" << std::endl;
		}

		template <class Mapper>
		auto fmap(const Mapper& map) const
		-> LazyFunctor<Function,S,decltype((declval<Mapper>())(declval<T>()))>
		{
				typedef decltype((declval<Mapper>())(declval<T>())) T_i;
				return LazyFunctor<Function,S,T_i>([this, &map](const S& s)
					{
						return map(mapper(s));
					}, f_s);
		}

		template <class T_i>
		LazyFunctor<Function,S,T_i> fmap(T_i(* const map)(const T&)) const
		{
				return LazyFunctor<Function,S,T_i>([this, &map](const S& s)
					{
						return map(mapper(s));
					}, f_s);
		}

		f_applied_type<T> get() const
		{
				return Function::apply(mapper, f_s);
		}

		const std::function<T(S)> mapper;
		const f_applied_type<S>& f_s;
};

// make it a friend
template <class S>
LazyFunctor<Vector,S,S> make_functor(const std::vector<S>& f_s)
{
		return LazyFunctor<Vector,S,S>([](const S& s) { return s; }, f_s);
}

double sqrt(const int& a)
{
		return std::sqrt(a);
}

void test()
{
		std::vector<int> l(10);
		std::iota(l.begin(), l.end(), 10);

		auto r = make_functor(l)
				.fmap(&sqrt)
				.fmap([](double x)
				{
					return std::to_string(x);
				})
				.get();

		std::for_each(r.begin(), r.end(), [](std::string s)
		{
			std::cout << s << std::endl;
		});
}

int main()
{
		test();
		return 0;
}
