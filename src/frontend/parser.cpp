#include <stdexcept>
#include <format>

#include "frontend/parser.hpp"

/*
Recursive descent order

parse_program()        - Top level
parse_statement()      - Statement dispatcher
parse_assignment()     - Assignments
parse_equality()       - Equality (==, !=)
parse_comparator()     - Comparison (<, >, <=, >=)
parse_bitwise()        - Bitwise ops (|, ^, &, <<, >>)
parse_term()           - Addition / subtraction
parse_factor()         - Multiplication / division
parse_power()          - Exponentiation
parse_expression()     - Literals, identifier
*/

Parser::parser_class::parser_class(Lexical::lexical_class& lexer) : current_pos(0),  m_previous_pos(0) {
    tokens = lexer.tokenize();
}

Token::token_class& Parser::parser_class::current_token() {
    return tokens[current_pos];
}

Token::token_class& Parser::parser_class::previous_token() {
    return tokens[m_previous_pos];
}

bool Parser::parser_class::match(const Token::token_type& type) {
    return type == current_token().type;
}

bool Parser::parser_class::is_at_end() {
    return current_pos >= tokens.size() || current_token().type == Token::token_type::EOF_TOKEN;
}

Ast::Program Parser::parser_class::parse() {
    Ast::Program program;

    while (!is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            program.statements.push_back(std::move(stmt));
        }
    }

    return program;
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
    block.token = current_token();

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            block.statements.push_back(std::move(stmt));
        }
    }

    consume(Token::token_type::DEDENT);
    return block;
}

Ast::ExprPtr Parser::parser_class::parse_expression_types() {
    Ast::ExprPtr expr;

    switch (current_token().type) {
        case Token::token_type::INTEGER_LITERAL: {
            Ast::IntegerLiteral lit{current_token()};
            expr = std::make_unique<Ast::ExprNode>(Ast::ExprNode{lit});
            consume(Token::token_type::INTEGER_LITERAL);
            break;
        }

        case Token::token_type::FLOAT_LITERAL: {
            Ast::FloatLiteral lit{current_token()};
            expr = std::make_unique<Ast::ExprNode>(Ast::ExprNode{lit});
            consume(Token::token_type::FLOAT_LITERAL);
            break;
        }

        case Token::token_type::STRING_LITERAL: {
            Ast::StringLiteral lit{current_token()};
            expr = std::make_unique<Ast::ExprNode>(Ast::ExprNode{lit});
            consume(Token::token_type::STRING_LITERAL);
            break;
        }

        case Token::token_type::IDENTIFIER: {
            Ast::Identifier id{current_token()};
            auto id_expr = std::make_unique<Ast::ExprNode>(Ast::ExprNode{id});
            consume(Token::token_type::IDENTIFIER);

            if (match(Token::token_type::LPAREN)) {
                if (valid_constructor) {
                    expr = parse_constructor_call(std::move(id_expr));
                    valid_constructor = false;
                } else {
                    expr = parse_call_expr(std::move(id_expr));
                }
            } else {
                expr = std::move(id_expr);
            }

            break;
        }

        case Token::token_type::LBRACKET:
            expr = parse_list();
            break;

        case Token::token_type::LCBRACE:
            expr = parse_dict();
            break;

        case Token::token_type::LPAREN: {
            consume(Token::token_type::LPAREN);
            expr = parse_equality();
            consume(Token::token_type::RPAREN);
            break;
        }

        case Token::token_type::KEYWORD_SELF:
            expr = parse_self();
            break;

        default:
            debug_syntax_error();
    }

    return expr;
}

Ast::ExprPtr Parser::parser_class::parse_attribute_expr() {
    Ast::Identifier inst_var{previous_token()};
    
    consume(Token::token_type::DOT);

    if (!match(Token::token_type::IDENTIFIER)) {
        debug_syntax_error();
    }


    Ast::Identifier attr{current_token()};
    consume();

    Ast::AttributeExpr attr_expr{current_token(), inst_var, attr};
    auto attr_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(attr_expr)});
    return attr_node;
}

