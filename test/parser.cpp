#include <gtest/gtest.h>

#include <lib/parser.hpp>
#include <tuple>

namespace utils
{
	template<class To, class From>
	auto convert(const From& from);

	template<class T>
	class rule
	{
	public:
		using value_type = T;
		using result_t = std::optional<value_type>;
	public:
		template<class Rule>
		rule(Rule&& rule)
		{}
		template<class Begin, class End>
		constexpr result_t operator()(Begin& it, End vend) const;
		template<std::size_t ISize>
		constexpr result_t operator()(const T(&value)[ISize]) const;
	};
}

struct Terminal
{

};

struct NonTerminal
{

};

struct Rule
{

};

TEST(parser, base)
{
	const auto& match = lib::match;
	EXPECT_TRUE(match("abc")("abc"));
	EXPECT_TRUE(!match("abc")("ab"));
	EXPECT_TRUE((match("abc") | match("cba"))("abc"));
	EXPECT_TRUE((match("abc") | match("cba"))("cba"));
	EXPECT_TRUE(!(match("abc") | match("cba"))("def"));
	EXPECT_TRUE((*match("a"))("aaa"));
	{
		constexpr auto regex = match("ab") & !match("c") & match("d");
		EXPECT_TRUE(regex("abd"));
		EXPECT_TRUE(regex("abcd"));
		EXPECT_TRUE(!regex("abed"));
		const auto result = regex("abcd");
		EXPECT_TRUE(regex(*result) == "abcd");
	}
	{
		constexpr auto regex = match("ab") & +match("c") & match("d");
		EXPECT_TRUE(!regex("abd"));
		EXPECT_TRUE(regex("abcd"));
		EXPECT_TRUE(regex("abccd"));
		EXPECT_TRUE(regex("abcccd"));
		EXPECT_TRUE(!regex("abce"));
	}
	{
		constexpr auto regex = match["abcdef"];
		EXPECT_TRUE(regex("a"));
		EXPECT_TRUE(regex("b"));
		EXPECT_TRUE(!regex("g"));
	}
	{
		constexpr auto symbol = match('a', 'z') | match('A', 'Z') | match("_");
		constexpr auto number = match["1234567890"];
		constexpr auto regex = symbol & *(symbol | number);
		EXPECT_TRUE(regex("some_value"));
		EXPECT_TRUE(regex("_"));
		EXPECT_TRUE(regex("LegalIdentifier"));
		EXPECT_TRUE(!regex("122_bytes"));
		EXPECT_TRUE(!regex(" value"));
		const auto result = regex("identifier");
		EXPECT_TRUE(regex(*result) == "identifier");
	}
}

TEST(parser, extended)
{
	const auto& match = lib::match;
	{
		constexpr auto symbol = match('a', 'z');
		constexpr auto number = match('0', '9');
		constexpr auto regex = symbol | number;
		const auto token = regex("1");
		EXPECT_TRUE(regex(*token, number) == "1");
		EXPECT_TRUE(regex(*token, number) != "0");
	}
	{
		constexpr auto symbol = match("_") | match('a', 'z') | match('A', 'Z');
		constexpr auto number = match["1234567890"];
		constexpr auto identifier = symbol & *(symbol | number);
		const auto token = identifier("some_value_0123");
		EXPECT_TRUE(identifier(*token, symbol) == "some_value_");
		EXPECT_TRUE(identifier(*token, number) == "0123");
	}
}

namespace {

	struct null_array_t {} constexpr null_array;
	struct null_element_t {} constexpr null_element;
	struct null_tuple_t {} constexpr null_tuple;

	template<std::size_t Index, class T, std::size_t Size>
	constexpr const auto& array_get(const std::array<T, Size>& array)
	{
		static_assert(Index < Size);
		return array[Index];
	}

	template<std::size_t Index, class T>
	constexpr const auto& array_get(const T& array)
	{
		static_assert(Index == 0);
		return array;
	}

	template<std::size_t Index>
	constexpr const auto& array_get(const null_array_t&)
	{
		return null_element;
	}

	template<class T, std::size_t Size>
	constexpr const auto& array_top(const std::array<T, Size>& array)
	{
		return array.back();
	}

	template<class T>
	constexpr const auto& array_top(const T& array)
	{
		return array;
	}

