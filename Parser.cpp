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

const std::string Parser::kTrueConst = "true";
const std::string Parser::kFalseConst = "false";
const std::string Parser::kNullConst = "null";

const std::string Parser::kUnicodeStart = "\\u";

const std::string Parser::kUnexpectedToken = "Unexpected token";

Parser::Parser(const std::string& input_file,
				const std::string& output_file,
				const std::string& mapping_file) :
	input_file_{input_file},
	output_file_{output_file},
	mapping_file_{mapping_file},
	line_number_{1},
	col_number_{0},
	last_token_{0} {

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

void Parser::ParseObject() {
	if (last_token_ == kObjectStart) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}

	ParseSpace();

	if (last_token_ == kObjectEnd) {
		StreamToken(output_file_);
		return;
	} else if (last_token_ == kStringDelim) {
		ParsePair();
	} else {
		RaiseError(kUnexpectedToken);
	}

	ParseSpace();

	while (last_token_ == kSeparator) {
		StreamToken(output_file_);
		ParseSpace();
		ParsePair();
		ParseSpace();
	}

	if (last_token_ == kObjectEnd) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}
}

void Parser::ParsePair() {
	ParseString();
	ParseSpace();

	if (last_token_ == kPairDelim) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}

	ParseSpace();
	ParseValue();
}

void Parser::ParseSpace() {
	while (input_file_.get(last_token_)) {
		if (last_token_ == kNewLine) {
			output_file_ << last_token_;
			++line_number_;
			col_number_ = 1;
		} else if (isspace(last_token_)) {
			StreamToken(output_file_);
		} else {
			break;
		}
	}
}

void Parser::ParseArray() {
	if (last_token_ == kArrayStart) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}

	ParseSpace();

	if (last_token_ == kArrayEnd) {
		StreamToken(output_file_);
		return;
	}

	ParseValue();
	ParseSpace();

	while (last_token_ == kSeparator) {
		StreamToken(output_file_);
		ParseSpace();
		ParseValue();
		ParseSpace();
	}

	if (last_token_ == kArrayEnd) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}
}

void Parser::ParseConst() {
	StreamToken(output_file_);
	std::string compare_string;

	if (last_token_ == kTrueConst[0]) {
		compare_string = kTrueConst;
	} else if (last_token_ == kFalseConst[0]) {
		compare_string = kFalseConst;
	} else if (last_token_ == kNullConst[0]) {
		compare_string = kNullConst;
	} else {
		RaiseError(kUnexpectedToken);
	}

	size_t next_chars{compare_string.length() - 1};
	std::ostringstream identifier;
	identifier << last_token_;

	while (next_chars > 0 && input_file_.get(last_token_)) {
		StreamToken(output_file_);
		identifier << last_token_;
		--next_chars;
	}

	if (next_chars > 0 || identifier.str() != compare_string) {
		RaiseError(kUnexpectedToken);
	}
}

void Parser::ParseNumber() {
	if (isdigit(last_token_) || last_token_ == kMinusSign) {
		input_file_.putback(last_token_);
		auto stream_position_start = input_file_.tellg();

		double digit;
		input_file_ >> digit;
		if (input_file_.fail()) {
			RaiseError("Invalid number");
		}

		// We have move past the digit now, and if we streamed
		// just read digit out, it would get incorrectly formatted
		// thus re-stream original characters
		auto stream_position_end = input_file_.tellg();
		input_file_.seekg(stream_position_start);
		while (input_file_.tellg() != stream_position_end) {
			input_file_.get(last_token_);
			StreamToken(output_file_);
		}
	} else {
		RaiseError(kUnexpectedToken);
	}
}

void Parser::ParseValue() {
	switch (last_token_) {
	case kObjectStart: {
		ParseObject();
		break;
	}

	case kArrayStart: {
		ParseArray();
		break;
	}

	case kStringDelim: {
		ParseString();
		break;
	}

	default:
		if (last_token_ == kTrueConst[0] ||
			last_token_ == kFalseConst[0] ||
			last_token_ == kNullConst[0]) {
			ParseConst();
		} else {
			ParseNumber();
		}
		break;
	}
}

