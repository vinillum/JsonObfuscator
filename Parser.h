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
#include <stack>
#include <string>

#include "JsonToken.h"

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
	std::string ParseEscapeSequence(std::stringstream& identifier_stream);

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
	 * Stack of processed JSON tokens
	 */
	std::stack<JsonToken> token_stack_;

	/**
	 * Current line number in the input stream
	 */
	int line_number_;

	/**
	 * Current column number in the input stream
	 */
	int col_number_;
};

#endif /* PARSER_H_ */
