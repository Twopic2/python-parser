#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string_view>
#include <string>

#include "lexical.hpp"

std::string read_file(std::string_view filename) {
    std::ifstream file(filename.data());

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

void print_tokens(const std::vector<Token::token_class>& tokens, Lexical::lexical_class& lexer) {
    std::cout << std::left << std::setw(20) << "TOKEN TYPE"
              << std::setw(20) << "VALUE"
              << std::setw(10) << "LINE"
              << std::setw(10) << "COLUMN" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto& token : tokens) {
        std::string display_value = token.value;
        if (token.value == "\n") {
            display_value = "\\n";
        }

        std::cout << std::left << std::setw(20) << lexer.token_type_name(token)
                  << std::setw(20) << display_value
                  << std::setw(10) << token.line
                  << std::setw(10) << token.column << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.py>" << std::endl;
        std::cerr << "Example: " << argv[0] << " test.py" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    std::string code = read_file(filename);

    std::cout << "=== Lexical Analyzer ===" << std::endl;
    std::cout << "File: " << filename << std::endl;

    Lexical::lexical_class lexer(code);
    auto tokens = lexer.tokenize();

    print_tokens(tokens, lexer);

    std::cout << std::endl << "Total tokens: " << tokens.size() << std::endl;

    return 0;
}