Ast::ExprPtr Parser::parser_class::parse_call_expr(Ast::ExprPtr callee) {
    Token::token_class token = current_token();
    consume(Token::token_type::LPAREN);

    Ast::CallExpr call{token, std::move(callee), {}};

    if (!match(Token::token_type::RPAREN)) {
        call.arguments.push_back(parse_term());

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RPAREN)) {
                break;
            }

            call.arguments.push_back(parse_term());
        }
    }

    consume();
    auto call_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(call)});
    return call_node;
}

Ast::ExprPtr Parser::parser_class::parse_constructor_call(Ast::ExprPtr constructor) {
    Token::token_class token = current_token();
    consume(Token::token_type::LPAREN);

    Ast::ConstructorCallExpr con{token, std::move(constructor), {}};

    if (!match(Token::token_type::RPAREN)) {
        con.arguments.push_back(parse_term());

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RPAREN)) {
                break;
            }

            con.arguments.push_back(parse_term());
        }
    }

    consume();
    auto con_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(con)});
    return con_node;
}

Ast::ExprPtr Parser::parser_class::parse_comparator() {
    auto left = parse_bitwise();

    if (match(Token::token_type::GREATER, Token::token_type::GREATER_EQUAL, Token::token_type::LESS, Token::token_type::LESS_EQUAL)) {
        Token::token_class op = current_token();
        consume(current_token().type);

        Ast::ComparisonOp comp{op, std::move(left), parse_comparator()};
        auto comp_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(comp)});
        return comp_node;
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_term() {
    auto left = parse_factor();

    while (match(Token::token_type::PLUS, Token::token_type::MINUS)) {
        Token::token_class op = current_token();
        consume(current_token().type);

        Ast::TermOp term{op, std::move(left), parse_factor()};
        left = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(term)});
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_equality() {
    auto left = parse_comparator();

    while (match(Token::token_type::DOUBLE_EQUAL, Token::token_type::NOT_EQUAL)) {
        Token::token_class op = current_token();
        consume();

        Ast::EqualityOp eq{op, std::move(left), parse_comparator()};
        left = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(eq)});
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_factor() {
    auto left = parse_power();

    while (match(Token::token_type::STAR, Token::token_type::SLASH, Token::token_type::DOUBLE_SLASH, Token::token_type::PERCENT)) {
        Token::token_class op = current_token();
        consume();

        Ast::FactorOp factor{op, std::move(left), parse_power()};
        left = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(factor)});
    }

    return left;
}

Ast::ExprPtr Parser::parser_class::parse_power() {
    auto base = parse_expression_types();

    if (match(Token::token_type::POWER)) {
        Token::token_class op = current_token();
        consume();

        Ast::PowerOp power{op, std::move(base), parse_power()};
        auto power_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(power)});
        return power_node;
    }

    return base;
}

Ast::ExprPtr Parser::parser_class::parse_bitwise() {
    auto left = parse_term();

    while (match(Token::token_type::PIPE, Token::token_type::CARET, Token::token_type::AMPERSAND, Token::token_type::LEFT_SHIFT, Token::token_type::RIGHT_SHIFT)) {
        Token::token_class op = current_token();
        consume();

        Ast::BitwiseOp bitwise{op, std::move(left), parse_term()};
        left = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(bitwise)});
    }

    return left;
}

Ast::StmtPtr Parser::parser_class::parse_lambda() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_LAMBDA);

    Ast::ParameterList params;

    if (!match(Token::token_type::COLON)) {
        if (match(Token::token_type::IDENTIFIER)) {
            params.params.push_back(Ast::Parameter{current_token()});
            consume(Token::token_type::IDENTIFIER);
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);
            if (match(Token::token_type::IDENTIFIER)) {
                params.params.push_back(Ast::Parameter{current_token()});
                consume(Token::token_type::IDENTIFIER);
            }
        }
    }
    consume(Token::token_type::COLON);

    std::vector<Ast::StmtPtr> body;
    while (!match(Token::token_type::NEWLINE) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    consume(Token::token_type::NEWLINE);

    Ast::LambdaStmt lambda{token, std::move(params), std::move(body)};
    auto lambda_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(lambda)});
    return lambda_node;
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

        case Token::token_type::KEYWORD_CASE:
            return parse_case();

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
            Token::token_class token = current_token();
            return parse_expression_stmt(token); 
        }
    }
}

