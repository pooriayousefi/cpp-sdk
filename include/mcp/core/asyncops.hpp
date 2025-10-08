
#pragma once
#include <stdexcept>
#include <exception>
#include <iostream>
#include <coroutine>
#include <vector>
#include <forward_list>
#include <thread>
#include <variant>
#include <utility>
#include <semaphore>
#include <memory>
#include <cassert>

/**********************************************************************************************
*
*                   			Asynchronous Operations
*                   			-----------------------
*    			This header provides utilities for asynchronous programming
*    			using C++20 coroutines. It includes:
*    			- A Generator class template for creating coroutine-based generators.
*    			- A GeneratorFactory class template for managing pools of objects.
*    			- An awaitable Task class template for defining asynchronous tasks.
*    			- A SyncWaitTask class template and sync_wait function for synchronously
*    			  waiting on asynchronous tasks to complete.
*
*                   			Developed by: Pooria Yousefi
*				   				Date: 2025-06-26
*				   				License: MIT
*
**********************************************************************************************/

// namespace pooriayousefi::core
namespace pooriayousefi::core
{
    template<class T> 
	struct Generator
	{
		struct Promise
		{
			T current_value;
			inline decltype(auto) initial_suspend() { return std::suspend_always{}; }
			inline decltype(auto) final_suspend() noexcept { return std::suspend_always{}; }
			inline decltype(auto) get_return_object() { return Generator{ std::coroutine_handle<Promise>::from_promise(*this) }; }
			inline decltype(auto) return_void() { return std::suspend_never{}; }
			inline decltype(auto) yield_value(T&& value) noexcept { current_value = value; return std::suspend_always{}; }
			inline void unhandled_exception() { std::terminate(); }
		};
        using promise_type = Promise;
		struct Sentinel {};
		struct Iterator
		{
			using iterator_category = std::input_iterator_tag;
			using value_type = T;
			using difference_type = ptrdiff_t;
			using pointer = T*;
			using reference = T&;
			using const_reference = const T&;
			std::coroutine_handle<promise_type> handle;
			explicit Iterator(std::coroutine_handle<promise_type>& h) :handle{ h } {}
			inline Iterator& operator++()
			{
				handle.resume();
				return *this;
			}
			inline void operator++(int) { (void)operator++(); }
			inline reference operator*() { return handle.promise().current_value; }
			inline pointer operator->() { return std::addressof(operator*()); }
			inline const_reference operator*() const { return handle.promise().current_value; }
			inline pointer operator->() const { return std::addressof(operator*()); }
			inline bool operator==(Sentinel) { return handle.done(); }
			inline bool operator==(Sentinel) const { return handle.done(); }
		};
		std::coroutine_handle<promise_type> handle;
		explicit Generator(std::coroutine_handle<promise_type> h) :handle{ h } {}
		~Generator() { if (handle) handle.destroy(); }
		Generator(const Generator&) = delete;
		Generator(Generator&& other) noexcept :handle(other.handle) { other.handle = nullptr; }
		constexpr Generator& operator=(const Generator&) = delete;
		constexpr Generator& operator=(Generator&& other) noexcept { handle = other.handle; other.handle = nullptr; return *this; }
		inline T get_value() { return handle.promise().current_value; }
		inline bool next() { handle.resume(); return !handle.done(); }
		inline bool resume() { handle.resume(); return !handle.done(); }
		inline decltype(auto) begin()
		{
			handle.resume();
			return Iterator{ handle };
		}
		inline decltype(auto) end() { return Sentinel{}; }
		inline T get_next_value()
		{
			next();
			if (handle.done()) throw std::out_of_range{ "Generator exhausted" };
			return get_value();
		}
	};

    template<class T, size_t N = 128> 
	class GeneratorFactory
	{
	public:
        using Pool = std::vector<T>;
        using Pools = std::forward_list<Pool>;
        static constexpr inline size_t number_of_objects_in_each_pool = N;

		GeneratorFactory():m_pools{}, m_object_counter{ 0 }
		{
			m_pools.emplace_front(Pool(number_of_objects_in_each_pool, T{}));
		}

		virtual ~GeneratorFactory()
		{
			for (auto& pool : m_pools)
			{
				pool.clear();
			}
			m_pools.clear();
		}

		inline Generator<std::shared_ptr<T>> generate()
		{
			while (true)
			{
                if (m_object_counter < number_of_objects_in_each_pool)
                {
                    co_yield std::make_shared<T>(m_pools.begin()->data()[m_object_counter++]);
                }
                else
                {
                    m_pools.emplace_front(Pool(number_of_objects_in_each_pool, T{}));
                    m_object_counter = 0;
                }
			}
		}
	private:
		Pools m_pools;
		size_t m_object_counter;
	};

