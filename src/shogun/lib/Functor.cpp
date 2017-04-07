#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <type_traits>
#include <cmath>

using std::declval;

// fmap :: Functor f => (a -> b) -> f a -> f b
// takes a function that maps a -> b
// takes a f(a) type instance
// returns a f(b) type instance
template <class Function, class A, class B>
struct Functor
{
		template <class T>
		using typeof_f_applied_to = decltype((declval<Function>())(declval<T>()));

		template <class Mapper>
		Functor(const Mapper& identity, const typeof_f_applied_to<A>& _f_a)
		: mapper(identity), f_a(_f_a)
		{
		}

		template <class Mapper>
		Functor<Function,A,decltype((declval<Mapper>())(declval<B>()))> fmap(const Mapper& map) const
		{
				using B_i = decltype((declval<Mapper>())(declval<B>()));
				auto composite_mapper = [this, &map](const A& a)
					{
						return std::forward<B_i>(map(std::forward<B>(mapper(a))));
					};
				return Functor<Function,A,B_i>(composite_mapper, f_a);
		}

		template <class B_i>
		Functor<Function,A,B_i> fmap(B_i(* const map)(const B&)) const
		{
				auto composite_mapper = [this, &map](const A& a)
					{
						return std::forward<B_i>(map(std::forward<B>(mapper(a))));
					};
				return Functor<Function,A,B_i>(composite_mapper, f_a);
		}

		typeof_f_applied_to<B> get() const
		{
				return Function::apply(mapper, f_a);
		}

		const std::function<B(A)> mapper;
		const typeof_f_applied_to<A>& f_a;
};

// Monad
template <class Function, class A, class B>
struct Monad : Functor<Function, A, Functor<Function, A, B>>
{
		Functor<Function, A, B> mjoin()
		{
				// TODO
		}
};

struct Vector
{
		template <class B> std::vector<B> operator()(B...) const;
		template <class B, class A>
		static std::vector<B> apply(const std::function<B(A)>& mapper, const std::vector<A>& source)
		{
				std::vector<B> target(source.size());
				std::transform(source.begin(), source.end(), target.begin(), mapper);
				return target;
		}
};

template <class A>
Functor<Vector,A,A> make_functor(const std::vector<A>& f_a)
{
		return Functor<Vector,A,A>([](A&& a) { return std::forward<A>(a); }, f_a);
}

double sqrt(const int& a)
{
		return std::sqrt(a);
}

void test(std::vector<int>& l)
{
		auto r = make_functor(l)
//			.fmap([](int x)
//			{
//				std::vector<int> v(x);
//				std::iota(v.begin(), v.end(), 1);
//				return make_functor(v);
//			})
//			.mjoin()
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
		std::vector<int> l(5);
		std::iota(l.begin(), l.end(), 1);
		test(l);
		return 0;
}