Ast::StmtPtr Parser::parser_class::parse_expression_stmt(const auto& token) {
    auto expr = parse_assignment();
    if (expr) {
        Ast::ExpressionStmt expr_stmt{token, std::move(expr)};
        auto expr_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(expr_stmt)});
        return expr_node;
    }

    return {};
}

Ast::ExprPtr Parser::parser_class::parse_self() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_SELF);

    std::optional<Ast::Identifier> attr = std::nullopt;
    if (match(Token::token_type::DOT)) {
        consume(Token::token_type::DOT);
        if (!match(Token::token_type::IDENTIFIER)) {
            debug_syntax_error();
        }
        attr = Ast::Identifier{current_token()};
        consume();
    }

    Ast::SelfExpr self{token, attr};
    auto self_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(self)});
    return self_node;
}

Ast::StmtPtr Parser::parser_class::parse_try() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_TRY);

    consume_newline();
    Ast::Block try_body = parse_block();

    std::optional<Ast::ExceptStmt> except_branch = std::nullopt;
    if (match(Token::token_type::KEYWORD_EXCEPT)) {
        Token::token_class except_token = current_token();
        consume(Token::token_type::KEYWORD_EXCEPT);

        consume_newline();
        Ast::Block except_body = parse_block();

        except_branch = Ast::ExceptStmt{except_token, std::move(except_body)};
    }

    std::optional<Ast::FinallyStmt> finally_branch = std::nullopt;
    if (match(Token::token_type::KEYWORD_FINALLY)) {
        Token::token_class finally_token = current_token();
        consume(Token::token_type::KEYWORD_FINALLY);

        consume_newline();
        Ast::Block finally_body = parse_block();

        finally_branch = Ast::FinallyStmt{finally_token, std::move(finally_body)};
    }

    std::optional<Ast::ElseStmt> else_branch = std::nullopt;
    if (match(Token::token_type::KEYWORD_ELSE)) {
        Token::token_class else_token = current_token();
        consume(Token::token_type::KEYWORD_ELSE);

        consume_newline();
        Ast::Block else_body = parse_block();

        else_branch = Ast::ElseStmt{else_token, std::move(else_body)};
    }

    Ast::TryStmt try_stmt{token, std::move(try_body), std::move(except_branch), std::move(finally_branch), std::move(else_branch)};
    auto try_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(try_stmt)});
    return try_node;
}

Ast::StmtPtr Parser::parser_class::parse_pass() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_PASS);

    Ast::PassStmt pass{token};
    auto pass_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(pass)});
    return pass_node;
}

Ast::ExprPtr Parser::parser_class::parse_assignment() {
    auto left = parse_equality();

    if (match(Token::token_type::EQUAL)) {
        Token::token_class token = current_token();
        consume(Token::token_type::EQUAL);

        Ast::AssignmentOp assign{token, std::move(left), parse_assignment()};
        auto assign_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(assign)});
        return assign_node;
    } else if (match(Token::token_type::PLUS_EQUAL) ||
               match(Token::token_type::MINUS_EQUAL) ||
               match(Token::token_type::STAR_EQUAL) ||
               match(Token::token_type::SLASH_EQUAL)) {
        Token::token_class op = current_token();
        consume(current_token().type);

        Ast::AugmentedAssignmentOp aug_assign{op, std::move(left), parse_assignment()};
        auto aug_assign_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(aug_assign)});
        return aug_assign_node;
    }

    return left;
}

Ast::StmtPtr Parser::parser_class::parse_return_stmt() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_RETURN);

    std::optional<Ast::ExprPtr> value = std::nullopt;
    if (!is_at_end() && !match(Token::token_type::NEWLINE)) {
        value = parse_expression_types();
    }

    Ast::ReturnStmt ret{token, std::move(value)};
    auto ret_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(ret)});
    return ret_node;
}

