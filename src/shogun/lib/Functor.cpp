#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <type_traits>
#include <cmath>

using std::declval;

template <class Function, class S, class T>
struct Functor
{
		template <class A>
		using f_applied_type = decltype((declval<Function>())(declval<A>()));

		template <class Mapper>
		using T_i = decltype((declval<Mapper>())(declval<T>()));

		template <class Mapper>
		Functor(const Mapper& map, const f_applied_type<S>& _f_s) : mapper(map), f_s(_f_s) {}

		// fmap :: Functor f => (a -> b) -> f a -> f b
		// takes a function that maps a -> b
		// takes a f(a) type instance
		// returns a f(b) type instance
		template <class Mapper>
		Functor<Function,S,T_i<Mapper>> fmap(const Mapper& map) const
		{
				return Functor<Function,S,T_i<Mapper>>([this, &map](const S& s)
					{
						return std::forward<T_i<Mapper>>(map(std::forward<T>(mapper(s))));
					}, f_s);
		}

		// fmap :: Functor f => (a -> b) -> f a -> f b
		// takes a function that maps a -> b
		// takes a f(a) type instance
		// returns a f(b) type instance
		template <class T_i>
		Functor<Function,S,T_i> fmap(T_i(* const map)(const T&)) const
		{
				return Functor<Function,S,T_i>([this, &map](const S& s)
					{
						return std::forward<T_i>(map(std::forward<T>(mapper(s))));
					}, f_s);
		}

		f_applied_type<T> get() const
		{
				return Function::apply(mapper, f_s);
		}

		const std::function<T(S)> mapper;
		const f_applied_type<S>& f_s;
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

template <class S>
Functor<Vector,S,S> make_functor(const std::vector<S>& f_s)
{
		return Functor<Vector,S,S>([](S&& s) { return std::forward<S>(s); }, f_s);
}

double sqrt(const int& a)
{
		return std::sqrt(a);
}

void test(std::vector<int>& l)
{
		auto r = make_functor(l)
			.fmap(&sqrt)
			.fmap([](const double& x)
			{
				return std::forward<std::string>(std::to_string(x));
			})
			.fmap([](const std::string& x)
			{
				return std::forward<float>(std::stof(x)/2);
			})
			.get();

		std::for_each(r.begin(), r.end(), [](float s)
		{
			std::cout << s << std::endl;
		});
}

int main()
{
		std::vector<int> l(10);
		std::iota(l.begin(), l.end(), 10);
		test(l);
		return 0;
}
