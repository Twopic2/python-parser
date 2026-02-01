#include <stdexcept>
#include <format>

#include "frontend/parser.hpp"

Parser::parser_class::parser_class(Lexical::lexical_class& lexer) : current_pos(0), m_error_count(0) {
    tokens = lexer.tokenize();
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

void Parser::parser_class::parse() {
    auto program = std::make_unique<Ast::Program>();
    program->token = Token::token_class{Token::token_type::DEFAULT, "", 1, 1};

    while (!is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            program->statements.push_back(std::move(stmt));
        }
    }

    ast_tree.set_root(std::move(program));
}

void Parser::parser_class::consume_newline() {
    consume(Token::token_type::COLON);
    consume(Token::token_type::NEWLINE);
    if (!match(Token::token_type::INDENT)) {
        debug_syntax_error();
    }
    consume(Token::token_type::INDENT);
}

void Parser::parser_class::consume_line() {
    consume(Token::token_type::NEWLINE);
    if (!match(Token::token_type::INDENT)) {
        debug_syntax_error();
    }
    consume(Token::token_type::INDENT);
}

Ast::Block Parser::parser_class::parse_block() {
    Ast::Block block;
    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            block.push_back(std::move(stmt));
        }
    }
    consume(Token::token_type::DEDENT);
    return block;
}

Ast::ExprPtr Parser::parser_class::parse_expression_types() {
    switch (current_token().type) {
        case Token::token_type::INTEGER_LITERAL: {
            auto token = current_token();
            consume(Token::token_type::INTEGER_LITERAL);
            return Ast::make_expr(Ast::Literal{Ast::IntegerLiteral{token}});
        }

        case Token::token_type::FLOAT_LITERAL: {
            auto token = current_token();
            consume(Token::token_type::FLOAT_LITERAL);
            return Ast::make_expr(Ast::Literal{Ast::FloatLiteral{token}});
        }

        case Token::token_type::STRING_LITERAL: {
            auto token = current_token();
            consume(Token::token_type::STRING_LITERAL);
            return Ast::make_expr(Ast::Literal{Ast::StringLiteral{token}});
        }

        case Token::token_type::IDENTIFIER: {
            auto token = current_token();
            consume(Token::token_type::IDENTIFIER);
            return Ast::make_expr(Ast::Identifier{token});
        }

        case Token::token_type::LBRACKET:
            return parse_list();

        case Token::token_type::LCBRACE:
            return parse_dict();

        case Token::token_type::LPAREN:
            return parse_call_expr();

        case Token::token_type::KEYWORD_SELF:
            return parse_self();

        default:
            debug_syntax_error();
            return nullptr;
    }
}

Ast::ExprPtr Parser::parser_class::parse_attribute_expr() {
    consume(Token::token_type::DOT);
    auto token = current_token();

    if (!match(Token::token_type::IDENTIFIER)) {
        debug_syntax_error();
    }

    std::string attr = current_token().value;
    consume();

    return Ast::make_expr(Ast::Identifier{token});
}

Ast::ExprPtr Parser::parser_class::parse_call_expr() {
    consume(Token::token_type::LPAREN);
    auto token = current_token();

    std::vector<Ast::ExprPtr> args;

    if (!match(Token::token_type::RPAREN)) {
        args.push_back(parse_term());

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RPAREN)) {
                break;
            }

            args.push_back(parse_term());
        }
    }

    consume();

    Ast::CallExpr call;
    call.callee = nullptr;
    call.args = std::move(args);
    call.token = token;

    return Ast::make_expr(std::move(call));
}

