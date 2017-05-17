#include <functional>
#include <type_traits>

template <typename interface>
struct _interface {
    _interface(interface* t): _impl(t) {}
    _interface(std::function<interface*(void)> f): _impl(f()) {}

    interface* operator->() { return _impl; }
    const interface* const operator->() const { return _impl; }

private:
    interface* _impl;
};

namespace detail {
    template <typename... ts>
    struct any_true;

    template <typename t, typename... ts>
    struct any_true<t, ts...> {
        static const bool value = t::value || any_true<ts...>::value;
    };

    template <typename t>
    struct any_true<t> {
        static const bool value = t::value;
    };

	template <typename... ts>
	struct all_same;

	template <typename t1, typename t2, typename... ts>
	struct all_same<t1, t2, ts...> {
		using type = t1;
        static const bool value = std::is_same<t1, t2>::value && all_same<t2, ts...>::value;
	};

	template <typename t1, typename t2>
	struct all_same<t1, t2> {
		using type = t1;
		static const bool value = std::is_same<t1, t2>::value;
	};

	template <typename t, typename... ts>
	struct type_getter { using type = t; };
} // detail


template <typename... ts> // interfaces
struct interface_wrapper: virtual _interface<ts>... {
    interface_wrapper(ts*... args): _interface<ts>(args)... {}
    interface_wrapper(std::function<ts*(void)>... fns): _interface<ts>(fns)... {}

	template <typename... cts> // concrete types constructed with arguments of their own
	static interface_wrapper<ts...>* create(typename cts::Args... args) { return new interface_wrapper<ts...>(new cts(args...)...); }

	template <typename... cts> // concrete types constructed with the same argument
	static interface_wrapper<ts...>* create(typename std::enable_if<detail::all_same<typename cts::Args...>::value, typename detail::type_getter<typename cts::Args...>::type>::type arg) { return new interface_wrapper<ts...>(new cts(arg)...); }

    template <typename it>
    _interface<it>& get() {
        static_assert(std::is_base_of<_interface<it>, interface_wrapper<ts...>>::value, "Non-exsitent interface requested");
        return *static_cast<_interface<it>*>(this);
    }
};
