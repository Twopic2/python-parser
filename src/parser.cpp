#include "parser.hpp"
#include <iostream>
#include <stdexcept>

/* 

Recursive descent order

parse_program()        - Top level
parse_statement()      - Statement dispatcher
parse_assignment()     - Assignments
parse_expression()     - Expression entry point
parse_comparison()     - Comparison operators
parse_term()           - Addition / subtraction
parse_factor()         - Multiplication / division
parse_unary()          - Unary operators
parse_primary()        - Literals and identifiers
parse_function_def()   - Function definitions
parse_return_stmt()    - Return statements
parse_if_stmt()        - If statements

*/

Parser::parser_class::parser_class(Lexical::lexical_class& lexer) : current_pos(0) {
    tokens = lexer.tokenize();

    std::cerr << "Parser constructor called" << std::endl;
}

bool Parser::parser_class::consume(const Token::token_type& type) {
    if (match(type)) {
        current_pos++;
        return true;
    }

    return false;
}

Token::token_class& Parser::parser_class::current_token() {
    return tokens[current_pos];
}

Ast::ast_class& Parser::parser_class::get_ast() {
    return ast_tree;
}

bool Parser::parser_class::match(const Token::token_type& type) {
    return type == current_token().type;
}

bool Parser::parser_class::is_at_end() {
    return current_pos >= tokens.size() || current_token().type == Token::token_type::EOF_TOKEN;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse() {
    auto root = std::make_unique<Ast::ast_node>(Ast::node_type::PROGRAM);

   try {
        while (!is_at_end()) {
            auto stmt = parse_statement();
            if (stmt) {
                root->add_child(std::move(stmt));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        throw;
    }

    ast_tree.set_root(std::move(root));

    return nullptr;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_expression() {
    if (match(Token::token_type::INTEGER_LITERAL)) {
        auto node = std::make_unique<Ast::ast_node>(Ast::node_type::INTEGER_LITERAL,
                                                     current_token().value,
                                                     current_token().line,
                                                     current_token().column);
        consume(Token::token_type::INTEGER_LITERAL);
        return node;
    }

    if (match(Token::token_type::FLOAT_LITERAL)) {
        auto node = std::make_unique<Ast::ast_node>(Ast::node_type::FLOAT_LITERAL,
                                                     current_token().value,
                                                     current_token().line,
                                                     current_token().column);
        consume(Token::token_type::FLOAT_LITERAL);
        return node;
    }

    if (match(Token::token_type::STRING_LITERAL)) {
        auto node = std::make_unique<Ast::ast_node>(Ast::node_type::STRING_LITERAL,
                                                     current_token().value,
                                                     current_token().line,
                                                     current_token().column);
        consume(Token::token_type::STRING_LITERAL);
        return node;
    }

    if (match(Token::token_type::IDENTIFIER)) {
        auto node = std::make_unique<Ast::ast_node>(Ast::node_type::IDENTIFIER,
                                                     current_token().value,
                                                     current_token().line,
                                                     current_token().column);
        consume(Token::token_type::IDENTIFIER);
        return node;
    }

    if (match(Token::token_type::LPAREN)) {
        consume(Token::token_type::LPAREN);
        auto expr = parse_operator();
        consume(Token::token_type::RPAREN);
        return expr;
    }

    throw std::runtime_error("Unexpected token: " + current_token().value +
                           " at line " + std::to_string(current_token().line));
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_operator() {
    auto left = parse_expression();

    while (match(Token::token_type::PLUS) || match(Token::token_type::MINUS) ||
           match(Token::token_type::STAR) || match(Token::token_type::SLASH) ||
           match(Token::token_type::DOUBLE_SLASH) || match(Token::token_type::PERCENT) ||
           match(Token::token_type::POWER)) {

        auto op_node = std::make_unique<Ast::ast_node>(Ast::node_type::BINARY_OP,
                                                        current_token().value,
                                                        current_token().line,
                                                        current_token().column);
        current_pos++;

        op_node->add_child(std::move(left));
        op_node->add_child(parse_expression());

        left = std::move(op_node);
    }

    return left;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_statement() {
    if (match(Token::token_type::NEWLINE)) {
        consume(Token::token_type::NEWLINE);
        return nullptr;
    }

    if (match(Token::token_type::KEYWORD_DEF)) {
        return parse_function_def();
    }

    if (match(Token::token_type::KEYWORD_CLASS)) {
        return parse_class_def();
    }

    if (match(Token::token_type::KEYWORD_IF)) {
        return parse_if_stmt();
    }

    if (match(Token::token_type::KEYWORD_WHILE)) {
        return parse_while_stmt();
    }

    if (match(Token::token_type::KEYWORD_FOR)) {
        return parse_for_stmt();
    }

    if (match(Token::token_type::KEYWORD_MATCH)) {
        return parse_match_stmt();
    }

    if (match(Token::token_type::KEYWORD_CASE)) {
        return parse_case();
    }

    if (match(Token::token_type::KEYWORD_RETURN)) {
        return parse_return_stmt();
    }

    return parse_assignment();
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_assignment() {
    auto expr = parse_operator();

    if (match(Token::token_type::EQUAL)) {
        auto assign_node = std::make_unique<Ast::ast_node>(Ast::node_type::ASSIGNMENT,
                                                            "=",
                                                            current_token().line,
                                                            current_token().column);
        consume(Token::token_type::EQUAL);

        assign_node->add_child(std::move(expr));
        assign_node->add_child(parse_operator());

        return assign_node;
    }

    if (match(Token::token_type::PLUS_EQUAL) || match(Token::token_type::MINUS_EQUAL) ||
        match(Token::token_type::STAR_EQUAL) || match(Token::token_type::SLASH_EQUAL)) {

        auto aug_node = std::make_unique<Ast::ast_node>(Ast::node_type::AUGMENTED_ASSIGNMENT,
                                                         current_token().value,
                                                         current_token().line,
                                                         current_token().column);
        current_pos++;

        aug_node->add_child(std::move(expr));
        aug_node->add_child(parse_operator());

        return aug_node;
    }

    auto expr_stmt = std::make_unique<Ast::ast_node>(Ast::node_type::EXPRESSION_STMT,
                                                      "",
                                                      expr->token_m.line,
                                                      expr->token_m.column);
    expr_stmt->add_child(std::move(expr));
    return expr_stmt;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_return_stmt() {
    auto ret_node = std::make_unique<Ast::ast_node>(Ast::node_type::RETURN_STMT,
                                                     "",
                                                     current_token().line,
                                                     current_token().column);
    consume(Token::token_type::KEYWORD_RETURN);

    if (!is_at_end() && !match(Token::token_type::NEWLINE)) {
        ret_node->add_child(parse_expression());
    }

    return ret_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_function_def() {
    consume(Token::token_type::KEYWORD_DEF);

    if (!match(Token::token_type::IDENTIFIER)) {
        throw std::runtime_error("Expected function name at line " +
                               std::to_string(current_token().line));
    }

    auto func_node = std::make_unique<Ast::ast_node>(Ast::node_type::FUNCTION_DEF,
                                                      current_token().value,
                                                      current_token().line,
                                                      current_token().column);
    consume(Token::token_type::IDENTIFIER);

    consume(Token::token_type::LPAREN);

    auto param_list = std::make_unique<Ast::ast_node>(Ast::node_type::PARAMETER_LIST,
                                                       "",
                                                       current_token().line,
                                                       current_token().column);

    if (!match(Token::token_type::RPAREN)) {
        if (match(Token::token_type::IDENTIFIER)) {
            auto param = std::make_unique<Ast::ast_node>(Ast::node_type::PARAMETER,
                                                          current_token().value,
                                                          current_token().line,
                                                          current_token().column);
            consume(Token::token_type::IDENTIFIER);
            param_list->add_child(std::move(param));
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);
            if (match(Token::token_type::IDENTIFIER)) {
                auto param = std::make_unique<Ast::ast_node>(Ast::node_type::PARAMETER,
                                                              current_token().value,
                                                              current_token().line,
                                                              current_token().column);
                consume(Token::token_type::IDENTIFIER);
                param_list->add_child(std::move(param));
            }
        }
    }

    consume(Token::token_type::RPAREN);
    consume(Token::token_type::COLON);

    func_node->add_child(std::move(param_list));

    auto body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                 "",
                                                 current_token().line,
                                                 current_token().column);

    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    func_node->add_child(std::move(body));

    return func_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_class_def() {
    consume(Token::token_type::KEYWORD_CLASS);

    if (!match(Token::token_type::IDENTIFIER)) {
        throw std::runtime_error("Expected class name at line " +
                               std::to_string(current_token().line));
    }

    auto class_node = std::make_unique<Ast::ast_node>(Ast::node_type::CLASS_DEF,
                                                       current_token().value,
                                                       current_token().line,
                                                       current_token().column);
    consume(Token::token_type::IDENTIFIER);

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    auto body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                 "",
                                                 current_token().line,
                                                 current_token().column);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    class_node->add_child(std::move(body));

    return class_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_if_stmt() {
    auto if_node = std::make_unique<Ast::ast_node>(Ast::node_type::IF_STMT,
                                                    "",
                                                    current_token().line,
                                                    current_token().column);
    consume(Token::token_type::KEYWORD_IF);

    if_node->add_child(parse_expression());

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    auto if_body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                    "",
                                                    current_token().line,
                                                    current_token().column);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            if_body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    if_node->add_child(std::move(if_body));

    while (match(Token::token_type::KEYWORD_ELIF)) {
        auto elif_node = std::make_unique<Ast::ast_node>(Ast::node_type::ELIF_STMT,
                                                          "",
                                                          current_token().line,
                                                          current_token().column);
        consume(Token::token_type::KEYWORD_ELIF);

        elif_node->add_child(parse_expression());

        consume(Token::token_type::COLON);
        consume(Token::token_type::NEWLINE);
        consume(Token::token_type::INDENT);

        auto elif_body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                          "",
                                                          current_token().line,
                                                          current_token().column);

        while (!match(Token::token_type::DEDENT) && !is_at_end()) {
            auto stmt = parse_statement();
            if (stmt) {
                elif_body->add_child(std::move(stmt));
            }
        }

        consume(Token::token_type::DEDENT);
        elif_node->add_child(std::move(elif_body));

        if_node->add_child(std::move(elif_node));
    }

    if (match(Token::token_type::KEYWORD_ELSE)) {
        auto else_node = std::make_unique<Ast::ast_node>(Ast::node_type::ELSE_STMT,
                                                          "",
                                                          current_token().line,
                                                          current_token().column);
        consume(Token::token_type::KEYWORD_ELSE);

        consume(Token::token_type::COLON);
        consume(Token::token_type::NEWLINE);
        consume(Token::token_type::INDENT);

        auto else_body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                          "",
                                                          current_token().line,
                                                          current_token().column);

        while (!match(Token::token_type::DEDENT) && !is_at_end()) {
            auto stmt = parse_statement();
            if (stmt) {
                else_body->add_child(std::move(stmt));
            }
        }

        consume(Token::token_type::DEDENT);
        else_node->add_child(std::move(else_body));

        if_node->add_child(std::move(else_node));
    }

    return if_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_while_stmt() {
    auto while_node = std::make_unique<Ast::ast_node>(Ast::node_type::WHILE_STMT,
                                                       "",
                                                       current_token().line,
                                                       current_token().column);
    consume(Token::token_type::KEYWORD_WHILE);

    while_node->add_child(parse_expression());

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    auto body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                 "",
                                                 current_token().line,
                                                 current_token().column);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    while_node->add_child(std::move(body));

    return while_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_for_stmt() {
    auto for_node = std::make_unique<Ast::ast_node>(Ast::node_type::FOR_STMT,
                                                     "",
                                                     current_token().line,
                                                     current_token().column);
    consume(Token::token_type::KEYWORD_FOR);

    if (match(Token::token_type::IDENTIFIER)) {
        auto var_node = std::make_unique<Ast::ast_node>(Ast::node_type::IDENTIFIER,
                                                         current_token().value,
                                                         current_token().line,
                                                         current_token().column);
        consume(Token::token_type::IDENTIFIER);
        for_node->add_child(std::move(var_node));
    }

    consume(Token::token_type::KEYWORD_IN);

    for_node->add_child(parse_expression());

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    auto body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                 "",
                                                 current_token().line,
                                                 current_token().column);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    for_node->add_child(std::move(body));

    return for_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_match_stmt() {
    auto match_node = std::make_unique<Ast::ast_node>(Ast::node_type::MATCH_STMT,
                                                       "",
                                                       current_token().line,
                                                       current_token().column);
    consume(Token::token_type::KEYWORD_MATCH);

    match_node->add_child(parse_expression());

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    while (match(Token::token_type::KEYWORD_CASE) && !is_at_end()) {
        auto case_stmt = parse_case();
        if (case_stmt) {
            match_node->add_child(std::move(case_stmt));
        }
    }

    consume(Token::token_type::DEDENT);

    return match_node;
}

std::unique_ptr<Ast::ast_node> Parser::parser_class::parse_case() {
    auto case_node = std::make_unique<Ast::ast_node>(Ast::node_type::CASE_STMT,
                                                      "",
                                                      current_token().line,
                                                      current_token().column);
    consume(Token::token_type::KEYWORD_CASE);

    case_node->add_child(parse_expression());

    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    consume(Token::token_type::INDENT);

    auto case_body = std::make_unique<Ast::ast_node>(Ast::node_type::BLOCK,
                                                          "",
                                                          current_token().line,
                                                          current_token().column);

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            case_body->add_child(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    case_node->add_child(std::move(case_body));

    return case_node;
}
