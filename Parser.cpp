/*
 * Parser.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include "Parser.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "ParserError.h"

Parser::Parser(const std::string& input_file,
				const std::string& output_file,
				const std::string& mapping_file) :
	input_file_{input_file},
	output_file_{output_file},
	mapping_file_{mapping_file},
	line_number_{1},
	col_number_{0} {

	if (!input_file_.is_open()) {
		throw std::runtime_error("Could not open input file");
	}

	if (!output_file_.is_open()) {
		throw std::runtime_error("Could not open output file");
	}

	if (!mapping_file_.is_open()) {
		throw std::runtime_error("Could not open mapping file");
	}
}

char Parser::GetToken() {
	char character;
	if (!input_file_.get(character)) {
		throw ParserError("Missing token", line_number_, col_number_);
	}
	return character;
}

void Parser::ParseObject() {
	char character = GetToken();
	if (character == '{') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	ParseSpace();

	character = GetToken();
	if (character == '}') {
		output_file_ << character;
		++col_number_;
		return;
	} else if (character == '"') {
		input_file_.putback(character);

		ParsePair();
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	ParseSpace();

	character = GetToken();
	while (character == ',') {
		output_file_ << character;
		++col_number_;

		ParseSpace();
		ParsePair();
		ParseSpace();

		character = GetToken();
	}

	if (character == '}') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}
}

void Parser::ParsePair() {
	ParseString();
	ParseSpace();

	char character = GetToken();
	if (character == ':') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	ParseSpace();
	ParseValue();
}

void Parser::ParseSpace() {
	char character;
	while (input_file_.get(character)) {
		if (character == '\n') {
			++line_number_;
			col_number_ = 1;
			output_file_ << character;
		} else if (isspace(character)) {
			output_file_ << character;
			++col_number_;
		} else {
			input_file_.putback(character);
			break;
		}
	}
}

void Parser::ParseArray() {
	char character = GetToken();
	if (character == '[') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	ParseSpace();

	character = GetToken();
	if (character == ']') {
		output_file_ << character;
		++col_number_;
		return;
	}

	input_file_.putback(character);

	ParseValue();
	ParseSpace();

	character = GetToken();
	while (character == ',') {
		output_file_ << character;
		++col_number_;

		ParseSpace();
		ParseValue();
		ParseSpace();

		character = GetToken();
	}

	if (character == ']') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}
}

void Parser::ParseConst() {
	char character = GetToken();
	int next_chars;
	std::string compare_string;
	if (character == 't') {
		next_chars = 3;
		compare_string = "true";
	} else if (character == 'f') {
		next_chars = 4;
		compare_string = "false";
	} else if (character == 'n') {
		next_chars = 3;
		compare_string = "null";
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	std::stringstream identifier;
	identifier << character;
	while (next_chars > 0 && input_file_.get(character)) {
		++col_number_;
		identifier << character;
		--next_chars;
	}

	if (next_chars > 0 || identifier.str() != compare_string) {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	output_file_ << identifier.str();
}

void Parser::ParseNumber() {
	char character = GetToken();
	if (isdigit(character) || character == '-') {
		input_file_.putback(character);
		auto stream_position_start = input_file_.tellg();

		double digit;
		input_file_ >> digit;
		if (input_file_.fail()) {
			throw ParserError("Invalid number", line_number_, col_number_);
		}

		// We have move past the digit now, and if we streamed
		// just read digit out, it would get incorrectly formatted
		// thus re-stream original characters
		auto stream_position_end = input_file_.tellg();
		input_file_.seekg(stream_position_start);
		while (input_file_.tellg() != stream_position_end) {
			++col_number_;
			input_file_.get(character);
			output_file_ << character;
		}
		--col_number_;

	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}
}

void Parser::ParseValue() {
	char character = GetToken();
	switch (character) {
	case '{': {
		input_file_.putback(character);
		ParseObject();
		break;
	}

	case '[': {
		input_file_.putback(character);
		ParseArray();
		break;
	}

	case '"': {
		input_file_.putback(character);
		ParseString();
		break;
	}

	case 't': // FALL-THROUGH
	case 'f': // FALL-THROUGH
	case 'n': {
		input_file_.putback(character);
		ParseConst();
		break;
	}

	default:
		input_file_.putback(character);
		ParseNumber();
		break;
	}
}

void Parser::ParseString() {
	char character = GetToken();
	if (character == '"') {
		output_file_ << character;
		++col_number_;
	} else {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}

	bool escape_seq{false};
	std::stringstream identifier;

	while (input_file_.get(character)) {
		if (escape_seq) {
			identifier << character;
			++col_number_;
			escape_seq = false;
		} else if (character == '\\') {
			identifier << character;
			++col_number_;
			escape_seq = true;
		} else if (character == '\n') {
			++line_number_;
			throw ParserError("Multi-line strings are not supported", line_number_, 1);
		} else if (character == '"') {
			auto it = identifier_map_.find(identifier.str());
			if (it != identifier_map_.end()) {
				output_file_ << it->second;
			} else {
				auto hex_val = ConvertToHexString(identifier.str());
				identifier_map_[identifier.str()] = hex_val;
				output_file_ << hex_val;
			}

			output_file_ << character;
			++col_number_;
			return;
		} else {
			identifier << character;
			++col_number_;
		}
	}
}

void Parser::Parse() {
	ParseSpace();
	ParseObject();
	ParseSpace();

	if (!input_file_.eof() ) {
		throw ParserError("Unexpected token", line_number_, col_number_);
	}
}

std::string Parser::ParseEscapeSequence(std::stringstream& identifier_stream) {
	char character;
	if (!identifier_stream.get(character)) {
		throw ParserError("Missing escape sequence", line_number_, col_number_);
	}

	// No apparent connection between an escape sequence and it's
	// hex representation could be found, so just hard code it
	switch (character) {
	case 'b':
		return "\\u0008";

	case 'f':
		return "\\u000c";

	case 'n':
		return "\\u000a";

	case 'r':
		return "\\u000d";

	case 't':
		return "\\u0009";

	case '"':
		return "\\u0022";

	case '/':
		return "\\u002f";

	case '\\':
		return "\\u005c";

	// 4 character hex representation
	case 'u':
		int skip_chars{4};
		std::string ret_val{"\\u"};
		char unicode_character;
		while (skip_chars > 0 && identifier_stream.get(unicode_character)) {
			if (!isxdigit(unicode_character)) {
				throw ParserError("Invalid unicode character", line_number_, col_number_);
			}
			ret_val += unicode_character;
			--skip_chars;
		}

		if (skip_chars > 0) {
			throw ParserError("Unfinished unicode character", line_number_, col_number_);
		}
		return ret_val;
	}

	throw ParserError("Invalid escape sequence", line_number_, col_number_);
}

std::string Parser::ConvertToHexString(const std::string& identifier) {
	std::stringstream ret_val;
	std::stringstream identifier_stream{identifier};
	ret_val << std::hex;

	char character;
	while (identifier_stream.get(character)) {

		if (character == '\\') {
			ret_val << ParseEscapeSequence(identifier_stream);

		} else {
			ret_val << "\\u";
			unsigned int converted_char = static_cast<unsigned char>(character);

			// Prepend zeroes if a hex representation is shorter than 4 characters
			if (converted_char < 0x1000) {
				ret_val << 0;
			}
			if (converted_char < 0x100) {
				ret_val << 0;
			}
			if (converted_char < 0x10) {
				ret_val << 0;
			}

			ret_val << converted_char;
		}
	}

	return ret_val.str();
}

void Parser::OutputMappings() {
	bool first_entry{true};

	mapping_file_ << "{" << std::endl;
	for (const auto& entry : identifier_map_ ) {
		if (!first_entry) {
			mapping_file_ << "," << std::endl;
		} else {
			first_entry = false;
		}
		mapping_file_ << "\t\"" << entry.first << "\": \"" << entry.second << '"';
	}
	mapping_file_ << std::endl << "}";
}
