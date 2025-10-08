
#pragma once
#include <concepts>
#include <type_traits>
#include <thread>
#include <ratio>
#include <utility>
#include <chrono>
#include <functional>
#include <cstdint>
#include <numbers>
#include <vector>
#include <algorithm>
#include <ranges>
#include <iterator>
#include <unordered_map>
#include <string_view>
#include <random>
#include <variant>
#include <iostream>

/**********************************************************************************************
*
*                   			    Utilities Header
*                   			-----------------------
*    		This header provides general utility functions and classes.
*    		It includes:
*    		- A wait_for class template for sleeping for various time durations.
*    		- A runtime function template for measuring the execution time of a callable.
*    		- A convert namespace with functions for unit conversions and number base conversions.
*    		- A countdown function template for displaying a countdown in seconds.
*    		- An iterate function template for iterating over a range with a specified step size
*    		- Specializations of standard functors for std::byte and std::reference_wrapper.
*    		- A histogram function template for counting occurrences of elements in a range.
*    		- A frequencies function template for counting word frequencies in a string view.
*    		- A do_n_times_shuffle_and_sample function template for shuffling and sampling a range.
*    		- A Result struct template for encapsulating expected values or exceptions.
*
*                   			Developed by: Pooria Yousefi
*				   				Date: 2025-06-26
*				   				License: MIT
*
**********************************************************************************************/

namespace pooriayousefi::core
{
    template<typename T> concept Arithmetic = std::floating_point<T> || std::integral<T>;

    template<Arithmetic T> 
    class wait_for
    {
    public:
        wait_for() = delete;
        wait_for(T value) :m_value{ value } {}
        inline void nanoseconds() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1'000'000'000>>(m_value)); }
        inline void microseconds() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1'000'000>>(m_value)); }
        inline void milliseconds() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1'000>>(m_value)); }
        inline void seconds() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1>>(m_value)); }
        inline void minutes() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<60>>(m_value)); }
        inline void hours() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<3'600>>(m_value)); }
        inline void days() { std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<86'400>>(m_value)); }
    private:
        T m_value;
    };

    template<typename F, typename... Args> 
    constexpr decltype(auto) runtime(F&& f, Args&&... args)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
        {
            auto ti{ std::chrono::high_resolution_clock::now() };
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            auto tf{ std::chrono::high_resolution_clock::now() };
            return std::chrono::duration<double>(tf - ti).count();
        }
        else if constexpr (!std::is_void_v<std::invoke_result_t<F, Args...>>)
        {
            auto ti{ std::chrono::high_resolution_clock::now() };
            auto retval{ std::invoke(std::forward<F>(f), std::forward<Args>(args)...) };
            auto tf{ std::chrono::high_resolution_clock::now() };
            return std::make_pair(std::move(retval), std::chrono::duration<double>(tf - ti).count());
        }
    }

    namespace convert
    {
        template<std::floating_point T> constexpr T degrees_to_radians(T x) { return x * std::numbers::pi_v<T> / (T)180; }
        template<std::floating_point T> constexpr T radians_to_degrees(T x) { return x * (T)180 / std::numbers::pi_v<T>; }
        template<std::floating_point T> constexpr T Celsius_to_Fahrenheit(T x) { return (x * (T)9 / (T)5) + (T)32; }
        template<std::floating_point T> constexpr T Fahrenheit_to_Celsius(T x) { return (x - (T)32) * (T)5 / (T)9; }
    }

    template<std::integral I> 
    constexpr void countdown(I nsec)
    {
        std::cout << "\nT-" << nsec << ' ';
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (auto i{ static_cast<int64_t>(nsec) - static_cast<int64_t>(1) }; i >= static_cast<int64_t>(0); --i)
        {
            std::cout << i << ' ';
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    template<std::input_or_output_iterator It, std::invocable<std::iter_value_t<It>&> F>
    constexpr void iterate(It begin, size_t n, size_t step_size, F&& f)
    {
        size_t c(0);
        auto it = begin;
        do
        {
            std::invoke(std::forward<F>(f), *it);
            c++;
        } while (c < n && [&]() { it = std::ranges::next(it, step_size); return true; }());
    }
}

namespace std
{
	template<> struct hash<byte>
	{
		constexpr const size_t operator()(const byte& b) const
		{
			return to_integer<size_t>(b);
		}
	};

	template<> struct equal_to<byte>
	{
		constexpr const bool operator()(const byte& lb, const byte& rb) const
		{
			return to_integer<size_t>(lb) == to_integer<size_t>(rb);
		}
	};

	template<class T> struct hash<reference_wrapper<const T>>
	{
		constexpr const size_t operator()(const reference_wrapper<const T>& ref) const
		{
			hash<T> hasher{};
			return hasher(ref.get());
		}
	};

	template<class T> struct equal_to<reference_wrapper<const T>>
	{
		constexpr const bool operator()(const reference_wrapper<const T>& lhs, const reference_wrapper<const T>& rhs) const
		{
			return lhs.get() == rhs.get();
		}
	};
}
