#pragma once
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
#include "symbol-identifiers.h"

extern "C" {
	TSLanguage* tree_sitter_c();
}
#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()



class FileInstrument {
public:
	FileInstrument(std::string sourcecode, std::string filename, int fileId) : sourcecode(std::move(sourcecode)), filename(std::move(filename)), fileId(std::move(fileId))
	{
		parseSource();
	}
//private:
	uint32_t lastInstrumentedLine = -1;

	void instrumentLine(uint32_t bytePos, uint32_t row)
	{
		if (row != lastInstrumentedLine) [[likely]]
		{
			instrumentations.emplace_back(bytePos, row + 1);
			lastInstrumentedLine = row;
		}
	}
	void instrumentRecursive(const ts::Node& node)
	{
		//std::cout << statement.getPointRange().start.row << ':' << statement.getPointRange().start.column << ' ' << statement.getType() << std::endl;
		auto type = static_cast<ts_symbol_identifiers>(node.getSymbol());

		// Instrument line itself
		switch (type)
		{
		default:
			std::cerr << "Encounter untested type, not instrumenting it: " << node.getType() << std::endl;
			return;
		case ts_symbol_identifiers::sym_return_statement:
		case ts_symbol_identifiers::sym_break_statement:
		case ts_symbol_identifiers::sym_continue_statement:
		case ts_symbol_identifiers::sym_if_statement:
		case ts_symbol_identifiers::sym_declaration:
		case ts_symbol_identifiers::sym_expression:
		case ts_symbol_identifiers::sym_for_statement:
		case ts_symbol_identifiers::sym_while_statement:
		case ts_symbol_identifiers::sym_expression_statement:
		{
			instrumentLine(node.getByteRange().start, node.getPointRange().start.row);
			break;
		}
		case ts_symbol_identifiers::sym_compound_statement:
		case ts_symbol_identifiers::anon_sym_LBRACE:
		case ts_symbol_identifiers::anon_sym_RBRACE:
		case ts_symbol_identifiers::sym_comment:
		case ts_symbol_identifiers::sym_switch_statement:
		case ts_symbol_identifiers::sym_case_statement:
			break;
		}


		// Instrument its children, if needed
		switch (type)
		{
		case ts_symbol_identifiers::sym_for_statement:
		case ts_symbol_identifiers::sym_while_statement:
		{
			assert(node.getNumChildren() > 0);

			instrumentPossibleOneLiner(node.getChild(node.getNumChildren() - 1));
			break;
		}
		case ts_symbol_identifiers::sym_if_statement:
		{
			if (node.getNumChildren() >= 3)
			{
				uint32_t child = 1;
				while (node.getChild(++child).getSymbol() == ts_symbol_identifiers::sym_comment);

				instrumentPossibleOneLiner(node.getChild(child));

				auto possibleElse = node.getChild(node.getNumChildren() - 1);

				if (possibleElse.getSymbol() == ts_symbol_identifiers::sym_else_clause)
					//instrumentChildren(possibleElse);
					instrumentPossibleOneLiner(possibleElse.getChild(1));
			}
			break;
		}
		case ts_symbol_identifiers::sym_compound_statement:
		{
			for (const auto& child : ts::Children(node))
				instrumentRecursive(child);
			break;
		}
		case ts_symbol_identifiers::sym_switch_statement:
		{
			instrumentRecursive(node.getChild(2));
			break;
		}
		case ts_symbol_identifiers::sym_case_statement:
		{
			uint32_t child = 0;
			while (node.getChild(child++).getSymbol() != ts_symbol_identifiers::anon_sym_COLON);

			for (size_t i = child; i < node.getNumChildren(); i++)
			{
				instrumentRecursive(node.getChild(i));
			}
			break;
		}
		//default:
		//{
		//	for (const auto& child : ts::Children(node))
		//		instrumentRecursive(child);
		//	break;
		//}
		}
	}

	void instrumentPossibleOneLiner(const ts::Node& statement)
	{
		auto type = static_cast<ts_symbol_identifiers>(statement.getSymbol());
		switch (type)
		{
		case ts_symbol_identifiers::sym_compound_statement: {
			instrumentRecursive(statement);
		} break;
		default: {
			// This truly is a one-liner. We need to put braces around it
			auto byteRange = statement.getByteRange();
			instrumentationsStr.emplace_back(byteRange.start, "{");
			instrumentRecursive(statement);
			instrumentationsStr.emplace_back(byteRange.end, "}");
		} break;
		case ts_symbol_identifiers::anon_sym_LBRACE:
		case ts_symbol_identifiers::anon_sym_RBRACE:
		case ts_symbol_identifiers::sym_comment:
			break;
		}
			
		//instrumentLine(byteRange.start, statement.getPointRange().start.row);
		//for (const auto& child : ts::Children(statement))
			//instrumentChildren(child);
	}

	//void instrumentCompoundStatement(const ts::Node& compoundStatement)
	//{
	//	assert(compoundStatement.getSymbol() == ts_symbol_identifiers::sym_compound_statement);



	//}