void Parser::ParseString() {
	if (last_token_ == kStringDelim) {
		StreamToken(output_file_);
	} else {
		RaiseError(kUnexpectedToken);
	}

	bool escape_seq{false};
	std::stringstream identifier;

	while (input_file_.get(last_token_)) {
		if (escape_seq) {
			StreamToken(identifier);
			escape_seq = false;
		} else if (last_token_ == kEscapeSeqStart) {
			StreamToken(identifier);
			escape_seq = true;
		} else if (last_token_ == kNewLine) {
			RaiseError("Multi-line strings are not supported");
		} else if (last_token_ == kStringDelim) {
			auto it = identifier_map_.find(identifier.str());
			if (it != identifier_map_.end()) {
				output_file_ << it->second;
			} else {
				auto hex_val = ConvertToHexString(identifier.str());
				identifier_map_[identifier.str()] = hex_val;
				output_file_ << hex_val;
			}

			StreamToken(output_file_);
			return;
		} else {
			StreamToken(identifier);
		}
	}
}

void Parser::Parse() {
	ParseSpace();
	ParseObject();
	ParseSpace();

	if (!input_file_.eof() ) {
		RaiseError(kUnexpectedToken);
	}
}

std::string Parser::ParseEscapeSequence(std::istringstream& identifier_stream) {
	char character;
	if (!identifier_stream.get(character)) {
		RaiseError("Missing escape sequence");
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
		int next_chars{4};
		std::string ret_val{kUnicodeStart};
		while (next_chars > 0 && identifier_stream.get(character)) {
			if (!isxdigit(character)) {
				RaiseError("Invalid unicode character");
			}
			ret_val.push_back(character);
			--next_chars;
		}

		if (next_chars > 0) {
			RaiseError("Unfinished unicode character");
		}

		return ret_val;
	}

	RaiseError("Invalid escape sequence");
	return "";
}

std::string Parser::ConvertToHexString(const std::string& identifier) {
	std::ostringstream ret_val;
	std::istringstream identifier_stream{identifier};
	ret_val << std::hex;

	char character;
	while (identifier_stream.get(character)) {

		if (character == kEscapeSeqStart) {
			ret_val << ParseEscapeSequence(identifier_stream);

		} else {
			ret_val << kUnicodeStart;
			unsigned int converted_char = static_cast<unsigned char>(character);

			// Unicode encoding
			if (converted_char & 0b10000000) {

				// Decode code point from UTF-8
				uint32_t code_point{0};
				int next_chars;
				if ((converted_char & 0b11110000) == 0b11110000) {
					next_chars = 3;
					code_point |= (converted_char & 0b00000111) << 18;
				} else if ((converted_char & 0b11100000) == 0b11100000) {
					next_chars = 2;
					code_point |= (converted_char & 0b00001111) << 12;
				} else if ((converted_char & 0b11000000) == 0b11000000) {
					next_chars = 1;
					code_point |= (converted_char & 0b00011111) << 6;
				}

				while (next_chars > 0 && identifier_stream.get(character)) {
					converted_char = static_cast<unsigned char>(character);
					code_point |= (converted_char & 0b00111111) << ((next_chars - 1) * 6);
					--next_chars;
				}

				if (next_chars > 0) {
					RaiseError("Invalid UTF-8 encoding");
				}

				// Encode code point into surrogate pairs
				if (code_point < 0xd800) {
					if (code_point < 0x1000) {
						ret_val << 0;
					}
					if (code_point < 0x100) {
						ret_val << 0;
					}
					if (code_point < 0x10) {
						ret_val << 0;
					}
					ret_val << code_point;
				} else if (code_point > 0xdfff) {
					uint32_t high_pair = (code_point - 0x10000) / 0x400 + 0xD800;
					uint32_t low_pair = (code_point - 0x10000) % 0x400 + 0xDC00;
					ret_val << high_pair << kUnicodeStart << low_pair;
				} else {
					RaiseError("Invalid code point");
				}

			// ASCII encoding
			} else {
				ret_val << "00";
				if (converted_char < 0x10) {
					ret_val << 0;
				}
				ret_val << converted_char;
			}
		}
	}

	return ret_val.str();
}

void Parser::OutputMappings() {
	bool first_entry{true};

	mapping_file_ << kObjectStart << std::endl;
	for (const auto& entry : identifier_map_ ) {
		if (!first_entry) {
			mapping_file_ << kSeparator << std::endl;
		} else {
			first_entry = false;
		}
		mapping_file_ << "\t" << kStringDelim << entry.first << kStringDelim << kPairDelim << ' ' << kStringDelim << entry.second << kStringDelim;
	}
	mapping_file_ << std::endl << kObjectEnd;
}
