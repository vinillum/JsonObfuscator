/*
 * main.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include <iostream>
#include <cstring>
#include "Parser.h"

constexpr int expectedNoOfArgs{7};

/**
 * Show help message
 */
void showHelp() {
	std::cerr << "Missing input, output and/or mapping files" << std::endl
			  << "Usage: tool -i <inputFile> -o <outputFile> -m <mapping file>" << std::endl;
	exit(1);
}

/**
 * Entry point
 */
int main(int argc, char* argv[]) {
	std::string inputFile;
	std::string outputFile;
	std::string mappingFile;

	if (argc != expectedNoOfArgs) {
		showHelp();
	}

	// Parse command line input
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-i") == 0) {
			++i;
			inputFile = argv[i];
		} else if (strcmp(argv[i], "-o") == 0) {
			++i;
			outputFile = argv[i];
		} else if (strcmp(argv[i], "-m") == 0) {
			++i;
			mappingFile = argv[i];
		} else {
			showHelp();
		}
	}

	// Both input and output are required
	if (inputFile.empty() || outputFile.empty() || mappingFile.empty()) {
		showHelp();
	}

	Parser parser(inputFile, outputFile, mappingFile);

	if (parser.IsOk()) {
		parser.Parse();
	} else {
		std::cerr << "Failed to initialise parser" << std::endl;
	}

	if (parser.IsOk()) {
		parser.OutputMappings();
	} else {
		std::cerr << "Failed to parse the JSON file" << std::endl;
	}

	if (parser.IsOk()) {
		std::cout << "Successfully parsed the JSON file" << std::endl;
	}

	return (parser.IsOk() ? 0 : 1);
}
