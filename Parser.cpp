/*
 * Parser.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include "Parser.h"

#include <sstream>

Parser::Parser(const std::string& input_file,
				const std::string& output_file,
				const std::string& mapping_file) :
	input_file_{input_file},
	output_file_{output_file},
	mapping_file_{mapping_file},
	is_ok_{true} {

	if (!input_file_.is_open()) {
		is_ok_ = false;
	}

	if (!output_file_.is_open()) {
		is_ok_ = false;
	}

	if (!mapping_file_.is_open()) {
		is_ok_ = false;
	}
}

void Parser::Parse() {
	bool in_string{false};
	std::stringstream identifier;
	bool escape_seq{false};

	char character;
	while (input_file_.get(character)) {

		if (escape_seq) {
			escape_seq = false;
			identifier << character;

		} else if (character == '"') {

			if (!in_string) {
				in_string = true;
				identifier.str(std::string{});

			} else {
				in_string = false;
				output_file_ << '"';

				auto it = identifierMap_.find(identifier.str());
				if (it != identifierMap_.end()) {
					output_file_ << it->second;
				} else {
					auto hex_val = ConvertToHexString(identifier.str());
					identifierMap_[identifier.str()] = hex_val;
					output_file_ << hex_val;
				}

				output_file_ << '"';
			}

		} else if (in_string) {
			identifier << character;
			if (character == '\\') {
				escape_seq = true;
			}

		} else {
			output_file_.put(character);
		}
	}

	if (!input_file_.eof()) {
		is_ok_ = false;
	}
}

std::string Parser::ParseEscapeSequence(std::stringstream& identifier_stream) {
	char character;
	if (!identifier_stream.get(character)) {
		is_ok_ = false;
		return "";
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
			ret_val += unicode_character;
			--skip_chars;
		}
		return ret_val;
	}

	is_ok_ = false;
	return "";
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
	for (const auto& entry : identifierMap_ ) {
		if (!first_entry) {
			mapping_file_ << "," << std::endl;
		} else {
			first_entry = false;
		}
		mapping_file_ << "\t\"" << entry.first << "\": \"" << entry.second << '"';
	}
	mapping_file_ << std::endl << "}";
}
