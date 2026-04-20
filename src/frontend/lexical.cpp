#include <cctype>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "frontend/lexical.hpp"
#include "frontend/token.hpp"

namespace TwoPy::Frontend {

lexical_class::lexical_class(const std::string& source)
    : m_source(source), m_curr_pos(m_source.data()), m_end(m_source.data() + m_source.size()), m_line(1), m_column(1) {
    indent.emplace_back(0);

    predefined_keyword = {
        {"if", token_type::KEYWORD_IF},
        {"else", token_type::KEYWORD_ELSE},
        {"elif", token_type::KEYWORD_ELIF},
        {"for", token_type::KEYWORD_FOR},
        {"while", token_type::KEYWORD_WHILE},
        {"def", token_type::KEYWORD_DEF},
        {"return", token_type::KEYWORD_RETURN},
        {"break", token_type::KEYWORD_BREAK},
        {"continue", token_type::KEYWORD_CONTINUE},
        {"pass", token_type::KEYWORD_PASS},
        {"True", token_type::KEYWORD_TRUE},
        {"False", token_type::KEYWORD_FALSE},
        {"None", token_type::KEYWORD_NONE},
        {"and", token_type::KEYWORD_AND},
        {"or", token_type::KEYWORD_OR},
        {"not", token_type::KEYWORD_NOT},
        {"in", token_type::KEYWORD_IN},
        {"is", token_type::KEYWORD_IS},
        {"class", token_type::KEYWORD_CLASS},
        {"import", token_type::KEYWORD_IMPORT},
        {"from", token_type::KEYWORD_FROM},
        {"as", token_type::KEYWORD_AS},
        {"try", token_type::KEYWORD_TRY},
        {"except", token_type::KEYWORD_EXCEPT},
        {"finally", token_type::KEYWORD_FINALLY},
        {"with", token_type::KEYWORD_WITH},
        {"lambda", token_type::KEYWORD_LAMBDA},
        {"yield", token_type::KEYWORD_YIELD},
        {"assert", token_type::KEYWORD_ASSERT},
        {"del", token_type::KEYWORD_DEL},
        {"global", token_type::KEYWORD_GLOBAL},
        {"nonlocal", token_type::KEYWORD_NONLOCAL},
        {"raise", token_type::KEYWORD_RAISE},
        {"async", token_type::KEYWORD_ASYNC},
        {"await", token_type::KEYWORD_AWAIT},
        {"match", token_type::KEYWORD_MATCH},
        {"case", token_type::KEYWORD_CASE},
        {"enum", token_type::KEYWORD_ENUM},
        {"self", token_type::KEYWORD_SELF},
        {"__init__", token_type::KEYWORD_INIT},
    };
}

/*
Creating an object in place such as literals use emplace back. If the type is trivial no difference between the two.
*/

std::string read_file(std::string_view filename) {
    std::ifstream file(filename.data());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + std::string(filename));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void lexical_class::handle_indentation(std::vector<token_class>& tokens, std::size_t start_line) {
    std::size_t spaces = 0;
    while (m_curr_pos < m_end && (*m_curr_pos == ' ' || *m_curr_pos == '\t')) {
        if (*m_curr_pos == '\t') {
            spaces += 4;
        } else {
            spaces++;
        }
        next_token();
    }

    if (m_curr_pos >= m_end || *m_curr_pos == '\n') {
        return;
    }

    std::size_t current_indent = indent.back();

    if (spaces > current_indent) {
        indent.push_back(spaces);
        tokens.push_back({.type=token_type::INDENT, .value="", .line=start_line, .column=spaces});
    } else if (spaces < current_indent) {
        while (!indent.empty() && indent.back() > spaces) {
            indent.pop_back();
            tokens.push_back({.type=token_type::DEDENT, .value="", .line=start_line, .column=spaces});
        }

        if (indent.empty() || indent.back() != spaces) {
            throw std::runtime_error("Indentation error at line " + std::to_string(m_line));
        }
    }
}

bool lexical_class::is_whitespace() const {
    if (m_curr_pos >= m_end) { 
        return false;
    }

    char c = *m_curr_pos;
    return c == ' ' || c == '\t' || c == '\r';
}

bool lexical_class::is_string() const {
    if (m_curr_pos >= m_end) { 
        return false;
    }
    char c = *m_curr_pos;
    return c == '"' || c == '\'';
}

bool lexical_class::is_float() const {
    if (m_curr_pos >= m_end) return false;

    const char* temp = m_curr_pos;
    while (temp < m_end && std::isdigit(*temp)) ++temp;

    if (temp < m_end && *temp == '.') {
        if (temp + 1 < m_end && std::isdigit(*(temp + 1))) {
            return true;
        }
    }

    return false;
}

bool lexical_class::is_integer() const {
    if (m_curr_pos >= m_end) return false;
    return std::isdigit(*m_curr_pos);
}

bool lexical_class::is_identifier() const {
    if (m_curr_pos >= m_end) return false;
    char c = *m_curr_pos;
    return std::isalpha(c) || c == '_';
}

void lexical_class::next_token() {
    if (m_curr_pos < m_end) {
        if (*m_curr_pos == '\n') {
            m_line++;
            m_column = 0;
        } else {
            m_column++;
        }
        ++m_curr_pos;
    }
}

std::vector<token_class> lexical_class::tokenize() {
    std::vector<token_class> tokens;

    bool at_line_start = true;

    while (m_curr_pos < m_end) {
        std::size_t start_line = m_line;
        std::size_t start_column = m_column;

        if (at_line_start && *m_curr_pos != '\n') {
            handle_indentation(tokens, start_line);
            at_line_start = false;
            continue;
        }

        if (is_whitespace()) {
            next_token();
            continue;
        }

        if (*m_curr_pos == '\n') {
            tokens.push_back({token_type::NEWLINE, "\n", start_line, start_column});
            next_token();
            at_line_start = true;
            continue;
        }

        if (is_float()) {
            const char* start = m_curr_pos;

            while (m_curr_pos < m_end && std::isdigit(*m_curr_pos)) next_token();
            if (m_curr_pos < m_end && *m_curr_pos == '.') next_token();
            while (m_curr_pos < m_end && std::isdigit(*m_curr_pos)) next_token();

            tokens.push_back({token_type::FLOAT_LITERAL, std::string(start, m_curr_pos - start), start_line, start_column});
            continue;
        }

        if (is_integer()) {
            const char* start = m_curr_pos;
            while (m_curr_pos < m_end && std::isdigit(*m_curr_pos)) next_token();

            tokens.push_back({token_type::INTEGER_LITERAL, std::string(start, m_curr_pos - start), start_line, start_column});
            continue;
        }

        if (is_string()) {
            char quote = *m_curr_pos;
            next_token();
            const char* start = m_curr_pos;

            while (m_curr_pos < m_end && *m_curr_pos != quote) {
                if (*m_curr_pos == '\\') {
                    next_token();
                    if (m_curr_pos < m_end) next_token();
                } else {
                    next_token();
                }
            }

            std::string str(start, m_curr_pos - start);
            if (m_curr_pos < m_end) next_token();

            tokens.push_back({token_type::STRING_LITERAL, str, start_line, start_column});
            continue;
        }

        if (is_identifier()) {
            const char* start = m_curr_pos;

            while (m_curr_pos < m_end && (std::isalnum(*m_curr_pos) || *m_curr_pos == '_')) {
                next_token();
            }

            std::string identifier(start, m_curr_pos - start);

            auto it = predefined_keyword.find(identifier);
            if (it == predefined_keyword.end()) {
                tokens.push_back({token_type::IDENTIFIER, identifier, start_line, start_column});
            } else {
                tokens.push_back({it->second, identifier, start_line, start_column});
            }
            continue;
        }

        if (m_curr_pos + 2 < m_end) {
            if (m_curr_pos[0] == '.' && m_curr_pos[1] == '.' && m_curr_pos[2] == '.') {
                tokens.push_back({token_type::ELLIPSIS, "...", start_line, start_column});
                m_curr_pos += 3; m_column += 3; continue;
            } else if (m_curr_pos[0] == '/' && m_curr_pos[1] == '/' && m_curr_pos[2] == '=') {
                tokens.push_back({token_type::DOUBLE_SLASH_EQUAL, "//=", start_line, start_column});
                m_curr_pos += 3; m_column += 3; continue;
            } else if (m_curr_pos[0] == '*' && m_curr_pos[1] == '*' && m_curr_pos[2] == '=') {
                tokens.push_back({token_type::POWER_EQUAL, "**=", start_line, start_column});
                m_curr_pos += 3; m_column += 3; continue;
            } else if (m_curr_pos[0] == '<' && m_curr_pos[1] == '<' && m_curr_pos[2] == '=') {
                tokens.push_back({token_type::LEFT_SHIFT_EQUAL, "<<=", start_line, start_column});
                m_curr_pos += 3; m_column += 3; continue;
            } else if (m_curr_pos[0] == '>' && m_curr_pos[1] == '>' && m_curr_pos[2] == '=') {
                tokens.push_back({token_type::RIGHT_SHIFT_EQUAL, ">>=", start_line, start_column});
                m_curr_pos += 3; m_column += 3; continue;
            }
        }

        if (m_curr_pos + 1 < m_end) {
            if (m_curr_pos[0] == '/' && m_curr_pos[1] == '/') {
                tokens.push_back({token_type::DOUBLE_SLASH, "//", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '*' && m_curr_pos[1] == '*') {
                tokens.push_back({token_type::POWER, "**", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '<' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::LESS_EQUAL, "<=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '>' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::GREATER_EQUAL, ">=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '=' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::DOUBLE_EQUAL, "==", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '!' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::NOT_EQUAL, "!=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '+' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::PLUS_EQUAL, "+=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '-' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::MINUS_EQUAL, "-=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '*' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::STAR_EQUAL, "*=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '/' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::SLASH_EQUAL, "/=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '%' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::PERCENT_EQUAL, "%=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '@' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::AT_EQUAL, "@=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '&' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::AMPERSAND_EQUAL, "&=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '|' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::PIPE_EQUAL, "|=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '^' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::CARET_EQUAL, "^=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == ':' && m_curr_pos[1] == '=') {
                tokens.push_back({token_type::WALRUS, ":=", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '-' && m_curr_pos[1] == '>') {
                tokens.push_back({token_type::ARROW, "->", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '<' && m_curr_pos[1] == '<') {
                tokens.push_back({token_type::LEFT_SHIFT, "<<", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            } else if (m_curr_pos[0] == '>' && m_curr_pos[1] == '>') {
                tokens.push_back({token_type::RIGHT_SHIFT, ">>", start_line, start_column});
                m_curr_pos += 2; m_column += 2; continue;
            }
        }

        token_type type;
        bool matched = true;

        char current = *m_curr_pos;
        switch (current) {
            case '+': type = token_type::PLUS; break;
            case '-': type = token_type::MINUS; break;
            case '*': type = token_type::STAR; break;
            case '/': type = token_type::SLASH; break;
            case '%': type = token_type::PERCENT; break;
            case '@': type = token_type::AT; break;
            case '<': type = token_type::LESS; break;
            case '>': type = token_type::GREATER; break;
            case '=': type = token_type::EQUAL; break;
            case '(': type = token_type::LPAREN; break;
            case ')': type = token_type::RPAREN; break;
            case '[': type = token_type::LBRACKET; break;
            case ']': type = token_type::RBRACKET; break;
            case '{': type = token_type::LCBRACE; break;
            case '}': type = token_type::RCBRACE; break;
            case ',': type = token_type::COMMA; break;
            case ':': type = token_type::COLON; break;
            case ';': type = token_type::SEMICOLON; break;
            case '.': type = token_type::DOT; break;
            case '&': type = token_type::AMPERSAND; break;
            case '|': type = token_type::PIPE; break;
            case '^': type = token_type::CARET; break;
            case '~': type = token_type::TILDE; break;
            default:
                matched = false;
                break;
        }

        if (matched) {
            tokens.push_back({type, std::string(1, current), start_line, start_column});
            next_token();
            continue;
        }

        tokens.push_back({token_type::DEFAULT, std::string(1, current), start_line, start_column});
        next_token();
    }

    while (indent.size() > 1) {
        indent.pop_back();
        tokens.push_back({token_type::DEDENT, "", m_line, m_column});
    }

    tokens.push_back({token_type::EOF_TOKEN, "", m_line, m_column});

    return tokens;
}

}
