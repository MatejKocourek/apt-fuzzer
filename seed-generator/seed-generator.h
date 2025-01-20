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

	// Captured consts that are of string type
	std::unordered_set<std::string> constsStrings;

	// Captured consts that are of char type
	std::unordered_set<std::string> constsChars;

	// Captured consts that are of int type
	std::unordered_set<std::string> constsInts;

	// Captured consts that are any other type
	std::unordered_set<std::string> constsOthers;

	/// <summary>
	/// Get string_view representation of the code of the underlaying node
	/// </summary>
	/// <param name="source">Source code to where the result will point</param>
	/// <param name="node">Node to show</param>
	/// <returns>string_view from source string</returns>
	static std::string_view nodeString(const std::string& source, const ts::Node& node)
	{
		auto range = node.getByteRange();
		std::string_view functionName(&source[range.start], range.end - range.start);
		return functionName;
	}

	/// <summary>
	/// Un-escape char in C compiler fashion
	/// </summary>
	/// <param name="c">Character following a backlash to un-escape</param>
	/// <returns>Original character</returns>
	static char unEscapeChar(char c)
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

	/// <summary>
	/// Un-escape string in C compiler fashion
	/// </summary>
	/// <param name="string">String that should be unescaped</param>
	/// <returns>New string that is unescaped</returns>
	static std::string unEscapeString(const std::string_view& string)
	{
		auto noQuotes = string.substr(1, string.size() - 2);
		std::string res;
		res.reserve(noQuotes.size());

		for (size_t i = 0; i < noQuotes.size(); i++)
		{
			if (noQuotes[i] == '\\')
			{
				++i;
				if (noQuotes.at(i) != '\n')
					res += unEscapeChar(noQuotes.at(i));
			}
			else [[likely]]
				res += noQuotes[i];
		}

		return res;
	}

	/// <summary>
	/// Process numeric constant and decide if it is 10-base integer or something else
	/// </summary>
	/// <param name="string"></param>
	void recordNumber(const std::string_view& string)
	{
		for (const auto& c : string)
		{
			if (!std::isdigit(c) && c != '-')
			{
				// Number is a double or any other abomination
				constsStrings.emplace(string);
				return;
			}
		}

		// Number is an integer
		constsInts.emplace(string);
	}

	/// <summary>
	/// Parse a node and all its children and look for constants to process
	/// </summary>
	/// <param name="sourcecode">Sourcecode that tree-sitter was run on</param>
	/// <param name="node">Node to parse</param>
	void parseRecursive(const std::string& sourcecode, const ts::Node& node)
	{
		for (const auto& child : ts::Children(node))
		{
			auto type = static_cast<ts_symbol_identifiers>(child.getSymbol());

			switch (type)
			{
			case ts_symbol_identifiers::sym_string_literal:
				constsStrings.emplace(unEscapeString(nodeString(sourcecode, child)));
				break;
			case ts_symbol_identifiers::sym_char_literal:
				constsChars.emplace(unEscapeString(nodeString(sourcecode, child)));
				break;
			case ts_symbol_identifiers::sym_number_literal:
				recordNumber(nodeString(sourcecode, child));
				break;
			case ts_symbol_identifiers::sym_preproc_include:
				continue; // We don't want to record the include "code.h" files.
			case ts_symbol_identifiers::sym_preproc_arg:
				constsOthers.emplace(nodeString(sourcecode, child));
				break;
			default:
			{
				//if (child.getNumChildren() == 0)
					//std::cerr << "Encounter untested type: " << child.getType() << ':' << nodeString(sourcecode, child) << std::endl;
				break;
			}

			}

			parseRecursive(sourcecode, child);
		}
	}

	/// <summary>
	/// Write a string filled with letter 'a's in certain amount of times
	/// </summary>
	/// <param name="out">Stream to write to</param>
	/// <param name="size">Size of the string to create</param>
	static void writeStringOfSize(std::ostream& out, size_t size)
	{
		std::ostream_iterator<char> out_iter(out);
		std::fill_n(out_iter, size, 'a');
	}

public:
	/// <summary>
	/// Parse given sourcecode of C file, and find and save all constants
	/// </summary>
	/// <param name="sourcecode">Full sourcecode of a given file</param>
	void parseSource(const std::string& sourcecode)
	{
		// Create a language and parser.
		ts::Parser parser{ tree_sitter_c() };

		// Parse the provided string into a syntax tree.
		auto tree = parser.parseString(sourcecode);

		// Get the root node of the syntax tree. 
		parseRecursive(sourcecode, tree.getRootNode());

		// Sort out constsOthers
		for (const auto& i : constsOthers)
		{
			if (i.starts_with('\"') && i.ends_with('\"')) // This is probably a string
				constsStrings.insert(unEscapeString(i));
			if (i.starts_with('\'') && i.ends_with('\'')) // This is probably a char
				constsChars.insert(unEscapeString(i));
			else if (std::isdigit(i.at(0))) // This begins as integer. Take the integer value
			{
				size_t pos;
				int64_t num = std::stoll(i, &pos);
				constsInts.insert(std::to_string(num));

				if(pos != i.size() && i[pos] == '.') // This is a float
					constsStrings.insert(std::to_string(num)); // Treat floats as strings
			}
		}
	}

	/// <summary>
	/// Create seeds from all saved constants. Will create a file for each constant, numbered from 0.txt up
	/// </summary>
	/// <param name="path">Path to the folder where to create</param>
	void createSeeds(const std::filesystem::path& path) const
	{
		std::unordered_set<std::string> strings(constsStrings);
		size_t fileCount = 0;

		std::cerr << "Found constants:" << '\n';
		std::cerr << "STRINGS: " << constsStrings.size() << '\n';
		std::cerr << "CHARS: " << constsChars.size() << '\n';
		std::cerr << "INTEGERS: " << constsInts.size() << '\n';
		std::cerr << std::endl;

		// First, create seeds from all literals, no matter their type, as string
		strings.insert(constsChars.begin(), constsChars.end());
		strings.insert(constsInts.begin(), constsInts.end());
		strings.insert(constsOthers.begin(), constsOthers.end());

		//We don't want an empty string, delete it (if exists).
		strings.erase("");

		std::filesystem::create_directories(path);

		std::unordered_set<size_t> stringSizes;

		// Output all strings, nothing to do with those
		for (const auto& i : strings)
		{
			std::ofstream(path / (std::to_string(fileCount++) + ".txt")) << std::string_view(i);
			stringSizes.insert(i.size());
		}

		// Now create strings of size the same as loaded ints (with a bound)
		for (const auto& i : constsInts)
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