	constexpr const auto& array_top(const null_array_t&)
	{
		return null_element;
	}

	template<class T, std::size_t Size, std::size_t... I>
	constexpr std::array<T, Size + 1u> array_push(const std::array<T, Size>& array, const T& value, std::index_sequence<I...>)
	{
		return {array[I]..., value};
	}

	template<class T, std::size_t Size>
	constexpr std::array<T, Size + 1u> array_push(const std::array<T, Size>& array, const T& value)
	{
		return array_push(array, value, std::make_index_sequence<Size>());
	}

	template<class T, std::size_t SizeA, std::size_t... Ia, std::size_t SizeB, std::size_t... Ib>
	constexpr std::array<T, SizeA + SizeB> array_push(const std::array<T, SizeA>& array, std::index_sequence<Ia...>, const std::array<T, SizeB>& value, std::index_sequence<Ib...>)
	{
		return {array[Ia]..., value[Ib]...};
	}

	template<class T, std::size_t SizeA, std::size_t SizeB>
	constexpr auto array_push(const std::array<T, SizeA>& array, const std::array<T, SizeB>& value)
	{
		return array_push(array, std::make_index_sequence<SizeA>(), value, std::make_index_sequence<SizeA>());
	}


	template<class T, std::size_t Size>
	constexpr const auto& array_push(const std::array<T, Size>& array, const null_element_t&)
	{
		return array;
	}

	template<class T, std::size_t Size>
	constexpr const auto& array_push(const std::array<T, Size>& array, const null_array_t&)
	{
		return array;
	}

	template<class T>
	constexpr std::array<T, 2u> array_push(const T& a, const T& b)
	{
		return {a, b};
	}

	template<class T>
	constexpr const auto& array_push(const null_array_t&, const T& value)
	{
		return value;
	}

	constexpr const auto& array_push(const null_array_t& array, const null_element_t&)
	{
		return array;
	}

	template<class ...TArgs>
	constexpr auto tuple_make(TArgs&& ...args)
	{
		return std::make_tuple(std::forward<TArgs>(args)...);
	}

	constexpr auto tuple_make()
	{
		return null_tuple;
	}

	template<class A, class B>
	constexpr auto tuple_cat(const A& a, const B& b)
	{
		return std::tuple_cat(a, b);
	}

	template<class T>
	constexpr const auto& tuple_cat(const T& tuple, const null_tuple_t&)
	{
		return tuple;
	}

	template<class T>
	constexpr const auto& tuple_cat(const null_tuple_t&, const T& tuple)
	{
		return tuple;
	}

	template<class Head, class ...Tail>
	constexpr auto& tuple_head(const std::tuple<Head, Tail...>& tuple)
	{
		return std::get<0>(tuple);
	}

	constexpr auto tuple_head(const null_tuple_t& null)
	{
		return null;
	}

	template<class Head, class ...Tail>
	constexpr auto tuple_tail(const std::tuple<Head, Tail...>& tuple)
	{
		return tuple_make(std::get<Tail>(tuple)...);
	}

	template<class Head>
	constexpr auto tuple_tail(const std::tuple<Head>&)
	{
		return null_tuple;
	}

	template<class T, class V>
	struct is_array_push_value
	{
		constexpr static bool value = false;
	};

	template<class T>
	struct is_array_push_value<T, T>
	{
		constexpr static bool value = true;
	};

	template<class T>
	struct is_array_push_value<const T&, T>
	{
		constexpr static bool value = true;
	};

	template<class T, std::size_t Size>
	struct is_array_push_value<const std::array<T, Size>&, T>
	{
		constexpr static bool value = true;
	};

	template<class T, class V>
	inline constexpr bool is_array_push_value_v = is_array_push_value<T, V>::value;

	template<class T, class ...Ts>
	constexpr auto tuple_push(const std::tuple<Ts...>& tuple, const T& value)
	{
		const auto& head = tuple_head(tuple);
		if constexpr(is_array_push_value_v<decltype(head), T>)
			return tuple_cat(tuple_make(array_push(head, value)), tuple_tail(tuple));
		else
			return tuple_cat(tuple_make(head), tuple_push(tuple_tail(tuple), value));
	}

	template<class T>
	constexpr auto tuple_push(const null_tuple_t&, const T& value)
	{
		return tuple_make(value);
	}