Ast::ExprPtr Parser::parser_class::parse_comparator() {
    auto left = parse_bitwise();

    if (match(Token::token_type::GREATER, Token::token_type::GREATER_EQUAL, Token::token_type::LESS, Token::token_type::LESS_EQUAL)) {
        auto op = current_token();
        consume(current_token().type);

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_comparator();

        return Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_term() {
    auto left = parse_factor();

    while (match(Token::token_type::PLUS, Token::token_type::MINUS)) {
        auto op = current_token();
        consume(current_token().type);

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_factor();

        left = Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_equality() {
    auto left = parse_comparator();

    while (match(Token::token_type::DOUBLE_EQUAL, Token::token_type::NOT_EQUAL)) {
        auto op = current_token();
        consume();

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_comparator();

        left = Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_factor() {
    auto left = parse_power();

    while (match(Token::token_type::STAR, Token::token_type::SLASH, Token::token_type::DOUBLE_SLASH, Token::token_type::PERCENT)) {
        auto op = current_token();
        consume();

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_power();

        left = Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_power() {
    auto left = parse_expression_types();

    if (match(Token::token_type::POWER)) {
        auto op = current_token();
        consume();

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_power();

        return Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_bitwise() {
    auto left = parse_term();

    while (match(Token::token_type::PIPE, Token::token_type::CARET, Token::token_type::AMPERSAND, Token::token_type::LEFT_SHIFT, Token::token_type::RIGHT_SHIFT)) {
        auto op = current_token();
        consume();

        Ast::BinaryOp binary;
        binary.op = op;
        binary.left = std::move(left);
        binary.right = parse_term();

        left = Ast::make_expr(std::move(binary));
    }

    return left;
}

Ast::StmtPtr Parser::parser_class::parse_lambda() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_LAMBDA);

    std::vector<Ast::Parameter> params;

    if (!match(Token::token_type::COLON)) {
        if (match(Token::token_type::IDENTIFIER)) {
            params.push_back(Ast::Parameter{current_token().value, current_token()});
            consume(Token::token_type::IDENTIFIER);
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);
            if (match(Token::token_type::IDENTIFIER)) {
                params.push_back(Ast::Parameter{current_token().value, current_token()});
                consume(Token::token_type::IDENTIFIER);
            }
        }
    }
    consume(Token::token_type::COLON);

    Ast::Block body;
    while (!match(Token::token_type::NEWLINE) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    consume(Token::token_type::NEWLINE);

    Ast::LambdaExpr lambda;
    lambda.params = std::move(params);
    lambda.body = std::move(body);
    lambda.token = token;

    return Ast::make_stmt(std::move(lambda));
}

Ast::StmtPtr Parser::parser_class::parse_statement() {
    switch (current_token().type) {
        case Token::token_type::NEWLINE:
            consume(Token::token_type::NEWLINE);
            return nullptr;

        case Token::token_type::KEYWORD_DEF:
            return parse_function_def();

        case Token::token_type::KEYWORD_CLASS:
            return parse_class();

        case Token::token_type::KEYWORD_IF:
            return parse_if_stmt();

        case Token::token_type::KEYWORD_WHILE:
            return parse_while_stmt();

        case Token::token_type::KEYWORD_FOR:
            return parse_for_stmt();

        case Token::token_type::KEYWORD_MATCH:
            return parse_match_stmt();

        case Token::token_type::KEYWORD_RETURN:
            return parse_return_stmt();

        case Token::token_type::KEYWORD_PASS:
            return parse_pass();

        case Token::token_type::KEYWORD_TRY:
            return parse_try();

        case Token::token_type::KEYWORD_BREAK:
            return parse_break();

        case Token::token_type::KEYWORD_CONTINUE:
            return parse_continue();

        case Token::token_type::KEYWORD_LAMBDA:
            return parse_lambda();

        default: {
            auto expr = parse_assignment();
            Ast::ExpressionStmt expr_stmt;
            expr_stmt.expr = std::move(expr);
            return Ast::make_stmt(std::move(expr_stmt));
        }
    }
}

Ast::ExprPtr Parser::parser_class::parse_self() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_SELF);

    Ast::SelfExpr self;
    self.token = token;

    if (match(Token::token_type::DOT)) {
        consume(Token::token_type::DOT);
        if (match(Token::token_type::IDENTIFIER)) {
            self.attr = current_token().value;
            consume();
        }
    }

    return Ast::make_expr(std::move(self));
}

Ast::StmtPtr Parser::parser_class::parse_try() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_TRY);

    consume_newline();
    auto try_body = parse_block();

    std::optional<Ast::ExceptClause> except_clause;
    if (match(Token::token_type::KEYWORD_EXCEPT)) {
        auto except_token = current_token();
        consume(Token::token_type::KEYWORD_EXCEPT);
        consume_newline();
        auto except_body = parse_block();
        except_clause = Ast::ExceptClause{std::move(except_body), except_token};
    }

    std::optional<Ast::Block> finally_body;
    if (match(Token::token_type::KEYWORD_FINALLY)) {
        consume(Token::token_type::KEYWORD_FINALLY);
        consume_newline();
        finally_body = parse_block();
    }

    std::optional<Ast::Block> else_body;
    if (match(Token::token_type::KEYWORD_ELSE)) {
        consume(Token::token_type::KEYWORD_ELSE);
        consume_newline();
        else_body = parse_block();
    }

    Ast::TryStmt try_stmt;
    try_stmt.body = std::move(try_body);
    try_stmt.except = std::move(except_clause);
    try_stmt.finally_body = std::move(finally_body);
    try_stmt.else_body = std::move(else_body);
    try_stmt.token = token;

    return Ast::make_stmt(std::move(try_stmt));
}

Ast::StmtPtr Parser::parser_class::parse_pass() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_PASS);
    return Ast::make_stmt(Ast::PassStmt{token});
}

Ast::ExprPtr Parser::parser_class::parse_assignment() {
    auto left = parse_equality();

    if (match(Token::token_type::EQUAL)) {
        auto token = current_token();
        consume(Token::token_type::EQUAL);

        Ast::Assignment assign;
        assign.target = std::move(left);
        assign.value = parse_assignment();
        assign.token = token;

        return Ast::make_expr(Ast::Identifier{token});
    } else if (match(Token::token_type::PLUS_EQUAL) ||
               match(Token::token_type::MINUS_EQUAL) ||
               match(Token::token_type::STAR_EQUAL) ||
               match(Token::token_type::SLASH_EQUAL)) {
        auto token = current_token();
        consume(current_token().type);

        Ast::AugmentedAssignment aug_assign;
        aug_assign.target = std::move(left);
        aug_assign.value = parse_assignment();
        aug_assign.token = token;

        return Ast::make_expr(Ast::Identifier{token});
    }

    return left;
}

Ast::StmtPtr Parser::parser_class::parse_return_stmt() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_RETURN);

    std::optional<Ast::ExprPtr> value;
    if (!is_at_end() && !match(Token::token_type::NEWLINE)) {
        value = parse_expression_types();
    }

    Ast::ReturnStmt ret;
    ret.value = std::move(value);
    ret.token = token;

    return Ast::make_stmt(std::move(ret));
}

