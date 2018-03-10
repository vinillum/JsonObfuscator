/*
 * Parser.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include "Parser.h"

#include <sstream>
#include <iostream>

/**
 * Constructor
 */
Parser::Parser(const std::string& inputFile,
				const std::string& outputFile,
				const std::string& mappingFile) :
	inputFile_{inputFile},
	outputFile_{outputFile},
	mappingFile_{mappingFile},
	isOk_{true} {

	if (!inputFile_.is_open()) {
		isOk_ = false;
	}

	if (!outputFile_.is_open()) {
		isOk_ = false;
	}

	if (!mappingFile_.is_open()) {
		isOk_ = false;
	}
}

/**
 * Parse
 */
void Parser::Parse() {
	// Flag to indicate whether we are in a string
	bool inString{false};

	// Building string identifier
	std::stringstream identifier;

	// Currently processed character
	char character;

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
		int nextChars{4};
		std::string retVal{"\\u"};
		// For special hex representations, put the next x characters
		// as they come into the return value
		char uniCharacter;
		while (nextChars > 0 && inputFile_.get(uniCharacter)) {
			retVal += uniCharacter;
			--nextChars;
		}
		return retVal;
	}

	isOk_ = false;
	return "";
}

/**
 * ConvertToHexString
 */
std::string Parser::ConvertToHexString(const std::string& identifier) {
	// Return value
	std::stringstream hexVal;

	// Enable hex mode - will only affect integer output
	hexVal << std::hex;

	// How many characters need to ignore the special conversion logic
	int ignoreChars{0};

	// Is escape sequence expected next?
	bool escapeSeq{false};

	for (const unsigned char character: identifier) {

		// Escape sequence is expected next
		if (character == '\\') {
			escapeSeq = true;
			hexVal << character;

		// Determine how many characters come in an escape sequence
		} else if (escapeSeq) {
			escapeSeq = false;
			ignoreChars = 4;
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

	mappingFile_ << "{" << std::endl;
	for (const auto& entry : identifierMap_ ) {
		if (!firstEntry) {
			mappingFile_ << "," << std::endl;
		} else {
			firstEntry = false;
		}
		mappingFile_ << "\t\"" << entry.first << "\": \"" << entry.second << '"';
	}
	mappingFile_ << std::endl << "}";
}
