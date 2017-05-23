#include <functional>
#include <type_traits>

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

	template<typename> struct type_eraser { using type = int; };
	template<typename t, typename... ts> struct type_getter { using type = t; };

	template <typename ct>
	struct interface_helper {
		// TODO: figure out how to pass multiple arguments not using tuples (might not be possible)
		// Possible fix: helper struct template with just the arguments
		// <template <typename... Args> typename>
		template <typename... Ignored>
		static ct* create(typename ct::Args arg, Ignored... rest) {
			static_assert(std::is_constructible<ct, typename ct::Args>::value, "One of the interfaces could not be creates using provided arguments");
			return new ct(arg);
		}
	};

	template <bool B, typename T = void>
	struct disable_if {
		using type = T;
	};

	template <typename T>
	struct disable_if<true,T> {
	};

	struct multi_wrapper_trait {};

} // detail

template <typename interface>
struct _interface {
private:
    interface* _impl;

public:
    _interface(interface* t): _impl(t) {}
    _interface(std::function<interface*(void)> f): _impl(f()) {}

    interface* operator->() { return _impl; }
    const interface* const operator->() const { return _impl; }

	template <typename RV, typename... Args>
	typename std::enable_if<!std::is_base_of<detail::multi_wrapper_trait, interface>::value, std::function<RV(Args...)>>::type
	call(RV (interface::*func)(Args...)) {
		return [this, func](Args... args) { (_impl->*func)(args...); };
	}

	template <typename RV, typename... Args>
	typename std::enable_if<std::is_base_of<detail::multi_wrapper_trait, interface>::value, std::function<RV(Args...)>>::type
	call(RV (interface::*func)(Args...)) {
		return [this, func](Args... args) { return _impl->operator[](func); };
	}
};

#include <vector>
template <typename IF>
struct multi_interface: IF, detail::multi_wrapper_trait {
	template <typename... cts>
	multi_interface(cts*... args): ifs(args...) {}

	template <typename... cts>
	struct concrete_creator {
		template <typename... Args>
		static multi_interface<IF>* create(Args... args) {
			return new multi_interface<IF>(detail::interface_helper<cts>::create(args...)...);
		}
	};

	template <typename RV, typename IT, typename... Args>
	auto operator[](RV (IT::*func)(Args...)) -> std::function<RV(Args...)> {
		// The interface better not be deleted!
		// we cannot afford passing the vector by value every time
		return [this, func](Args... args) {
			for (auto i: ifs) {
				(i->*func)(args...);
			}
		};
	}

private:
	std::vector<IF*> ifs;
};


template <typename... ts> // interfaces
struct interface_wrapper: virtual _interface<ts>... {
    interface_wrapper(ts*... args): _interface<ts>(args)... {}
    interface_wrapper(std::function<ts*(void)>... fns): _interface<ts>(fns)... {}

	template <typename... cts>
	struct concrete_creator {
		template <typename... Args>
		static interface_wrapper<ts...>* create(Args... args) {
			return new interface_wrapper<ts...>(detail::interface_helper<cts>::create(args...)...);
		}
	};

	template <typename RV, typename IT, typename... Args>
	auto call(RV (IT::*func)(Args...), Args... args) -> RV {
		return static_cast<_interface<IT>*>(this)->call(func)(args...);
	}

	template <typename RV, typename IT, typename... Args>
	auto operator()(RV (IT::*func)(Args...), Args... args) -> RV {
		return static_cast<_interface<IT>*>(this)->call(func)(args...);
	}

	template <typename RV, typename IT, typename... Args>
	auto operator[](RV (IT::*func)(Args...)) -> std::function<RV(Args...)> {
		return static_cast<_interface<IT>*>(this)->call(func);
		//IT* iface = get<IT>().operator->();
		//return [iface, func](Args... args){ (iface->*func)(args...); };
	}

    template <typename it>
    _interface<it>& get() {
        static_assert(std::is_base_of<_interface<it>, interface_wrapper<ts...>>::value, "Non-exsitent interface requested");
        return *static_cast<_interface<it>*>(this);
    }
};
