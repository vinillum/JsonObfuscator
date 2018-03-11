/*
 * Parser.h
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <fstream>
#include <string>
#include <map>

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
	 * Return parser status
	 */
	bool IsOk() { return is_ok_; }

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
	 * Parser status
	 */
	bool is_ok_;

	/**
	 * Conversion map from a string to it's hex version
	 */
	std::map<std::string, std::string> identifierMap_;
};

#endif /* PARSER_H_ */