Ast::StmtPtr Parser::parser_class::parse_function_def() {
    consume(Token::token_type::KEYWORD_DEF);

    Token::token_class token = current_token();
    if (!match(Token::token_type::IDENTIFIER)) {
        debug_syntax_error();
    }

    consume(Token::token_type::IDENTIFIER);
    consume(Token::token_type::LPAREN);

    Ast::ParameterList params;
    if (!match(Token::token_type::RPAREN)) {
        if (match(Token::token_type::IDENTIFIER)) {
            params.params.push_back(Ast::Parameter{current_token()});
            consume(Token::token_type::IDENTIFIER);
        }

        while (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);
            if (match(Token::token_type::IDENTIFIER)) {
                params.params.push_back(Ast::Parameter{current_token()});
                consume(Token::token_type::IDENTIFIER);
            }
        }
    }

    consume(Token::token_type::RPAREN);
    consume(Token::token_type::COLON);

    consume_line();
    Ast::Block body = parse_block();

    Ast::FunctionDef func{token, std::move(params), std::move(body)};
    auto func_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(func)});
    return func_node;
}

Ast::StmtPtr Parser::parser_class::parse_class() {
    consume(Token::token_type::KEYWORD_CLASS);

    Token::token_class token = current_token();
    consume(Token::token_type::IDENTIFIER);

    consume(Token::token_type::COLON);
    consume_line();

    Ast::Block body;
    body.token = current_token();

    while (!match(Token::token_type::DEDENT) && !is_at_end()) {
        if (match(Token::token_type::KEYWORD_DEF)) {
            auto method = parse_method();
            if (method) {
                body.statements.push_back(std::move(method));
            }
        } else {
            auto stmt = parse_statement();
            if (stmt) {
                body.statements.push_back(std::move(stmt));
            }
        }
    }

    consume(Token::token_type::DEDENT);

    Ast::ClassDef cls{token, std::move(body)};
    auto cls_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(cls)});
    return cls_node;
}

Ast::StmtPtr Parser::parser_class::parse_method() {
    consume(Token::token_type::KEYWORD_DEF);

    Token::token_class token = current_token();
    if (!match(Token::token_type::IDENTIFIER) && !match(Token::token_type::KEYWORD_INIT)) {
        debug_syntax_error();
    }

    if (match(Token::token_type::KEYWORD_INIT)) {
        consume(Token::token_type::KEYWORD_INIT);
    } else {
        consume(Token::token_type::IDENTIFIER);
    }

    valid_constructor = true; 

    consume(Token::token_type::LPAREN);

    Ast::ParameterList params;
    if (!match(Token::token_type::KEYWORD_SELF)) {
        debug_syntax_error();
    }

    params.params.push_back(Ast::Parameter{current_token()});
    consume(Token::token_type::KEYWORD_SELF);

    while (match(Token::token_type::COMMA)) {
        consume(Token::token_type::COMMA);
        if (match(Token::token_type::IDENTIFIER)) {
            params.params.push_back(Ast::Parameter{current_token()});
            consume(Token::token_type::IDENTIFIER);
        }
    }

    consume(Token::token_type::RPAREN);
    consume(Token::token_type::COLON);

    consume_line();
    Ast::Block body = parse_block();

    Ast::MethodDef method{token, std::move(params), std::move(body)};
    auto method_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(method)});
    return method_node;
}

Ast::StmtPtr Parser::parser_class::parse_break() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_BREAK);

    Ast::BreakStmt brk{token};
    auto brk_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(brk)});
    return brk_node;
}

Ast::StmtPtr Parser::parser_class::parse_continue() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_CONTINUE);

    Ast::ContinueStmt cont{token};
    auto cont_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(cont)});
    return cont_node;
}

Ast::StmtPtr Parser::parser_class::parse_if_stmt() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_IF);

    auto condition = parse_equality();

    consume_newline();
    Ast::Block body = parse_block();

    std::vector<Ast::ElifStmt> elifs;
    while (match(Token::token_type::KEYWORD_ELIF)) {
        Token::token_class elif_token = current_token();
        consume(Token::token_type::KEYWORD_ELIF);

        auto elif_condition = parse_expression_types();

        consume_newline();
        Ast::Block elif_body = parse_block();

        elifs.push_back(Ast::ElifStmt{elif_token, std::move(elif_condition), std::move(elif_body)});
    }

    std::optional<Ast::ElseStmt> else_branch = std::nullopt;
    if (match(Token::token_type::KEYWORD_ELSE)) {
        Token::token_class else_token = current_token();
        consume(Token::token_type::KEYWORD_ELSE);

        consume_newline();
        Ast::Block else_body = parse_block();

        else_branch = Ast::ElseStmt{else_token, std::move(else_body)};
    }

    Ast::IfStmt if_stmt{token, std::move(condition), std::move(body), std::move(elifs), std::move(else_branch)};
    
    auto if_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(if_stmt)});
    
    return if_node;
}

