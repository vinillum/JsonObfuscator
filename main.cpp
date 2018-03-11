/*
 * main.cpp
 *
 *  Created on: 8 Mar 2018
 *      Author: Juozas Varonenka
 */

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

#include "Parser.h"

constexpr int kExpectedNoOfArgs{7};

/**
 * Show help message
 */
void showHelp() {
	std::cerr << "Missing input, output and/or mapping files" << std::endl
			  << "Usage: tool -i <inputFile> -o <outputFile> -m <mapping file>" << std::endl;
	exit(1);
}

int main(int argc, char* argv[]) {
	std::string input_file;
	std::string output_file;
	std::string mapping_file;

	if (argc != kExpectedNoOfArgs) {
		showHelp();
	}

	// Parse command line input
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-i") == 0) {
			++i;
			input_file = argv[i];
		} else if (strcmp(argv[i], "-o") == 0) {
			++i;
			output_file = argv[i];
		} else if (strcmp(argv[i], "-m") == 0) {
			++i;
			mapping_file = argv[i];
		} else {
			showHelp();
		}
	}

	if (input_file.empty() || output_file.empty() || mapping_file.empty()) {
		showHelp();
	}

	try {
		Parser parser(input_file, output_file, mapping_file);
		parser.Parse();
		parser.OutputMappings();
	}
	catch (std::exception& e) {
		std::cerr << e.what();
		return 1;
	}

	std::cout << "Successfully parsed the JSON file" << std::endl;
	return 0;
}
