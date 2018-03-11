/*
 * JsonDelimiter.h
 *
 *  Created on: 11 Mar 2018
 *      Author: Juozas Varonenka
 */

#ifndef PARSER_ERROR_H_
#define PARSER_ERROR_H_

#include <exception>
#include <sstream>
#include <string>

/**
 * Parser error container
 */
class ParserError: public std::exception {
public:
	ParserError(const std::string& message, int line_number, int col_number):
		message_{} {
			std::stringstream message_stream;
		message_stream << "Line: "
						<< line_number
						<< ", Column: "
						<< col_number
						<< " "
						<< message;
		message_ = message_stream.str();
	}

	~ParserError() {}

	const char* what() const noexcept override {
		return message_.c_str();
	}

	ParserError(const ParserError&) = default;
	ParserError& operator=(const ParserError&) = default;

	ParserError(ParserError&&) = default;
	ParserError& operator=(ParserError&&) = default;

private:

	/**
	 * Formatter error message
	 */
	std::string message_;
};

#endif /* PARSER_ERROR_H_ */
