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

void Parser::Parse() {
	std::stringstream identifier;
	JsonToken prev_token{JsonToken::None};

	char character;
	while (input_file_.get(character)) {
		++col_number_;

		if (!token_stack_.empty() && token_stack_.top() == JsonToken::EscapeSequence) {
			identifier << character;
			token_stack_.pop();

		} else if (!token_stack_.empty() && token_stack_.top() == JsonToken::Quotes) {
			if (character == '\\') {
				identifier << character;
				token_stack_.push(JsonToken::EscapeSequence);

			} else if (character == '"') {
				token_stack_.pop();
				output_file_ << '"';

				auto it = identifier_map_.find(identifier.str());
				if (it != identifier_map_.end()) {
					output_file_ << it->second;
				} else {
					auto hex_val = ConvertToHexString(identifier.str());
					identifier_map_[identifier.str()] = hex_val;
					output_file_ << hex_val;
				}

				output_file_ << '"';

				// Determine if this a string or a value for tokenisation purposes
				if (prev_token == JsonToken::ObjectSeparator ||
					(!token_stack_.empty() && token_stack_.top() == JsonToken::ArrayEnd)) {
					prev_token = JsonToken::Value;
				} else {
					prev_token = JsonToken::String;
				}

				identifier.str(std::string{});

			} else if (character == '\n') {
				throw ParserError("Multiline strings are not supported", line_number_, col_number_);

			} else {
				identifier << character;
			}

		} else if (character == '"') {
			if (prev_token != JsonToken::ArrayStart &&
				prev_token != JsonToken::ObjectStart &&
				prev_token != JsonToken::Iterator &&
				prev_token != JsonToken::ObjectSeparator) {
				throw ParserError("Misplaced token", line_number_, col_number_);
			}

			token_stack_.push(JsonToken::Quotes);

		} else {
			switch (character) {
			case '{': {
				if (prev_token != JsonToken::ObjectSeparator &&
					prev_token != JsonToken::None &&
					prev_token != JsonToken::ArrayStart &&
					(prev_token != JsonToken::Iterator ||
					(!token_stack_.empty() && token_stack_.top() == JsonToken::ObjectEnd))) {
					throw ParserError("Misplaced token", line_number_, col_number_);
				}

				output_file_.put(character);
				token_stack_.push(JsonToken::ObjectEnd);
				prev_token = JsonToken::ObjectStart;
				break;
			}

			case '}': {
				if (token_stack_.empty() || token_stack_.top() != JsonToken::ObjectEnd) {
					throw ParserError("Mismatched token", line_number_, col_number_);
				}

				output_file_.put(character);
				token_stack_.pop();
				if (token_stack_.empty()) {
					prev_token = JsonToken::Done;
				} else {
					prev_token = JsonToken::ObjectEnd;
				}
				break;
			}

			case '[': {
				if (prev_token != JsonToken::ObjectSeparator &&
					prev_token != JsonToken::ArrayStart &&
					(prev_token != JsonToken::Iterator ||
					(!token_stack_.empty() && token_stack_.top() != JsonToken::ArrayEnd))) {
					throw ParserError("Misplaced token", line_number_, col_number_);
				}

				output_file_.put(character);
				token_stack_.push(JsonToken::ArrayEnd);
				prev_token = JsonToken::ArrayStart;
				break;
			}

			case ']': {
				if (token_stack_.empty() || token_stack_.top() != JsonToken::ArrayEnd) {
					throw ParserError("Mismatched token", line_number_, col_number_);
				}

				output_file_.put(character);
				token_stack_.pop();
				prev_token = JsonToken::ArrayEnd;
				break;
			}

			case ':': {
				if (prev_token != JsonToken::String) {
					throw ParserError("Misplaced token", line_number_, col_number_);
				}

				output_file_.put(character);
				prev_token = JsonToken::ObjectSeparator;
				break;
			}

			case ',': {
				if (prev_token != JsonToken::Value &&
					prev_token != JsonToken::ObjectEnd &&
					prev_token != JsonToken::ArrayEnd) {
					throw ParserError("Misplaced token", line_number_, col_number_);
				}

				output_file_.put(character);
				prev_token = JsonToken::Iterator;
				break;
			}

			case 't': // FALL-THROUGH
			case 'f': // FALL-THROUGH
			case 'n': {
				if (prev_token != JsonToken::ObjectSeparator &&
					prev_token != JsonToken::ArrayStart &&
					(prev_token != JsonToken::Iterator ||
					(!token_stack_.empty() && token_stack_.top() != JsonToken::ArrayEnd))) {
					throw ParserError("Misplaced token", line_number_, col_number_);
				}

				identifier << character;
				int next_chars;
				std::string compare_string;
				if (character == 't') {
					next_chars = 3;
					compare_string = "true";
				} else if (character == 'f') {
					next_chars = 4;
					compare_string = "false";
				} else {
					next_chars = 3;
					compare_string = "null";
				}

				while (next_chars > 0 && input_file_.get(character)) {
					++col_number_;
					identifier << character;
					--next_chars;
				}

				if (next_chars > 0 || identifier.str() != compare_string) {
					throw ParserError("Unexpected token", line_number_, col_number_);
				}

				output_file_ << identifier.str();
				prev_token = JsonToken::Value;
				identifier.str(std::string{});
				break;
			}

			case '\n': {
				output_file_.put(character);
				++line_number_;
				col_number_ = 0;
				break;
			}

			default:
				if (isdigit(character) || character == '-') {
					if (prev_token != JsonToken::ObjectSeparator &&
						prev_token != JsonToken::ArrayStart &&
						prev_token != JsonToken::Iterator) {
						throw ParserError("Misplaced token", line_number_, col_number_);
					}

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

					prev_token = JsonToken::Value;

				} else if (!isspace(character)) {
					throw ParserError("Unexpected token", line_number_, col_number_);

				} else {
					output_file_.put(character);
				}
			}
		}
	}

	if (!input_file_.eof()) {
		throw std::runtime_error("Could not finish reading input file");
	} else if (prev_token == JsonToken::None ) {
		throw std::runtime_error("No JSON object found inside input file");
	} else if (prev_token != JsonToken::Done ) {
		throw std::runtime_error("JSON object was not finished");
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
