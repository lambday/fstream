#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>

using std::declval;

struct dummy
{
		template <class T> std::vector<T> operator()(T...) const;
};

template <class T, class... F>
struct lazy_operator
{
		template <class U> lazy_operator<U, F...> operator()(U...) const;
		lazy_operator() {}
		lazy_operator(std::tuple<F...> _opstack) : opstack(_opstack) {}
		std::tuple<F...> opstack;
		// auto get(operand) // TODO
};

template <class F>
struct Functor
{
		// fmap :: Functor f => (a -> b) -> f a -> f b
		// takes a function that maps a -> b
		// takes a f(a) type instance
		// returns a f(b) type instance
		template <class S, class T>
		decltype(f(declval<T>())) fmap(std::function<T(S)>, decltype(f(declval<S>())));
		F f;
};

struct EagerFunctor : Functor<dummy>
{
		using F = dummy;
		template <class S, class T>
		decltype(f(declval<T>())) fmap(std::function<T(S)> fn, decltype(f(declval<S>())) f_s)
		{
				decltype(f(declval<T>())) _new(f_s.size());
				std::transform(f_s.begin(), f_s.end(), _new.begin(), fn);
				return _new;
		}
};

template <class L>
struct LazyFunctor : Functor<lazy_operator<L>>
{
		using F = lazy_operator<L>;
		template <class S, class T>
		decltype(f(declval<T>())) fmap(std::function<T(S)> fn, decltype(f(declval<S>())) f_s)
		{
				decltype(f(declval<T>())) _new(std::tuple_cat(f_s.opstack, std::make_tuple(fn)));
				return _new;
		}
};

void test()
{
		std::vector<int> l(10);
		std::iota(l.begin(), l.end(), 1);
		std::function<std::string(int)> fn = [](int x) { return std::to_string(x); };
		auto r = EagerFunctor().fmap(fn, l);
		std::for_each(r.begin(), r.end(), [](const std::string& s)
				{
					std::cout << s << std::endl;
				});
		lazy_operator<int> lazy;
		auto r2 = LazyFunctor<int>().fmap(fn, lazy);
}

int main()
{
		test();
		return 0;
}