Ast::StmtPtr Parser::parser_class::parse_function_def() {
    consume(Token::token_type::KEYWORD_DEF);

    auto token = current_token();
    std::string name = current_token().value;

    if (!match(Token::token_type::IDENTIFIER)) {
        debug_syntax_error();
    }
    consume(Token::token_type::IDENTIFIER);

    consume(Token::token_type::LPAREN);

    std::vector<Ast::Parameter> params;
    if (!match(Token::token_type::RPAREN)) {
        if (match(Token::token_type::IDENTIFIER)) {
            params.push_back(Ast::Parameter{current_token().value, current_token()});
            consume(Token::token_type::IDENTIFIER);
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);
            if (match(Token::token_type::IDENTIFIER)) {
                params.push_back(Ast::Parameter{current_token().value, current_token()});
                consume(Token::token_type::IDENTIFIER);
            }
        }
    }

    consume(Token::token_type::RPAREN);
    consume(Token::token_type::COLON);
    consume_line();

    auto body = parse_block();

    Ast::FunctionDef func;
    func.name = name;
    func.params = std::move(params);
    func.body = std::move(body);
    func.token = token;

    return Ast::make_stmt(std::move(func));
}

Ast::StmtPtr Parser::parser_class::parse_class() {
    consume(Token::token_type::KEYWORD_CLASS);

    auto token = current_token();
    std::string name = current_token().value;
    consume(Token::token_type::IDENTIFIER);

    consume(Token::token_type::COLON);
    consume_line();

    Ast::Block body;
    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        if (match(Token::token_type::KEYWORD_DEF)) {
            auto method = parse_method();
            if (method) {
                body.push_back(std::move(method));
            }
        } else {
            auto stmt = parse_statement();
            if (stmt) {
                body.push_back(std::move(stmt));
            }
        }
    }
    consume(Token::token_type::DEDENT);

    Ast::ClassDef cls;
    cls.name = name;
    cls.body = std::move(body);
    cls.token = token;

    return Ast::make_stmt(std::move(cls));
}

Ast::StmtPtr Parser::parser_class::parse_method() {
    consume(Token::token_type::KEYWORD_DEF);

    auto token = current_token();
    std::string name;

    if (!match(Token::token_type::IDENTIFIER) && !match(Token::token_type::KEYWORD_INIT)) {
        debug_syntax_error();
    }

    if (match(Token::token_type::KEYWORD_INIT)) {
        name = "__init__";
        consume(Token::token_type::KEYWORD_INIT);
    } else {
        name = current_token().value;
        consume(Token::token_type::IDENTIFIER);
    }

    consume(Token::token_type::LPAREN);

    std::vector<Ast::Parameter> params;
    if (!match(Token::token_type::KEYWORD_SELF)) {
        debug_syntax_error();
    }

    params.push_back(Ast::Parameter{current_token().value, current_token()});
    consume(Token::token_type::KEYWORD_SELF);

    while (match(Token::token_type::COMMA)) {
        consume(Token::token_type::COMMA);
        if (match(Token::token_type::IDENTIFIER)) {
            params.push_back(Ast::Parameter{current_token().value, current_token()});
            consume(Token::token_type::IDENTIFIER);
        }
    }

    consume(Token::token_type::RPAREN);
    consume(Token::token_type::COLON);
    consume_line();

    auto body = parse_block();

    Ast::MethodDef method;
    method.name = name;
    method.params = std::move(params);
    method.body = std::move(body);
    method.token = token;

    return Ast::make_stmt(std::move(method));
}

Ast::StmtPtr Parser::parser_class::parse_break() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_BREAK);
    return Ast::make_stmt(Ast::BreakStmt{token});
}

