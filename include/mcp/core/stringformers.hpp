
#pragma once
#include <cctype>
#include <string>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>

/**********************************************************************************************
*
*                   			String Transformers
*                   			-------------------
*    			This header provides utility functions for string manipulation,
*    			including case conversion and tokenization.
*
*                   			Developed by: Pooria Yousefi
*				   				Date: 2025-06-26
*				   				License: MIT
*
**********************************************************************************************/

namespace pooriayousefi::core
{
    template<class Enc, class EncTraits = std::char_traits<Enc>, class EncAlloc = std::allocator<Enc>>
    constexpr decltype(auto) to_lowercase(const std::basic_string<Enc, EncTraits, EncAlloc>& word)
    {
        std::basic_string<Enc, EncTraits, EncAlloc> lowercased_word{};
        lowercased_word.resize(std::ranges::size(word));
        std::ranges::transform(std::ranges::cbegin(word), std::ranges::cend(word),
            std::ranges::begin(lowercased_word), [](const auto& c) { return std::tolower(c); });
        return lowercased_word;
    }
    template<class Enc, class EncTraits = std::char_traits<Enc>, class EncAlloc = std::allocator<Enc>>
    constexpr decltype(auto) to_lowercase(std::basic_string_view<Enc, EncTraits> word_view)
    {
        std::basic_string<Enc, EncTraits, EncAlloc> lowercased_word{};
        lowercased_word.resize(std::ranges::size(word_view));
        std::ranges::transform(std::ranges::cbegin(word_view), std::ranges::cend(word_view),
            std::ranges::begin(lowercased_word), [](const auto& c) { return std::tolower(c); });
        return lowercased_word;
    }

    template<class Enc, class EncTraits = std::char_traits<Enc>, class EncAlloc = std::allocator<Enc>>
    constexpr decltype(auto) to_uppercase(const std::basic_string<Enc, EncTraits, EncAlloc>& word)
    {
        std::basic_string<Enc, EncTraits, EncAlloc> uppercased_word{};
        uppercased_word.resize(std::ranges::size(word));
        std::ranges::transform(std::ranges::cbegin(word), std::ranges::cend(word),
            std::ranges::begin(uppercased_word), [](const auto& c) { return std::toupper(c); });
        return uppercased_word;
    }
    template<class Enc, class EncTraits = std::char_traits<Enc>, class EncAlloc = std::allocator<Enc>>
    constexpr decltype(auto) to_uppercase(std::basic_string_view<Enc, EncTraits> word_view)
    {
        std::basic_string<Enc, EncTraits, EncAlloc> uppercased_word{};
        uppercased_word.resize(std::ranges::size(word_view));
        std::ranges::transform(std::ranges::cbegin(word_view), std::ranges::cend(word_view),
            std::ranges::begin(uppercased_word), [](const auto& c) { return std::toupper(c); });
        return uppercased_word;
    }

    template<class T, class Traits = std::char_traits<T>>
    constexpr void tokenize(
        std::basic_string_view<T, Traits> src, 
        std::basic_string_view<T, Traits> delim,
        std::vector<std::basic_string_view<T, Traits>>& tokens
    )
    {
        tokens.clear();
        tokens.reserve(src.size());

        auto last_pos = src.find_first_not_of(delim, 0);
        auto pos = src.find_first_of(delim, last_pos);

        while (pos != std::basic_string_view<T, Traits>::npos || last_pos != std::basic_string_view<T, Traits>::npos)
        {
            tokens.emplace_back(src.substr(last_pos, pos - last_pos));
            last_pos = src.find_first_not_of(delim, pos);
            pos = src.find_first_of(delim, last_pos);
        }
    }
    template<class T, class Traits = std::char_traits<T>>
	constexpr void tokenize(
        std::basic_string_view<T, Traits> src, 
        std::basic_string_view<T, Traits> delim,
        std::unordered_set<std::basic_string_view<T, Traits>>& tokens
    )
	{
        tokens.clear();
        tokens.reserve(src.size());

        auto last_pos = src.find_first_not_of(delim, 0);
        auto pos = src.find_first_of(delim, last_pos);

		while (pos != std::basic_string_view<T, Traits>::npos || last_pos != std::basic_string_view<T, Traits>::npos)
		{
			tokens.emplace(src.substr(last_pos, pos - last_pos));
			last_pos = src.find_first_not_of(delim, pos);
			pos = src.find_first_of(delim, last_pos);
		}
	}
	template<class T, class Traits = std::char_traits<T>>
	auto tokenize(
        std::basic_string_view<T, Traits> src, 
        std::basic_string_view<T, Traits> delim,
        std::unordered_map<std::basic_string_view<T, Traits>, size_t>& tokens
    )
	{
        tokens.clear();
        tokens.reserve(src.size());

		auto last_pos = src.find_first_not_of(delim, 0);
		auto pos = src.find_first_of(delim, last_pos);

		while (pos != std::basic_string_view<T, Traits>::npos || last_pos != std::basic_string_view<T, Traits>::npos)
		{
			tokens[src.substr(last_pos, pos - last_pos)]++;
			last_pos = src.find_first_not_of(delim, pos);
			pos = src.find_first_of(delim, last_pos);
		}
	}
}
