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

extern "C" {
	TSLanguage* tree_sitter_c();
}
#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()


enum ts_symbol_identifiers : uint16_t {
	sym_identifier = 1,
	aux_sym_preproc_include_token1 = 2,
	aux_sym_preproc_include_token2 = 3,
	aux_sym_preproc_def_token1 = 4,
	anon_sym_LPAREN = 5,
	anon_sym_DOT_DOT_DOT = 6,
	anon_sym_COMMA = 7,
	anon_sym_RPAREN = 8,
	aux_sym_preproc_if_token1 = 9,
	anon_sym_LF = 10,
	aux_sym_preproc_if_token2 = 11,
	aux_sym_preproc_ifdef_token1 = 12,
	aux_sym_preproc_ifdef_token2 = 13,
	aux_sym_preproc_else_token1 = 14,
	aux_sym_preproc_elif_token1 = 15,
	aux_sym_preproc_elifdef_token1 = 16,
	aux_sym_preproc_elifdef_token2 = 17,
	sym_preproc_arg = 18,
	sym_preproc_directive = 19,
	anon_sym_LPAREN2 = 20,
	anon_sym_defined = 21,
	anon_sym_BANG = 22,
	anon_sym_TILDE = 23,
	anon_sym_DASH = 24,
	anon_sym_PLUS = 25,
	anon_sym_STAR = 26,
	anon_sym_SLASH = 27,
	anon_sym_PERCENT = 28,
	anon_sym_PIPE_PIPE = 29,
	anon_sym_AMP_AMP = 30,
	anon_sym_PIPE = 31,
	anon_sym_CARET = 32,
	anon_sym_AMP = 33,
	anon_sym_EQ_EQ = 34,
	anon_sym_BANG_EQ = 35,
	anon_sym_GT = 36,
	anon_sym_GT_EQ = 37,
	anon_sym_LT_EQ = 38,
	anon_sym_LT = 39,
	anon_sym_LT_LT = 40,
	anon_sym_GT_GT = 41,
	anon_sym_SEMI = 42,
	anon_sym___extension__ = 43,
	anon_sym_typedef = 44,
	anon_sym_extern = 45,
	anon_sym___attribute__ = 46,
	anon_sym___attribute = 47,
	anon_sym_COLON_COLON = 48,
	anon_sym_LBRACK_LBRACK = 49,
	anon_sym_RBRACK_RBRACK = 50,
	anon_sym___declspec = 51,
	anon_sym___based = 52,
	anon_sym___cdecl = 53,
	anon_sym___clrcall = 54,
	anon_sym___stdcall = 55,
	anon_sym___fastcall = 56,
	anon_sym___thiscall = 57,
	anon_sym___vectorcall = 58,
	sym_ms_restrict_modifier = 59,
	sym_ms_unsigned_ptr_modifier = 60,
	sym_ms_signed_ptr_modifier = 61,
	anon_sym__unaligned = 62,
	anon_sym___unaligned = 63,
	anon_sym_LBRACE = 64,
	anon_sym_RBRACE = 65,
	anon_sym_signed = 66,
	anon_sym_unsigned = 67,
	anon_sym_long = 68,
	anon_sym_short = 69,
	anon_sym_LBRACK = 70,
	anon_sym_static = 71,
	anon_sym_RBRACK = 72,
	anon_sym_EQ = 73,
	anon_sym_auto = 74,
	anon_sym_register = 75,
	anon_sym_inline = 76,
	anon_sym___inline = 77,
	anon_sym___inline__ = 78,
	anon_sym___forceinline = 79,
	anon_sym_thread_local = 80,
	anon_sym___thread = 81,
	anon_sym_const = 82,
	anon_sym_constexpr = 83,
	anon_sym_volatile = 84,
	anon_sym_restrict = 85,
	anon_sym___restrict__ = 86,
	anon_sym__Atomic = 87,
	anon_sym__Noreturn = 88,
	anon_sym_noreturn = 89,
	anon_sym__Nonnull = 90,
	anon_sym_alignas = 91,
	anon_sym__Alignas = 92,
	sym_primitive_type = 93,
	anon_sym_enum = 94,
	anon_sym_COLON = 95,
	anon_sym_struct = 96,
	anon_sym_union = 97,
	anon_sym_if = 98,
	anon_sym_else = 99,
	anon_sym_switch = 100,
	anon_sym_case = 101,
	anon_sym_default = 102,
	anon_sym_while = 103,
	anon_sym_do = 104,
	anon_sym_for = 105,
	anon_sym_return = 106,
	anon_sym_break = 107,
	anon_sym_continue = 108,
	anon_sym_goto = 109,
	anon_sym___try = 110,
	anon_sym___except = 111,
	anon_sym___finally = 112,
	anon_sym___leave = 113,
	anon_sym_QMARK = 114,
	anon_sym_STAR_EQ = 115,
	anon_sym_SLASH_EQ = 116,
	anon_sym_PERCENT_EQ = 117,
	anon_sym_PLUS_EQ = 118,
	anon_sym_DASH_EQ = 119,
	anon_sym_LT_LT_EQ = 120,
	anon_sym_GT_GT_EQ = 121,
	anon_sym_AMP_EQ = 122,
	anon_sym_CARET_EQ = 123,
	anon_sym_PIPE_EQ = 124,
	anon_sym_DASH_DASH = 125,
	anon_sym_PLUS_PLUS = 126,
	anon_sym_sizeof = 127,
	anon_sym___alignof__ = 128,
	anon_sym___alignof = 129,
	anon_sym__alignof = 130,
	anon_sym_alignof = 131,
	anon_sym__Alignof = 132,
	anon_sym_offsetof = 133,
	anon_sym__Generic = 134,
	anon_sym_asm = 135,
	anon_sym___asm__ = 136,
	anon_sym___asm = 137,
	anon_sym___volatile__ = 138,
	anon_sym_DOT = 139,
	anon_sym_DASH_GT = 140,
	sym_number_literal = 141,
	anon_sym_L_SQUOTE = 142,
	anon_sym_u_SQUOTE = 143,
	anon_sym_U_SQUOTE = 144,
	anon_sym_u8_SQUOTE = 145,
	anon_sym_SQUOTE = 146,
	aux_sym_char_literal_token1 = 147,
	anon_sym_L_DQUOTE = 148,
	anon_sym_u_DQUOTE = 149,
	anon_sym_U_DQUOTE = 150,
	anon_sym_u8_DQUOTE = 151,
	anon_sym_DQUOTE = 152,
	aux_sym_string_literal_token1 = 153,
	sym_escape_sequence = 154,
	sym_system_lib_string = 155,
	sym_true = 156,
	sym_false = 157,
	anon_sym_NULL = 158,
	anon_sym_nullptr = 159,
	sym_comment = 160,
	sym_translation_unit = 161,
	sym__top_level_item = 162,
	sym__block_item = 163,
	sym_preproc_include = 164,
	sym_preproc_def = 165,
	sym_preproc_function_def = 166,
	sym_preproc_params = 167,
	sym_preproc_call = 168,
	sym_preproc_if = 169,
	sym_preproc_ifdef = 170,
	sym_preproc_else = 171,
	sym_preproc_elif = 172,
	sym_preproc_elifdef = 173,
	sym_preproc_if_in_field_declaration_list = 174,
	sym_preproc_ifdef_in_field_declaration_list = 175,
	sym_preproc_else_in_field_declaration_list = 176,
	sym_preproc_elif_in_field_declaration_list = 177,
	sym_preproc_elifdef_in_field_declaration_list = 178,
	sym_preproc_if_in_enumerator_list = 179,
	sym_preproc_ifdef_in_enumerator_list = 180,
	sym_preproc_else_in_enumerator_list = 181,
	sym_preproc_elif_in_enumerator_list = 182,
	sym_preproc_elifdef_in_enumerator_list = 183,
	sym_preproc_if_in_enumerator_list_no_comma = 184,
	sym_preproc_ifdef_in_enumerator_list_no_comma = 185,
	sym_preproc_else_in_enumerator_list_no_comma = 186,
	sym_preproc_elif_in_enumerator_list_no_comma = 187,
	sym_preproc_elifdef_in_enumerator_list_no_comma = 188,
	sym__preproc_expression = 189,
	sym_preproc_parenthesized_expression = 190,
	sym_preproc_defined = 191,
	sym_preproc_unary_expression = 192,
	sym_preproc_call_expression = 193,
	sym_preproc_argument_list = 194,
	sym_preproc_binary_expression = 195,
	sym_function_definition = 196,
	sym__old_style_function_definition = 197,
	sym_declaration = 198,
	sym_type_definition = 199,
	sym__type_definition_type = 200,
	sym__type_definition_declarators = 201,
	sym__declaration_modifiers = 202,
	sym__declaration_specifiers = 203,
	sym_linkage_specification = 204,
	sym_attribute_specifier = 205,
	sym_attribute = 206,
	sym_attribute_declaration = 207,
	sym_ms_declspec_modifier = 208,
	sym_ms_based_modifier = 209,
	sym_ms_call_modifier = 210,
	sym_ms_unaligned_ptr_modifier = 211,
	sym_ms_pointer_modifier = 212,
	sym_declaration_list = 213,
	sym__declarator = 214,
	sym__declaration_declarator = 215,
	sym__field_declarator = 216,
	sym__type_declarator = 217,
	sym__abstract_declarator = 218,
	sym_parenthesized_declarator = 219,
	sym_parenthesized_field_declarator = 220,
	sym_parenthesized_type_declarator = 221,
	sym_abstract_parenthesized_declarator = 222,
	sym_attributed_declarator = 223,
	sym_attributed_field_declarator = 224,
	sym_attributed_type_declarator = 225,
	sym_pointer_declarator = 226,
	sym_pointer_field_declarator = 227,
	sym_pointer_type_declarator = 228,
	sym_abstract_pointer_declarator = 229,
	sym_function_declarator = 230,
	sym__function_declaration_declarator = 231,
	sym_function_field_declarator = 232,
	sym_function_type_declarator = 233,
	sym_abstract_function_declarator = 234,
	sym__old_style_function_declarator = 235,
	sym_array_declarator = 236,
	sym_array_field_declarator = 237,
	sym_array_type_declarator = 238,
	sym_abstract_array_declarator = 239,
	sym_init_declarator = 240,
	sym_compound_statement = 241,
	sym_storage_class_specifier = 242,
	sym_type_qualifier = 243,
	sym_alignas_qualifier = 244,
	sym_type_specifier = 245,
	sym_sized_type_specifier = 246,
	sym_enum_specifier = 247,
	sym_enumerator_list = 248,
	sym_struct_specifier = 249,
	sym_union_specifier = 250,
	sym_field_declaration_list = 251,
	sym__field_declaration_list_item = 252,
	sym_field_declaration = 253,
	sym__field_declaration_declarator = 254,
	sym_bitfield_clause = 255,
	sym_enumerator = 256,
	sym_variadic_parameter = 257,
	sym_parameter_list = 258,
	sym__old_style_parameter_list = 259,
	sym_parameter_declaration = 260,
	sym_attributed_statement = 261,
	sym_statement = 262,
	sym__top_level_statement = 263,
	sym_labeled_statement = 264,
	sym__top_level_expression_statement = 265,
	sym_expression_statement = 266,
	sym_if_statement = 267,
	sym_else_clause = 268,
	sym_switch_statement = 269,
	sym_case_statement = 270,
	sym_while_statement = 271,
	sym_do_statement = 272,
	sym_for_statement = 273,
	sym__for_statement_body = 274,
	sym_return_statement = 275,
	sym_break_statement = 276,
	sym_continue_statement = 277,
	sym_goto_statement = 278,
	sym_seh_try_statement = 279,
	sym_seh_except_clause = 280,
	sym_seh_finally_clause = 281,
	sym_seh_leave_statement = 282,
	sym_expression = 283,
	sym__string = 284,
	sym_comma_expression = 285,
	sym_conditional_expression = 286,
	sym_assignment_expression = 287,
	sym_pointer_expression = 288,
	sym_unary_expression = 289,
	sym_binary_expression = 290,
	sym_update_expression = 291,
	sym_cast_expression = 292,
	sym_type_descriptor = 293,
	sym_sizeof_expression = 294,
	sym_alignof_expression = 295,
	sym_offsetof_expression = 296,
	sym_generic_expression = 297,
	sym_subscript_expression = 298,
	sym_call_expression = 299,
	sym_gnu_asm_expression = 300,
	sym_gnu_asm_qualifier = 301,
	sym_gnu_asm_output_operand_list = 302,
	sym_gnu_asm_output_operand = 303,
	sym_gnu_asm_input_operand_list = 304,
	sym_gnu_asm_input_operand = 305,
	sym_gnu_asm_clobber_list = 306,
	sym_gnu_asm_goto_list = 307,
	sym_extension_expression = 308,
	sym_argument_list = 309,
	sym_field_expression = 310,
	sym_compound_literal_expression = 311,
	sym_parenthesized_expression = 312,
	sym_initializer_list = 313,
	sym_initializer_pair = 314,
	sym_subscript_designator = 315,
	sym_subscript_range_designator = 316,
	sym_field_designator = 317,
	sym_char_literal = 318,
	sym_concatenated_string = 319,
	sym_string_literal = 320,
	sym_null = 321,
	sym__empty_declaration = 322,
	sym_macro_type_specifier = 323,
	aux_sym_translation_unit_repeat1 = 324,
	aux_sym_preproc_params_repeat1 = 325,
	aux_sym_preproc_if_repeat1 = 326,
	aux_sym_preproc_if_in_field_declaration_list_repeat1 = 327,
	aux_sym_preproc_if_in_enumerator_list_repeat1 = 328,
	aux_sym_preproc_if_in_enumerator_list_no_comma_repeat1 = 329,
	aux_sym_preproc_argument_list_repeat1 = 330,
	aux_sym__old_style_function_definition_repeat1 = 331,
	aux_sym_declaration_repeat1 = 332,
	aux_sym_type_definition_repeat1 = 333,
	aux_sym__type_definition_type_repeat1 = 334,
	aux_sym__type_definition_declarators_repeat1 = 335,
	aux_sym__declaration_specifiers_repeat1 = 336,
	aux_sym_attribute_declaration_repeat1 = 337,
	aux_sym_attributed_declarator_repeat1 = 338,
	aux_sym_pointer_declarator_repeat1 = 339,
	aux_sym_function_declarator_repeat1 = 340,
	aux_sym_array_declarator_repeat1 = 341,
	aux_sym_sized_type_specifier_repeat1 = 342,
	aux_sym_enumerator_list_repeat1 = 343,
	aux_sym__field_declaration_declarator_repeat1 = 344,
	aux_sym_parameter_list_repeat1 = 345,
	aux_sym__old_style_parameter_list_repeat1 = 346,
	aux_sym_case_statement_repeat1 = 347,
	aux_sym_generic_expression_repeat1 = 348,
	aux_sym_gnu_asm_expression_repeat1 = 349,
	aux_sym_gnu_asm_output_operand_list_repeat1 = 350,
	aux_sym_gnu_asm_input_operand_list_repeat1 = 351,
	aux_sym_gnu_asm_clobber_list_repeat1 = 352,
	aux_sym_gnu_asm_goto_list_repeat1 = 353,
	aux_sym_argument_list_repeat1 = 354,
	aux_sym_initializer_list_repeat1 = 355,
	aux_sym_initializer_pair_repeat1 = 356,
	aux_sym_char_literal_repeat1 = 357,
	aux_sym_concatenated_string_repeat1 = 358,
	aux_sym_string_literal_repeat1 = 359,
	alias_sym_field_identifier = 360,
	alias_sym_statement_identifier = 361,
	alias_sym_type_identifier = 362,
};

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
			std::cerr << "Encounter untested type, treating it as one-line expression/statement: " << node.getType() << std::endl;
			[[fallthrough]];
		case ts_symbol_identifiers::sym_return_statement:
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
				auto tmp = topLevelExpr.getNamedChild(1).getNamedChild(0).getByteRange();
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
			"SF:" << "filename" << "\\n";//allFiles[i].filename

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
