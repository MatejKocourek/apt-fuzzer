#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <iterator>
#include <string_view>
#include <cpp-tree-sitter.h>
#include <vector>
#include <variant>
#include <sstream>
#include <optional>
#include <filesystem>
#include <unordered_set>
#include "symbol-identifiers.h"

extern "C" {
	TSLanguage* tree_sitter_c();
}


class SeedGenerator {
public:
	std::unordered_set<std::string> constsStrings;
	std::unordered_set<std::string> constsChars;
	std::unordered_set<std::string> constsNumbersInts;
	std::unordered_set<std::string> constsNumbersOther;

	static std::string_view nodeString(const std::string& source, const ts::Node& node)
	{
		auto range = node.getByteRange();
		std::string_view functionName(&source[range.start], range.end - range.start);
		return functionName;
	}

	static char escapeChar(char c)
	{
		switch (c)
		{
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';
		case '0':
			return '\0';
		case '\\':
		case '\'':
		case '\"':
		case '\?':
			return c;

		default:
			std::cerr << "Unknown escape character: " << c <<'(' << (int)c << ')' << std::endl;
			return ' ';
		}
	}

	static std::string escapeString(const std::string_view& string)
	{
		auto noQuotes = string.substr(1, string.size() - 2);
		std::string res;
		res.reserve(noQuotes.size());

		for (size_t i = 0; i < noQuotes.size(); i++)
		{
			if (noQuotes[i] == '\\')
			{
				res += escapeChar(noQuotes.at(++i));
			}
			else [[likely]]
				res += noQuotes[i];
		}

		return res;
	}

	void recordNumber(const std::string_view& string)
	{
		for (const auto& c : string)
		{
			if (!std::isdigit(c) && c != '-')
			{
				// Number is a double or any other abomination
				constsNumbersOther.emplace(string);
				return;
			}
		}

		// Number is an integer
		constsNumbersInts.emplace(string);
	}

	void parseRecursive(const std::string& sourcecode, const ts::Node& node)
	{
		for (const auto& child : ts::Children(node))
		{
			auto type = static_cast<ts_symbol_identifiers>(child.getSymbol());

			switch (type)
			{
			case ts_symbol_identifiers::sym_string_literal:
				constsStrings.emplace(escapeString(nodeString(sourcecode, child)));
				break;
			case ts_symbol_identifiers::sym_char_literal:
				constsChars.emplace(escapeString(nodeString(sourcecode, child)));
				break;
			case ts_symbol_identifiers::sym_number_literal:
				recordNumber(nodeString(sourcecode, child));
				break;
			case ts_symbol_identifiers::sym_preproc_include:
				continue; // We don't want to record the include "code.h" files.
			default:
			{
				//if (child.getNumChildren() == 0)
				//	std::cerr << "Encounter untested type: " << nodeString(sourcecode, child) << std::endl;
				break;
			}

			}

			parseRecursive(sourcecode, child);
		}
	}

	static void writeStringOfSize(std::ostream& out, size_t size)
	{
		std::ostream_iterator<char> out_iter(out);
		std::fill_n(out_iter, size, 'a');
	}

public:
	void parseSource(const std::string& sourcecode)
	{
		// Create a language and parser.
		ts::Parser parser{ tree_sitter_c() };

		// Parse the provided string into a syntax tree.

		auto tree = parser.parseString(sourcecode);

		// Get the root node of the syntax tree. 

		parseRecursive(sourcecode, tree.getRootNode());
	}

	void createSeeds(const std::filesystem::path& path) const
	{
		std::unordered_set<std::string> strings(constsStrings);
		size_t fileCount = 0;

		// First, create seeds from all literals, no matter their type, as string
		strings.insert(constsChars.begin(), constsChars.end());
		strings.insert(constsNumbersInts.begin(), constsNumbersInts.end());
		strings.insert(constsNumbersOther.begin(), constsNumbersOther.end());

		//We don't want an empty string, delete it (if exists).
		strings.erase("");

		std::filesystem::create_directories(path);

		std::unordered_set<size_t> stringSizes;

		for (const auto& i : strings)
		{
			//std::cerr << fileCount << ":" << i.size() << std::endl;
			std::ofstream(path / (std::to_string(fileCount++) + ".txt")) << std::string_view(i);
			stringSizes.insert(i.size());
		}

		// Now create strings of size the same as loaded ints (with a bound)
		for (const auto& i : constsNumbersInts)
		{
			constexpr int64_t maxBound = 65536;

			int64_t num = std::stoll(i);

			// Only generate the string if in bounds and a string of same length does not already exist
			if (num > 0 && num <= maxBound && stringSizes.insert(num).second)
			{
				std::ofstream file(path / (std::to_string(fileCount++) + ".txt"));
				writeStringOfSize(file, num);
			}
		}

		std::cerr << "Created " << fileCount << " new seeds." << std::endl;
	}

};
