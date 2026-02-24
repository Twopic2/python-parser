#include <cctype>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "frontend/lexical.hpp"

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
        tokens.push_back({token_type::INDENT, "", start_line, spaces});
    } else if (spaces < current_indent) {
        while (!indent.empty() && indent.back() > spaces) {
            indent.pop_back();
            tokens.push_back({token_type::DEDENT, "", start_line, spaces});
        }

        if (indent.empty() || indent.back() != spaces) {
            throw std::runtime_error("Indentation error at line " + std::to_string(m_line));
        }
    }
}

bool lexical_class::is_whitespace() const {
    if (m_curr_pos >= m_end) return false;
    char c = *m_curr_pos;
    return c == ' ' || c == '\t' || c == '\r';
}

bool lexical_class::is_string() const {
    if (m_curr_pos >= m_end) return false;
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

std::string lexical_class::token_type_name(const token_class& token) {
    try {
        switch (token.type) {
            case token_type::KEYWORD_FALSE: return "KEYWORD_FALSE";
            case token_type::KEYWORD_NONE: return "KEYWORD_NONE";
            case token_type::KEYWORD_TRUE: return "KEYWORD_TRUE";
            case token_type::KEYWORD_AND: return "KEYWORD_AND";
            case token_type::KEYWORD_AS: return "KEYWORD_AS";
            case token_type::KEYWORD_CASE: return "KEYWORD_CASE";
            case token_type::KEYWORD_MATCH: return "KEYWORD_MATCH";
            case token_type::KEYWORD_ASSERT: return "KEYWORD_ASSERT";
            case token_type::KEYWORD_ASYNC: return "KEYWORD_ASYNC";
            case token_type::KEYWORD_AWAIT: return "KEYWORD_AWAIT";
            case token_type::KEYWORD_BREAK: return "KEYWORD_BREAK";
            case token_type::KEYWORD_CLASS: return "KEYWORD_CLASS";
            case token_type::KEYWORD_CONTINUE: return "KEYWORD_CONTINUE";
            case token_type::KEYWORD_DEF: return "KEYWORD_DEF";
            case token_type::KEYWORD_DEL: return "KEYWORD_DEL";
            case token_type::KEYWORD_ELIF: return "KEYWORD_ELIF";
            case token_type::KEYWORD_ELSE: return "KEYWORD_ELSE";
            case token_type::KEYWORD_EXCEPT: return "KEYWORD_EXCEPT";
            case token_type::KEYWORD_FINALLY: return "KEYWORD_FINALLY";
            case token_type::KEYWORD_FOR: return "KEYWORD_FOR";
            case token_type::KEYWORD_FROM: return "KEYWORD_FROM";
            case token_type::KEYWORD_GLOBAL: return "KEYWORD_GLOBAL";
            case token_type::KEYWORD_IF: return "KEYWORD_IF";
            case token_type::KEYWORD_IMPORT: return "KEYWORD_IMPORT";
            case token_type::KEYWORD_IN: return "KEYWORD_IN";
            case token_type::KEYWORD_IS: return "KEYWORD_IS";
            case token_type::KEYWORD_LAMBDA: return "KEYWORD_LAMBDA";
            case token_type::KEYWORD_NONLOCAL: return "KEYWORD_NONLOCAL";
            case token_type::KEYWORD_NOT: return "KEYWORD_NOT";
            case token_type::KEYWORD_OR: return "KEYWORD_OR";
            case token_type::KEYWORD_PASS: return "KEYWORD_PASS";
            case token_type::KEYWORD_RAISE: return "KEYWORD_RAISE";
            case token_type::KEYWORD_RETURN: return "KEYWORD_RETURN";
            case token_type::KEYWORD_TRY: return "KEYWORD_TRY";
            case token_type::KEYWORD_WHILE: return "KEYWORD_WHILE";
            case token_type::KEYWORD_WITH: return "KEYWORD_WITH";
            case token_type::KEYWORD_YIELD: return "KEYWORD_YIELD";
            case token_type::KEYWORD_ENUM: return "KEYWORD_ENUM";
            case token_type::KEYWORD_SELF: return "KEYWORD_SELF";
            case token_type::KEYWORD_INIT: return "KEYWORD_INIT";
            case token_type::IDENTIFIER: return "IDENTIFIER";
            case token_type::INTEGER_LITERAL: return "INTEGER_LITERAL";
            case token_type::FLOAT_LITERAL: return "FLOAT_LITERAL";
            case token_type::STRING_LITERAL: return "STRING_LITERAL";
            case token_type::BYTES_LITERAL: return "BYTES_LITERAL";
            case token_type::PLUS: return "PLUS";
            case token_type::MINUS: return "MINUS";
            case token_type::STAR: return "STAR";
            case token_type::SLASH: return "SLASH";
            case token_type::DOUBLE_SLASH: return "DOUBLE_SLASH";
            case token_type::PERCENT: return "PERCENT";
            case token_type::POWER: return "POWER";
            case token_type::AT: return "AT";
            case token_type::AMPERSAND: return "AMPERSAND";
            case token_type::PIPE: return "PIPE";
            case token_type::CARET: return "CARET";
            case token_type::TILDE: return "TILDE";
            case token_type::LEFT_SHIFT: return "LEFT_SHIFT";
            case token_type::RIGHT_SHIFT: return "RIGHT_SHIFT";
            case token_type::LESS: return "LESS";
            case token_type::GREATER: return "GREATER";
            case token_type::LESS_EQUAL: return "LESS_EQUAL";
            case token_type::GREATER_EQUAL: return "GREATER_EQUAL";
            case token_type::DOUBLE_EQUAL: return "DOUBLE_EQUAL";
            case token_type::NOT_EQUAL: return "NOT_EQUAL";
            case token_type::EQUAL: return "EQUAL";
            case token_type::PLUS_EQUAL: return "PLUS_EQUAL";
            case token_type::MINUS_EQUAL: return "MINUS_EQUAL";
            case token_type::STAR_EQUAL: return "STAR_EQUAL";
            case token_type::SLASH_EQUAL: return "SLASH_EQUAL";
            case token_type::DOUBLE_SLASH_EQUAL: return "DOUBLE_SLASH_EQUAL";
            case token_type::PERCENT_EQUAL: return "PERCENT_EQUAL";
            case token_type::POWER_EQUAL: return "POWER_EQUAL";
            case token_type::AT_EQUAL: return "AT_EQUAL";
            case token_type::AMPERSAND_EQUAL: return "AMPERSAND_EQUAL";
            case token_type::PIPE_EQUAL: return "PIPE_EQUAL";
            case token_type::CARET_EQUAL: return "CARET_EQUAL";
            case token_type::LEFT_SHIFT_EQUAL: return "LEFT_SHIFT_EQUAL";
            case token_type::RIGHT_SHIFT_EQUAL: return "RIGHT_SHIFT_EQUAL";
            case token_type::WALRUS: return "WALRUS";
            case token_type::LPAREN: return "LPAREN";
            case token_type::RPAREN: return "RPAREN";
            case token_type::LBRACKET: return "LBRACKET";
            case token_type::RBRACKET: return "RBRACKET";
            case token_type::LCBRACE: return "LCBRACE";
            case token_type::RCBRACE: return "RCBRACE";
            case token_type::COMMA: return "COMMA";
            case token_type::COLON: return "COLON";
            case token_type::SEMICOLON: return "SEMICOLON";
            case token_type::DOT: return "DOT";
            case token_type::ARROW: return "ARROW";
            case token_type::ELLIPSIS: return "ELLIPSIS";
            case token_type::NEWLINE: return "NEWLINE";
            case token_type::INDENT: return "INDENT";
            case token_type::DEDENT: return "DEDENT";
            case token_type::COMMENT: return "COMMENT";
            case token_type::EOF_TOKEN: return "EOF_TOKEN";
            case token_type::DEFAULT: return "DEFAULT";
            default: return "UNKNOWN";
        }
    } catch (const std::exception& e) {
        return "ERROR";
    }
}

}
