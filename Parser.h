/*
 * Parser.h
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <fstream>
#include <map>
#include <string>

#include "ParserError.h"

/**
 * Parser for JSON files
 *
 * Given an input JSON file, it will convert all found strings into their
 * hex counterparts
 */
class Parser {
public:

	Parser(const std::string& input_file,
			const std::string& output_file,
			const std::string& mapping_file);

	~Parser() { }

	/**
	 * Parse the input file, writing to output file immediately
	 */
	void Parse();

	/**
	 * Outputs mappings into a file
	 */
	void OutputMappings();

	Parser(Parser&&) = default;
	Parser& operator=(Parser&&) = default;

private:

	Parser(const Parser&) = delete;
	Parser& operator=(const Parser&) = delete;

	/**
	 * Converts a string identifier to a hex version
	 */
	std::string ConvertToHexString(const std::string& identifier);

	/**
	 * Parses escape sequence in a string
	 */
	std::string ParseEscapeSequence(std::istringstream& identifier_stream);

	/**
	 * Parse { } object block
	 */
	void ParseObject();

	/**
	 * Parse space inbetween tokens
	 */
	void ParseSpace();

	/**
	 * Parse strings inbetween quotes
	 */
	void ParseString();

	/**
	 * Parse value by calling other parsers
	 */
	void ParseValue();

	/**
	 * Parse name/value pair
	 */
	void ParsePair();

	/**
	 * Pare [ ] array block
	 */
	void ParseArray();

	/**
	 * Parse true, false and null
	 */
	void ParseConst();

	/**
	 * Parse number
	 */
	void ParseNumber();

	/**
	 * Raise a parsing error
	 */
	void RaiseError(const std::string& message) {
		throw ParserError(message, line_number_, col_number_);
	}

	/**
	 * Stream token helper
	 */
	template<typename T>
	void StreamToken(std::basic_ostream<T>& token_stream) {
		token_stream.put(last_token_);
		++col_number_;
	}

	/**
	 * Various tokens
	 */
	static constexpr char kObjectStart = '{';
	static constexpr char kObjectEnd = '}';
	static constexpr char kStringDelim = '"';
	static constexpr char kSeparator = ',';
	static constexpr char kPairDelim = ':';
	static constexpr char kNewLine = '\n';
	static constexpr char kArrayStart = '[';
	static constexpr char kArrayEnd = ']';
	static constexpr char kMinusSign = '-';
	static constexpr char kEscapeSeqStart = '\\';

	/**
	 * Various supported constants
	 */
	static const std::string kTrueConst;
	static const std::string kFalseConst;
	static const std::string kNullConst;

	/**
	 * Unicode start
	 */
	static const std::string kUnicodeStart;

	/**
	 * Most used error message
	 */
	static const std::string kUnexpectedToken;

	/**
	 * Input stream
	 */
	std::ifstream input_file_;

	/**
	 * Output stream
	 */
	std::ofstream output_file_;

	/**
	 * Mapping stream
	 */
	std::ofstream mapping_file_;

	/**
	 * Conversion map from a string to it's hex version
	 */
	std::map<std::string, std::string> identifier_map_;

	/**
	 * Current line number in the input stream
	 */
	int line_number_;

	/**
	 * Current column number in the input stream
	 */
	int col_number_;

	/**
	 * Last retrieved token
	 */
	char last_token_;
};

#endif /* PARSER_H_ */