	template<class ...Ta, class ...Tb>
	constexpr auto tuple_push(const std::tuple<Ta...>& tuple, const std::tuple<Tb...>& value)
	{
		return tuple_push(tuple_push(tuple, tuple_head(value)), tuple_tail(value));
	}

	template<class T, class ...Ts>
	constexpr auto& tuple_top(const std::tuple<Ts...>& tuple)
	{
		const auto& head = tuple_head(tuple);
		if constexpr(is_array_push_value_v<decltype(head), T>)
			return array_top(head);
		else
			return tuple_top<T>(tuple_tail(tuple));
	}

	template<class T, std::size_t Index, class ...Ts>
	constexpr auto& tuple_get(const std::tuple<Ts...>& tuple)
	{
		const auto& head = tuple_head(tuple);
		if constexpr(is_array_push_value_v<decltype(head), T>)
			return array_get<Index>(head);
		else
			return tuple_get<T, Index>(tuple_tail(tuple));
	}
}

TEST(parser, extended2)
{
	const auto& match = lib::match;
	constexpr auto symbol = match('a', 'z') | match('A', 'Z') | match["_"];
	constexpr auto number = match('0', '9');
	constexpr auto anysymbol = (match('a', 'z') | match('A', 'Z')) | (match["_"] | match('0', '9'));
	symbol & *(symbol | number);

	constexpr auto t1 = tuple_push(null_tuple, 1);
	constexpr auto t2 = tuple_push(t1, 2);
	constexpr auto t3 = tuple_push(t2, 3u);
	constexpr auto t4 = tuple_push(t3, 4u);
	constexpr auto t5 = tuple_push(t4, 5);

	constexpr auto a1 = array_push(null_array, 1);
	constexpr auto a2 = array_push(a1, 2);
	constexpr auto a3 = array_push(a2, a1);
	constexpr auto a4 = array_push(a3, null_array);
	constexpr auto a5 = array_push(a3, null_element);

	constexpr auto e1 = array_top(null_array);
	constexpr auto e2 = array_top(a1);
	constexpr auto e3 = array_top(a2);
	constexpr auto e4 = array_get<2>(a5);
	constexpr auto e5 = array_get<0>(e1);

	constexpr auto e6 = tuple_top<int>(t4);
	constexpr auto e7 = tuple_get<int, 0>(t4);

	//const auto& token = utils::token;
	//auto identifier = token(symbol & *(symbol | number), utils::tag<std::string>);
	/*auto lexer = utils::tokenize(
		utils::tokens(
			identifier >> [](const char* b, const char* e) {
				return std::string("identifier{\"") + std::string(b, e) + "\"}";
			},
			+number >> [](const char* b, const char* e) {
				return std::string("number{" + std::string(b, e) + "}");
			}
		),
		utils::channel::input("")
	);*/
	//std::cout << tokenize(lexer)("abcd") << std::endl;
	//std::cout << tokenize(lexer)("1234") << std::endl;

	/*const auto& match = utils::match;
	{
		auto rule = match['('] >> token[identifier] >> match[')']; // -> std::string
		std::string sequence = "(abcdef)";
		auto value = parse(rule, sequence); // -> string("abcdef")
		generate(rule, value); // -> sequence "(abcdef)"
	}
	{
		auto rule_k = number >> match['K'];
		auto rule_m = number >> match['M'];
		auto rule_g = number >> match['G'];
		auto rule = rule_k | rule_m | rule_g | number;
	}*/

}

/*template<>
struct converter<unsigned>
{
	unsigned load(const number_range& range)
	{
		switch(range.tag)
		{
			case 'K': return boost::lexical_cast<unsigned>(range) * 1000;
			case 'M': return boost::lexical_cast<unsigned>(range) * 1000 * 1000;
			case 'G': return boost::lexical_cast<unsigned>(range) * 1000 * 1000 * 1000;
		}
	}
	number_range save(unsigned value)
	{
		if(value > 1000)
			return number_range(boost::lexical_cast<std::string>(value / 1000f), 'K');
		if(value > 1000'000)
			return number_range(boost::lexical_cast<std::string>(value / 1000'000f), 'M');
		if(value > 1000'000'000)
			return number_range(boost::lexical_cast<std::string>(value / 1000'000'000f), 'G');
	}
};*/