	std::optional<ts::Node> findChild(const ts::Node& node, ts::Symbol sym)
	{
		for (const auto& child : ts::Children(node))
		{
			if (child.getSymbol() == sym)
				return child;
			else
			{
				auto res = findChild(child, sym);
				if (res.has_value())
					return res;
			}
		}
		return {}; // no value
	}

	void parseSource()
	{
		// Create a language and parser.
		ts::Parser parser{ tree_sitter_c() };

		// Parse the provided string into a syntax tree.

		ts::Tree tree = parser.parseString(sourcecode);

		// Get the root node of the syntax tree. 

		for (const auto& topLevelExpr : ts::Children(tree.getRootNode()))
		{
			if (topLevelExpr.getSymbol() == ts_symbol_identifiers::sym_function_definition)
			{
				auto found = findChild(topLevelExpr, ts_symbol_identifiers::sym_identifier);
				if (!found.has_value())
					continue;

				auto tmp = found->getByteRange();
				std::string_view functionName(&sourcecode[tmp.start], tmp.end - tmp.start);
				if (functionName == "main") [[unlikely]]
					{
						for (const auto& child : ts::Children(topLevelExpr))
						{
							if (child.getSymbol() == ts_symbol_identifiers::sym_compound_statement)
							{
								for (const auto& child2 : ts::Children(child))
								{
									if (child2.getSymbol() == ts_symbol_identifiers::anon_sym_LBRACE)
									{
										instrumentationsStr.emplace_back(child2.getByteRange().end, "atexit(_GenerateLcov);");
										thisIsMainFile = true;
										goto found;
									}
								}
							}
						}
						// If the function reaches here, main was declared, but not defined
					}

					found:

					for (const auto& child : ts::Children(topLevelExpr))
					{
						if (child.getSymbol() == ts_symbol_identifiers::sym_compound_statement)
						{
							instrumentRecursive(child);
							break;
						}
					}
					//std::cout << std::endl;
			}
		}
	}
public:
	void instrument(std::ostream& os) const
	{
		size_t sourcePos = 0;
		size_t strPos = 0;

		for (size_t i = 0; i < instrumentations.size(); )
		{
			if (instrumentationsStr.size() - strPos > 0 && instrumentationsStr[strPos].first <= instrumentations[i].first)
			{
				os << std::string_view(sourcecode.begin() + sourcePos, sourcecode.begin() + instrumentationsStr[strPos].first);
				sourcePos = instrumentationsStr[strPos].first;

				os << instrumentationsStr[strPos++].second;
			}
			else
			{
				os << std::string_view(sourcecode.begin() + sourcePos, sourcecode.begin() + instrumentations[i].first);
				sourcePos = instrumentations[i].first;

				os << "++" << "_F" << fileId << '[' << i << ']' << ';';
				++i;
			}
		}

		if (instrumentationsStr.size() - strPos > 0)
		{
			os << std::string_view(sourcecode.begin() + sourcePos, sourcecode.begin() + instrumentationsStr[strPos].first);
			sourcePos = instrumentationsStr[strPos].first;

			os << instrumentationsStr[strPos++].second;
		}

		os << std::string_view(sourcecode.begin() + sourcePos, sourcecode.end());
	}

	const std::string sourcecode;
	const std::string filename;
	const int fileId;
	bool thisIsMainFile = false;

	std::vector<std::pair<uint32_t, uint32_t>> instrumentations;
	std::vector<std::pair<uint32_t, std::string>> instrumentationsStr;
};

void instrumentHeaderExtern(std::ostream& os, const FileInstrument& file)
{
	os << "extern unsigned long long " << "_F" << file.fileId << "[];\n";
}

void instrumentHeaderMain(std::ostream& os, const std::vector<FileInstrument>& allFiles)
{
	for (const auto& i : allFiles)
		os << "unsigned long long " << "_F" << i.fileId << "[" << i.instrumentations.size() << "];";

	os << '\n';

	os <<
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"void _GenerateLcov(){"
		"FILE *f = fopen(\"coverage.lcov\", \"w\");"
		;

	for (const auto& i : allFiles)
	{
		os <<
			"unsigned long long LH" << i.fileId << "=0;"
			"for(unsigned long long i=0;i<" << i.instrumentations.size() << ";++i)"
				"if(" << "_F" << i.fileId << "[i]" ">" "0" ")"
					"++LH" << i.fileId << ";"
			;

	}

	os <<
		"fprintf(f,\""
		"TN:test\\n"
		;

	for (const auto& i : allFiles)
	{
		os <<
			"SF:" << i.filename << "\\n";

		for (const auto& j : i.instrumentations)
			os << "DA:" << j.second << ",%llu\\n";

		os <<
			"LH:%llu\\n"
			"LF:" << i.instrumentations.size() << "\\n"
			<< "end_of_record" "\\n"
			;
	}

	os << "\"";

	for (const auto& i : allFiles)
	{
		os << ",";
		for (size_t j = 0; j < i.instrumentations.size(); j++)
		{
			os <<
				"_F" << i.fileId << "[" << j << "]" ",";
		}
		os << "LH" << i.fileId;
	}

	os << ");}\n";
}