Ast::StmtPtr Parser::parser_class::parse_continue() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_CONTINUE);
    return Ast::make_stmt(Ast::ContinueStmt{token});
}

Ast::StmtPtr Parser::parser_class::parse_if_stmt() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_IF);

    auto condition = parse_equality();

    consume_newline();
    auto if_body = parse_block();

    std::vector<Ast::ElifClause> elifs;
    while (match(Token::token_type::KEYWORD_ELIF)) {
        auto elif_token = current_token();
        consume(Token::token_type::KEYWORD_ELIF);

        auto elif_cond = parse_expression_types();
        consume_newline();
        auto elif_body = parse_block();

        elifs.push_back(Ast::ElifClause{std::move(elif_cond), std::move(elif_body), elif_token});
    }

    std::optional<Ast::Block> else_body;
    if (match(Token::token_type::KEYWORD_ELSE)) {
        consume(Token::token_type::KEYWORD_ELSE);
        consume_newline();
        else_body = parse_block();
    }

    Ast::IfStmt if_stmt;
    if_stmt.condition = std::move(condition);
    if_stmt.body = std::move(if_body);
    if_stmt.elifs = std::move(elifs);
    if_stmt.else_body = std::move(else_body);
    if_stmt.token = token;

    return Ast::make_stmt(std::move(if_stmt));
}

Ast::StmtPtr Parser::parser_class::parse_while_stmt() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_WHILE);

    auto condition = parse_equality();

    consume_newline();
    auto body = parse_block();

    Ast::WhileStmt while_stmt;
    while_stmt.condition = std::move(condition);
    while_stmt.body = std::move(body);
    while_stmt.token = token;

    return Ast::make_stmt(std::move(while_stmt));
}

Ast::StmtPtr Parser::parser_class::parse_for_stmt() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_FOR);

    std::string var_name;
    if (match(Token::token_type::IDENTIFIER)) {
        var_name = current_token().value;
        consume(Token::token_type::IDENTIFIER);
    }

    consume(Token::token_type::KEYWORD_IN);

    auto iterable = parse_expression_types();

    consume_newline();
    auto body = parse_block();

    Ast::ForStmt for_stmt;
    for_stmt.var_name = var_name;
    for_stmt.iterable = std::move(iterable);
    for_stmt.body = std::move(body);
    for_stmt.token = token;

    return Ast::make_stmt(std::move(for_stmt));
}

Ast::StmtPtr Parser::parser_class::parse_match_stmt() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_MATCH);

    auto subject = parse_expression_types();

    consume_newline();

    std::vector<Ast::CaseClause> cases;
    while (match(Token::token_type::KEYWORD_CASE) && !is_at_end()) {
        cases.push_back(parse_case());
    }

    consume(Token::token_type::DEDENT);

    Ast::MatchStmt match_stmt;
    match_stmt.subject = std::move(subject);
    match_stmt.cases = std::move(cases);
    match_stmt.token = token;

    return Ast::make_stmt(std::move(match_stmt));
}

Ast::CaseClause Parser::parser_class::parse_case() {
    auto token = current_token();
    consume(Token::token_type::KEYWORD_CASE);

    auto pattern = parse_expression_types();

    consume_newline();
    auto body = parse_block();

    return Ast::CaseClause{std::move(pattern), std::move(body), token};
}

Ast::ExprPtr Parser::parser_class::parse_list() {
    auto token = current_token();
    consume(Token::token_type::LBRACKET);

    std::vector<Ast::ExprPtr> elements;

    if (!match(Token::token_type::RBRACKET)) {
        elements.push_back(parse_expression_types());

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RBRACKET)) {
                break;
            }

            elements.push_back(parse_expression_types());
        }
    }

    consume(Token::token_type::RBRACKET);

    Ast::ListExpr list;
    list.elements = std::move(elements);
    list.token = token;

    return Ast::make_expr(std::move(list));
}

Ast::ExprPtr Parser::parser_class::parse_dict() {
    auto token = current_token();
    consume(Token::token_type::LCBRACE);

    std::vector<Ast::ExprPtr> keys;
    std::vector<Ast::ExprPtr> values;

    if (!match(Token::token_type::RCBRACE)) {
        keys.push_back(parse_expression_types());

        if (match(Token::token_type::COLON)) {
            consume(Token::token_type::COLON);
            values.push_back(parse_expression_types());
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RCBRACE)) {
                break;
            }

            keys.push_back(parse_expression_types());

            if (match(Token::token_type::COLON)) {
                consume(Token::token_type::COLON);
                values.push_back(parse_expression_types());
            }
        }
    }

    consume(Token::token_type::RCBRACE);

    Ast::DictExpr dict;
    dict.keys = std::move(keys);
    dict.values = std::move(values);
    dict.token = token;

    return Ast::make_expr(std::move(dict));
}
