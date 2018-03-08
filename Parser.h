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

	/**
	 * Constructor
	 */
	Parser(std::string inputFile, std::string outputFile, std::string mappingFile);

	/**
	 * Destructor
	 */
	~Parser();

	/**
	 * Parse the input file, writing to output file immediately
	 */
	void Parse();

	/**
	 * Return parser status
	 */
	inline bool IsOk();

	/**
	 * Outputs mappings into a file
	 */
	void OutputMappings();

private:

	// Don't allow copying and assignment
	Parser(const Parser&) = delete;
	Parser& operator=(const Parser&) = delete;

	/**
	 * Converts a string identifier to a hex version
	 */
	std::string ConvertToHexString(const std::string& identifier);

	/**
	 * Parses escape sequence in a string
	 */
	std::string ParseEscapeSequence(const char character);

	/**
	 * Input stream
	 */
	std::ifstream inputFile_;

	/**
	 * Output stream
	 */
	std::ofstream outputFile_;

	/**
	 * Mapping stream
	 */
	std::ofstream mappingFile_;

	/**
	 * Parser status
	 */
	bool isOk_;

	/**
	 * Conversion map from a string to it's hex version
	 */
	std::map<std::string, std::string> identifierMap_;
};

/**
 * IsOk
 */
bool Parser::IsOk() {
	return isOk_;
}

#endif /* PARSER_H_ */