	template<class T> 
    struct Task
	{
		struct promise_type
		{
			std::variant<std::monostate, T, std::exception_ptr> result;
			std::coroutine_handle<> continuation;
			constexpr decltype(auto) get_return_object() noexcept { return Task{ *this }; }
			constexpr void return_value(T value) { result.template emplace<1>(std::move(value)); }
			constexpr void unhandled_exception() noexcept { result.template emplace<2>(std::current_exception()); }
			constexpr decltype(auto) initial_suspend() { return std::suspend_always{}; }
			struct awaitable
			{
				constexpr bool await_ready() noexcept { return false; }
				constexpr decltype(auto) await_suspend(std::coroutine_handle<promise_type> h) noexcept
				{
					return h.promise().continuation;
				}
				constexpr void await_resume() noexcept {}
			};
			constexpr decltype(auto) final_suspend() noexcept { return awaitable{}; }
		};
		std::coroutine_handle<promise_type> handle;
		explicit Task(promise_type& p) noexcept :handle{ std::coroutine_handle<promise_type>::from_promise(p) } {}
		Task(Task&& t) noexcept :handle{ t.handle } {}
		~Task() { if (handle) handle.destroy(); }
		constexpr bool await_ready() { return false; }
		constexpr decltype(auto) await_suspend(std::coroutine_handle<> c)
		{
			handle.promise().continuation = c;
			return handle;
		}
		constexpr T await_resume()
		{
			auto& result = handle.promise().result;
			if (result.index() == 1)
				return std::get<1>(std::move(result));
			else
				std::rethrow_exception(std::get<2>(std::move(result)));
		}
	};
	template<> 
    struct Task<void>
	{
		struct promise_type
		{
			std::exception_ptr e;
			std::coroutine_handle<> continuation;
			inline decltype(auto) get_return_object() noexcept { return Task{ *this }; }
			constexpr void return_void() {}
			inline void unhandled_exception() noexcept { e = std::current_exception(); }
			constexpr decltype(auto) initial_suspend() { return std::suspend_always{}; }
			struct awaitable
			{
				constexpr bool await_ready() noexcept { return false; }
				inline decltype(auto) await_suspend(std::coroutine_handle<promise_type> h) noexcept
				{
					return h.promise().continuation;
				}
				constexpr void await_resume() noexcept {}
			};
			constexpr decltype(auto) final_suspend() noexcept { return awaitable{}; }
		};
		std::coroutine_handle<promise_type> handle;
		explicit Task(promise_type& p) noexcept :handle{ std::coroutine_handle<promise_type>::from_promise(p) } {}
		Task(Task&& t) noexcept :handle{ t.handle } {}
		~Task() { if (handle) handle.destroy(); }
		constexpr bool await_ready() { return false; }
		inline decltype(auto) await_suspend(std::coroutine_handle<> c)
		{
			handle.promise().continuation = c;
			return handle;
		}
		inline void await_resume()
		{
			if (handle.promise().e)
				std::rethrow_exception(handle.promise().e);
		}
	};

	template<class T> using ResultType = decltype(std::declval<T&>().await_resume());

	template<class T> 
    struct SyncWaitTask
	{
		struct promise_type
		{
			T* value{ nullptr };
			std::exception_ptr error{ nullptr };
			std::binary_semaphore sema4{ 0 };
			inline SyncWaitTask get_return_object() noexcept { return SyncWaitTask{ *this }; }
			constexpr void unhandled_exception() noexcept { error = std::current_exception(); }
			constexpr decltype(auto) yield_value(T&& x) noexcept
			{
				value = std::addressof(x);
				return final_suspend();
			}
			constexpr decltype(auto) initial_suspend() noexcept { return std::suspend_always{}; }
			struct awaitable
			{
				constexpr bool await_ready() noexcept { return false; }
				constexpr void await_suspend(std::coroutine_handle<promise_type> h) noexcept { h.promise().sema4.release(); }
				constexpr void await_resume() noexcept {}
			};
			constexpr decltype(auto) final_suspend() noexcept { return awaitable{}; }
			constexpr void return_void() noexcept { assert(false); }
		};
		std::coroutine_handle<promise_type> handle;
		explicit SyncWaitTask(promise_type& p) noexcept :handle{ std::coroutine_handle<promise_type>::from_promise(p) } {}
		SyncWaitTask(SyncWaitTask&& t) noexcept :handle{ t.handle } {}
		~SyncWaitTask() { if (handle) handle.destroy(); }
		inline T&& get()
		{
			auto& p = handle.promise();
			handle.resume();
			p.sema4.acquire();
			if (p.error)
				std::rethrow_exception(p.error);
			return static_cast<T&&>(*p.value);
		}
	};

	template<class T> ResultType<T> sync_wait(T&& Task)
	{
		if constexpr (std::is_void_v<ResultType<T>>)
		{
			struct empty_type {};
			auto coro = [&]() -> SyncWaitTask<empty_type>
				{
					co_await std::forward<T>(Task);
					co_yield empty_type{};
					assert(false);
				};
			coro().get();
		}
		else
		{
			auto coro = [&]() -> SyncWaitTask<ResultType<T>>
				{
					co_yield co_await std::forward<T>(Task);
					assert(false);
				};
			return coro().get();
		}
	}
}
