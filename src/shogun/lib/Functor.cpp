#include <functional>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <type_traits>
#include <cmath>

using std::declval;

namespace Functional
{

template <class> class Vector;

// fmap :: Functor f => (a -> b) -> f a -> f b
// takes a function that maps a -> b
// takes a f(a) type instance
// returns a f(b) type instance
template <class B, class A>
Vector<B> fmap(const std::function<B(A)>& mapper, const Vector<A>& source);

}

template <class Functor, class A, class B>
struct Eval
{
	template <class T>
	using typeof_f_of = decltype((declval<Functor>())(declval<T>()));

	// fmap :: Functor f => (a -> b) -> f a -> f b
	Eval(std::function<B(A)>&& map, const typeof_f_of<A>& _f_a) : mapper(map), f_a(_f_a)
	{
	}

	// (.) :: (b -> c) -> (a -> b) -> a -> c
	template <class Map>
	Eval<Functor,A,decltype((declval<Map>())(declval<B>()))> composite(const Map& map) const
	{
		using C = decltype((declval<Map>())(declval<B>()));
		auto composite_mapper = [this, &map](const A& a)
		{
			return std::forward<C>(map(std::forward<B>(mapper(a))));
		};
		return Eval<Functor,A,C>(composite_mapper, f_a);
	}

	// (.) :: (b -> c) -> (a -> b) -> a -> c
	template <class C>
	Eval<Functor,A,C> composite(C(* const map)(const B&)) const
	{
		auto composite_mapper = [this, &map](const A& a)
		{
			return std::forward<C>(map(std::forward<B>(mapper(a))));
		};
		return Eval<Functor,A,C>(composite_mapper, f_a);
	}

	typeof_f_of<B> yield() const
	{
		return Functional::fmap(mapper, f_a);
	}

	const std::function<B(A)> mapper;
	const typeof_f_of<A>& f_a;
};

// Monad
// (>>=) :: Monad m => m a -> (a -> m b) -> m b
// (>>)  :: Monad m => m a -> m b -> m b
template <class Monad, class A, class B>
struct Eval<Monad, A, Eval<Monad, A, B>>
{
	template <class T>
	using typeof_m_of = decltype((declval<Monad>())(declval<T>()));

	Eval(std::function<Eval<Monad, A, B>(A)>&& map, const typeof_m_of<A>& _f_a)
	: mapper(map), f_a(_f_a)
	{
		std::cout << "Monad constructed" << std::endl;
	}

	Eval<Monad, A, B> mjoin()
	{
		// TODO
		typeof_m_of<Eval<Monad, A, B>> f_f_a;
	}

	const std::function<Eval<Monad, A, B>(A)> mapper;
	const typeof_m_of<A>& f_a;
};

namespace Functional
{

template <class T>
struct Vector
{
	const std::vector<T>& vector;
	Vector(size_t size) : vector(std::vector<T>(size))
	{
		std::cout << "size = "<< vector.size() << std::endl;
	}
	Vector(const std::vector<T>& _vector) : vector(_vector)
	{
		std::cout << "size = "<< _vector.size() << std::endl;
		std::cout << "size = "<< vector.size() << std::endl;
	}

	decltype(vector.size()) size() const
	{
		std::cout << "size = "<< vector.size() << std::endl;
		return vector.size();
	}

	decltype(vector.begin()) begin() const
	{
		return vector.begin();
	}

	decltype(vector.end()) end() const
	{
		return vector.end();
	}

	decltype(const_cast<std::vector<T>*>(&vector)->begin()) begin()
	{
		return const_cast<std::vector<T>*>(&vector)->begin();
	}

	decltype(const_cast<std::vector<T>*>(&vector)->end()) end()
	{
		return const_cast<std::vector<T>*>(&vector)->end();
	}

	// satisfies functor requirement
	template <class A> Vector<A> operator()(A...) const;

	// satisfies monad requirement
	// TODO
};

// fmap :: Functor f => (a -> b) -> f a -> f b
template <class B, class A>
Vector<B> fmap(const std::function<B(A)>& mapper, const Vector<A>& source)
{
	std::cout << "ahoy, fmap!"<< std::endl;
	std::vector<B> tmp(source.size());
	std::cout << "ahoy, alloc!"<< std::endl;
	Vector<B> target(tmp);
	std::for_each(source.begin(), source.end(), [](A s)
	{
		std::cout << "ahoy, element, " << s << std::endl;
	});
	std::transform(source.begin(), source.end(), target.begin(), mapper);
	return target;
}

template <class T>
Eval<Vector<T>,T,T> as_functor(const std::vector<T>& f_a)
{
	auto identity_map = [](T&& a) { return std::forward<T>(a); };
	return Eval<Vector<T>,T,T>(identity_map, Vector<T>(f_a));
}

}

double sqrt(const int& a)
{
	return std::sqrt(a);
}

void test(const std::vector<int>& l)
{
	auto r = Functional::as_functor(l)
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
			std::cout << s << std::endl;
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
	std::vector<int> l(10);
	std::iota(l.begin(), l.end(), 1);
	test(l);
	return 0;
}
