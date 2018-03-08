/*
 * Parser.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include "Parser.h"

#include <sstream>

/**
 * Constructor
 */
Parser::Parser(std::string inputFile, std::string outputFile, std::string mappingFile) :
	inputFile_{},
	outputFile_{},
	mappingFile_{},
	isOk_{true} {

	inputFile_.open(inputFile);
	if (!inputFile_.is_open()) {
		isOk_ = false;
	}

	outputFile_.open(outputFile);
	if (!outputFile_.is_open()) {
		isOk_ = false;
	}

	mappingFile_.open(mappingFile);
	if (!mappingFile_.is_open()) {
		isOk_ = false;
	}
}

/**
 * Destructor
 */
Parser::~Parser() {
	if (inputFile_.is_open()) {
		inputFile_.close();
	}

	if (outputFile_.is_open()) {
		outputFile_.close();
	}

	if (mappingFile_.is_open()) {
		mappingFile_.close();
	}
}

/**
 * Parse
 */
void Parser::Parse() {
	// Flag to indicate whether we are in a string
	bool inString{false};

	// Building string identifier
	std::stringstream identifier{};

	// Currenly processed character
	char character{0};

	// Has the previous character signalled an escape character is incoming
	bool escapeSeq{false};

	while (inputFile_.get(character)) {

		// If an escape character is incoming, process it with a special logic
		if (escapeSeq) {
			escapeSeq = false;
			identifier << ParseEscapeSequence(character);

		// Quotes identify a string
		} else if (character == '"') {

			// This is a new string
			if (!inString) {
				inString = true;
				identifier.str(std::string{});

			// We just finished processing a string
			} else {
				inString = false;
				outputFile_ << '"';

				// Either find an existing hex value for this identifier,
				// or create a new one
				auto it = identifierMap_.find(identifier.str());
				if (it != identifierMap_.end()) {
					outputFile_ << it->second;
				} else {
					auto hexVal = ConvertToHexString(identifier.str());
					identifierMap_[identifier.str()] = hexVal;
					outputFile_ << hexVal;
				}

				outputFile_ << '"';
			}

		// We are inside a string, thus put everything inside building identifier,
		// unless it's an escape character signal
		} else if (inString) {
			if (character == '\\') {
				escapeSeq = true;
			} else {
				identifier << character;
			}

		// Everything outside strings is left as is
		} else {
			outputFile_.put(character);
		}
	}

	if (!inputFile_.eof()) {
		isOk_ = false;
	}
}

/**
 * ParseEscapeSequence
 */
std::string Parser::ParseEscapeSequence(const char character) {
	// Identifies how many characters are expected in special hex representations
	int nextChars{0};

	// Return value
	std::string retVal{};

	// No apparent connection between an escape sequence and it's
	// hex representation could be found, so just hard code it
	switch (character) {
	case 'a':
		retVal = "\\u0007";
		break;

	case 'b':
		retVal = "\\u0008";
		break;

	case 'f':
		retVal = "\\u000c";
		break;

	case 'n':
		retVal = "\\u000a";
		break;

	case 'r':
		retVal = "\\u000d";
		break;

	case 't':
		retVal = "\\u0009";
		break;

	case 'v':
		retVal = "\\u000b";
		break;

	case '"':
		retVal = "\\u0022";
		break;

	case '\'':
		retVal = "\\u0027";
		break;

	case '\\':
		retVal = "\\u005c";
		break;


	// 2 character hex representation
	case 'x':
		nextChars = 2;
		retVal = "\\u00";
		break;

	// 4 character hex representation
	case 'u':
		nextChars = 4;
		retVal = "\\u";
		break;

	// 8 character hex representation
	case 'U':
		nextChars = 8;
		retVal = "\\U";
		break;

	default:
		isOk_ = false;
	}

	// For special hex representations, put the next x characters
	// as they come into the return value
	char uniCharacter{0};
	while (nextChars > 0 && inputFile_.get(uniCharacter)) {
		retVal += uniCharacter;
		--nextChars;
	}

	return retVal;
}

/**
 * ConvertToHexString
 */
std::string Parser::ConvertToHexString(const std::string& identifier) {
	// Return value
	std::stringstream hexVal{};

	// Enable hex mode - will only affect integer output
	hexVal << std::hex;

	// How many characters need to ignore the special conversion logic
	int ignoreChars{0};

	// Is escape sequence expected next?
	bool escapeSeq{false};

	for (const char character: identifier) {

		// Escape sequence is expected next
		if (character == '\\') {
			escapeSeq = true;
			hexVal << character;

		// Determine how many characters come in an escape sequence
		} else if (escapeSeq) {
			escapeSeq = false;
			if (character == 'u') {
				ignoreChars = 4;
			} else if (character == 'U') {
				ignoreChars = 8;
			} else {
				isOk_ = false;
			}
			hexVal << character;

		// Ignore conversion logic while inside espace sequence
		} else if (ignoreChars > 0) {
			--ignoreChars;
			hexVal << character;

		// Special conversion logic
		} else {
			hexVal << "\\u";
			int convertedChar = static_cast<int>(character);

			// Prepend zeroes if a hex representation is shorter than 4 characters
			if (convertedChar < 0x1000) {
				hexVal << 0;
			}
			if (convertedChar < 0x100) {
				hexVal << 0;
			}
			if (convertedChar < 0x10) {
				hexVal << 0;
			}

			hexVal << convertedChar;
		}
	}
	return hexVal.str();
}

/**
 * OutputMappings
 */
void Parser::OutputMappings() {
	// Flag to tell if a comma is required before the entry
	bool firstEntry{true};

	mappingFile_ << "{\n";
	for (const auto& entry : identifierMap_ ) {
		if (!firstEntry) {
			mappingFile_ << ",\n";
		} else {
			firstEntry = false;
		}
		mappingFile_ << "\t\"" << entry.first << "\": \"" << entry.second << '"';
	}
	mappingFile_ << "\n}";
}