Ast::StmtPtr Parser::parser_class::parse_while_stmt() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_WHILE);

    auto condition = parse_equality();

    consume_newline();
    Ast::Block body = parse_block();

    Ast::WhileStmt while_stmt{token, std::move(condition), std::move(body)};
    auto while_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(while_stmt)});
    return while_node;
}

Ast::StmtPtr Parser::parser_class::parse_for_stmt() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_FOR);

    Ast::Identifier variable{current_token()};
    if (match(Token::token_type::IDENTIFIER)) {
        consume(Token::token_type::IDENTIFIER);
    }

    consume(Token::token_type::KEYWORD_IN);

    auto iterable = parse_expression_types();

    consume_newline();
    Ast::Block body = parse_block();

    Ast::ForStmt for_stmt{token, std::move(variable), std::move(iterable), std::move(body)};
    auto for_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(for_stmt)});
    return for_node;
}

Ast::StmtPtr Parser::parser_class::parse_match_stmt() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_MATCH);

    auto subject = parse_expression_types();

    consume_newline();
    std::vector<Ast::CaseStmt> cases;
    while (match(Token::token_type::KEYWORD_CASE) && !is_at_end()) {
        Token::token_class case_token = current_token();
        consume(Token::token_type::KEYWORD_CASE);

        auto pattern = parse_expression_types();

        consume_newline();
        Ast::Block case_body = parse_block();

        cases.push_back(Ast::CaseStmt{case_token, std::move(pattern), std::move(case_body)});
    }

    consume(Token::token_type::DEDENT);

    Ast::MatchStmt match_stmt{token, std::move(subject), std::move(cases)};
    auto match_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(match_stmt)});
    return match_node;
}

Ast::StmtPtr Parser::parser_class::parse_case() {
    Token::token_class token = current_token();
    consume(Token::token_type::KEYWORD_CASE);

    auto pattern = parse_expression_types();

    consume_newline();
    Ast::Block body = parse_block();

    Ast::CaseStmt case_stmt{token, std::move(pattern), std::move(body)};
    auto case_node = std::make_unique<Ast::StmtNode>(Ast::StmtNode{std::move(case_stmt)});
    return case_node;
}

Ast::ExprPtr Parser::parser_class::parse_list() {
    Token::token_class token = current_token();
    consume(Token::token_type::LBRACKET);

    Ast::ListExpr list{token, {}};

    if (match(Token::token_type::RBRACKET)) {
        consume(Token::token_type::RBRACKET);
        auto list_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(list)});
        return list_node;
    }

    list.elements.push_back(parse_expression_types());

    while (match(Token::token_type::COMMA)) {
        consume(Token::token_type::COMMA);

        if (match(Token::token_type::RBRACKET)) {
            break;
        }

        list.elements.push_back(parse_expression_types());
    }

    consume(Token::token_type::RBRACKET);
    auto list_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(list)});
    return list_node;
}

Ast::ExprPtr Parser::parser_class::parse_dict() {
    Token::token_class token = current_token();
    consume(Token::token_type::LCBRACE);

    Ast::DictExpr dict{token, {}};

    if (match(Token::token_type::RCBRACE)) {
        consume(Token::token_type::RCBRACE);
        auto dict_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(dict)});
        return dict_node;
    }

    auto key = parse_expression_types();

    while (match(Token::token_type::COLON) || match(Token::token_type::COMMA)) {
        consume(Token::token_type::COLON);
        auto value = parse_expression_types();
        dict.entries.push_back({std::move(key), std::move(value)});

        if (match(Token::token_type::COMMA)) {
            consume(Token::token_type::COMMA);

            if (match(Token::token_type::RCBRACE)) {
                break;
            }

            key = parse_expression_types();
        }
    }

    consume(Token::token_type::RCBRACE);
    auto dict_node = std::make_unique<Ast::ExprNode>(Ast::ExprNode{std::move(dict)});
    return dict_node;
}
