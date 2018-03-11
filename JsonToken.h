/*
 * JsonDelimiter.h
 *
 *  Created on: 11 Mar 2018
 *      Author: Juozas Varonenka
 */

#ifndef JSON_DELIMITER_H_
#define JSON_DELIMITER_H_

/**
 * Various acceptable JSON tokens
 */
enum class JsonToken {
	None,
	Done,
	ObjectStart,
	ObjectEnd,
	ObjectSeparator,
	Iterator,
	EscapeSequence,
	Quotes,
	ArrayStart,
	ArrayEnd,
	String,
	Value
};

#endif /* JSON_DELIMITER_H_ */
