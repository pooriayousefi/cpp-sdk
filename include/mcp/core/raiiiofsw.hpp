
#pragma once
#include <type_traits>
#include <filesystem>
#include <stdexcept>
#include <typeinfo>
#include <fstream>
#include <string>

/**********************************************************************************************
*
*                   			RAII Input/Output File Stream Wrapper
*                   			-----------------------
*    			This header provides a RAII wrapper for basic input/output
*    			file streams. It includes:
*    			- A BasicInputFileStreamWrapper class template for managing file streams.
*    			- A BasicOutputFileStreamWrapper class template for managing file streams.
*    			- Specialization for std::byte for binary file streams.
*
*                   			Developed by: Pooria Yousefi
*				   				Date: 2025-06-26
*				   				License: MIT
*
**********************************************************************************************/

namespace pooriayousefi::core
{
	namespace raii
	{
		template<typename Elem, typename Traits = std::char_traits<Elem>, typename Alloc = std::allocator<Elem>>
		struct BasicInputFileStreamWrapper
		{
			using file_stream_type = std::basic_ifstream<Elem, Traits>;
			using type = BasicInputFileStreamWrapper<Elem, Traits, Alloc>;
			using string_type = std::basic_string<Elem, Traits, Alloc>;
			using stream_buffer_iterator = std::istreambuf_iterator<Elem, Traits>;

			file_stream_type file_stream;

			BasicInputFileStreamWrapper() :file_stream{} {}
			virtual ~BasicInputFileStreamWrapper() { if (is_open()) close(); }

			template<typename T> constexpr type& operator>>(T& value) { file_stream >> value; return *this; }

			bool is_open() { return file_stream.is_open(); }
			void close() { file_stream.close(); }
			void open(std::filesystem::path file_path, std::ios_base::openmode open_mode = std::ios_base::in)
			{
				file_stream.open(file_path, std::ios_base::in | open_mode);
				if (!is_open())
					throw std::runtime_error(
						std::string{
							std::string{ "ERROR! Cannot open "} +
							file_path.string() +
							std::string{ " file in raii::BasicInputFileStreamWrapper<" } +
							std::string{ typeid(Elem).name() } +
							std::string{ ", " } +
							std::string{ typeid(Traits).name() } +
							std::string{ ", " } +
							std::string{ typeid(Alloc).name() } +
							std::string{ ">::open() method." }
						}.c_str()
					);
			}
		};

		template<>
		struct BasicInputFileStreamWrapper<std::byte>
		{
			using file_stream_type = std::basic_ifstream<std::byte>;
			using type = BasicInputFileStreamWrapper<std::byte>;
			using string_type = std::basic_string<std::byte>;
			using stream_buffer_iterator = std::istreambuf_iterator<std::byte>;

			file_stream_type file_stream;

			BasicInputFileStreamWrapper() :file_stream{} {}
			virtual ~BasicInputFileStreamWrapper() { if (is_open()) close(); }

			template<typename T> constexpr type& operator>>(T& value) { file_stream >> value; return *this; }

			bool is_open() { return file_stream.is_open(); }
			void close() { file_stream.close(); }
			void open(std::filesystem::path file_path, std::ios_base::openmode open_mode = std::ios_base::in | std::ios_base::binary)
			{
				file_stream.open(file_path, std::ios_base::in | std::ios_base::binary | open_mode);
				if (!is_open())
					throw std::runtime_error(
						std::string{
							std::string{"ERROR! Cannot open "} +
							file_path.string() +
							std::string{" file in raii::BasicInputFileStreamWrapper<std::byte>::open() method." }
						}.c_str()
					);
			}
		};

		template<typename Elem, typename Traits = std::char_traits<Elem>, typename Alloc = std::allocator<Elem>>
		struct BasicOutputFileStreamWrapper
		{
			using file_stream_type = std::basic_ofstream<Elem, Traits>;
			using type = BasicOutputFileStreamWrapper<Elem, Traits, Alloc>;
			using string_type = std::basic_string<Elem, Traits, Alloc>;
			using stream_buffer_iterator = std::ostreambuf_iterator<Elem, Traits>;

			file_stream_type file_stream;

			BasicOutputFileStreamWrapper() :file_stream{} {}
			virtual ~BasicOutputFileStreamWrapper() { if (is_open()) close(); }

			template<typename T> constexpr type& operator<<(const T& value) { file_stream << value; return *this; }
			template<typename T> constexpr type& operator<<(T&& value) noexcept { file_stream << value; return *this; }

			bool is_open() { return file_stream.is_open(); }
			void close() { file_stream.flush(); file_stream.close(); }
			void open(std::filesystem::path file_path, std::ios_base::openmode open_mode = std::ios_base::out)
			{
				file_stream.open(file_path, std::ios::out | open_mode);
				if (!is_open())
					throw std::runtime_error(
						std::string{
							std::string{"ERROR! Cannot open "} +
							file_path.string() +
							std::string{" file in raii::BasicOutputFileStreamWrapper<"} +
							std::string{typeid(Elem).name()} +
							std::string{", "} +
							std::string{typeid(Traits).name()} +
							std::string{", "} +
							std::string{ typeid(Alloc).name() } +
							std::string{ "::open() method." }
						}.c_str()
					);
			}
		};

		template<>
		struct BasicOutputFileStreamWrapper<std::byte>
		{
			using file_stream_type = std::basic_ofstream<std::byte>;
			using type = BasicOutputFileStreamWrapper<std::byte>;
			using string_type = std::basic_string<std::byte>;
			using stream_buffer_iterator = std::ostreambuf_iterator<std::byte>;

			file_stream_type file_stream;

			BasicOutputFileStreamWrapper() :file_stream{} {}
			virtual ~BasicOutputFileStreamWrapper() { if (is_open()) close(); }

			template<typename T> constexpr type& operator<<(const T& value) { file_stream << value; return *this; }
			template<typename T> constexpr type& operator<<(T&& value) noexcept { file_stream << value; return *this; }

			bool is_open() { return file_stream.is_open(); }
			void close() { file_stream.flush(); file_stream.close(); }
			void open(std::filesystem::path file_path, std::ios_base::openmode open_mode = std::ios_base::out | std::ios_base::binary)
			{
				file_stream.open(file_path, std::ios::out | std::ios_base::binary | open_mode);
				if (!is_open())
					throw std::runtime_error(
						std::string{
							std::string{"ERROR! Cannot open "} +
							file_path.string() +
							std::string{" file in raii::BasicOutputFileStreamWrapper<std::byte>::open() method." }
						}.c_str()
					);
			}
		};

		namespace native
		{
			namespace narrow_encoded 
			{
				using InputFileStreamWrapper = BasicInputFileStreamWrapper<char>;
				using OutputFileStreamWrapper = BasicOutputFileStreamWrapper<char>;
			}
			namespace wide_encoded
			{
				using InputFileStreamWrapper = BasicInputFileStreamWrapper<wchar_t>;
				using OutputFileStreamWrapper = BasicOutputFileStreamWrapper<wchar_t>;
			}
		}
        #if __cplusplus >= 202002L
		namespace utf8
		{
			using InputFileStreamWrapper = BasicInputFileStreamWrapper<char8_t>;
			using OutputFileStreamWrapper = BasicOutputFileStreamWrapper<char8_t>;
		}
        #endif
		namespace binary
		{
			using InputFileStreamWrapper = BasicInputFileStreamWrapper<std::byte>;
			using OutputFileStreamWrapper = BasicOutputFileStreamWrapper<std::byte>;
		}
	}
}
