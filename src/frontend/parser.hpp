#ifndef PARSER_HPP
#define PARSER_HPP

#include <utility>
#include <string>
#include <iostream>
#include <concepts>

#include <fmt/core.h>
#include <fmt/format.h>

#include "frontend/ast.hpp"
#include "frontend/lexical.hpp"

namespace Parser {
    class parser_class {
        private:
            std::vector<Token::token_class> tokens {};
            std::size_t current_pos {};
            Ast::ast_class ast_tree {};
            Token::token_class& current_token();
            int m_error_count;

            bool match(const Token::token_type& type);

            void debug_syntax_error() {
                m_error_count++;
                throw std::runtime_error {
                    fmt::format("Syntax Error at: line {} column {} ", std::to_string(current_token().line), std::to_string(current_token().column))
                };
            }

            template <typename TokenType, typename ... Rest> requires (std::same_as<TokenType, Token::token_type>)
            bool match(TokenType first_type, Rest ... more_types) noexcept {
                const auto current_tag = tokens.at(current_pos).type;
                return ((current_tag == first_type) || ... || (current_tag == more_types));
            }

            bool is_at_end();

            template <typename ... TokenTypes>
            void consume(TokenTypes ... types) {
                if constexpr (sizeof...(types) < 1) {
                    current_pos++;
                    return;
                } else {
                    if (const auto& current_token_ref = tokens.at(current_pos); !match(types...)) {
                        throw std::runtime_error(
                            fmt::format(
                                "Parse Error at source:{}:{}: Unexpected token.\n",
                                current_token_ref.line,
                                current_token_ref.column
                            )
                        );
                    }
                }

                current_pos++;
            }

            // Expression parsers - return ExprPtr
            Ast::ExprPtr parse_expression_types();
            Ast::ExprPtr parse_term();
            Ast::ExprPtr parse_factor();
            Ast::ExprPtr parse_power();
            Ast::ExprPtr parse_equality();
            Ast::ExprPtr parse_comparator();
            Ast::ExprPtr parse_assignment();
            Ast::ExprPtr parse_bitwise();

            Ast::ExprPtr parse_list();
            Ast::ExprPtr parse_dict();
            Ast::ExprPtr parse_call_expr();
            Ast::ExprPtr parse_attribute_expr();
            Ast::ExprPtr parse_self();

            // Statement parsers - return StmtPtr
            Ast::StmtPtr parse_statement();
            Ast::StmtPtr parse_function_def();
            Ast::StmtPtr parse_class();
            Ast::StmtPtr parse_if_stmt();
            Ast::StmtPtr parse_while_stmt();
            Ast::StmtPtr parse_for_stmt();
            Ast::StmtPtr parse_match_stmt();
            Ast::StmtPtr parse_return_stmt();
            Ast::StmtPtr parse_pass();
            Ast::StmtPtr parse_try();
            Ast::StmtPtr parse_break();
            Ast::StmtPtr parse_continue();
            Ast::StmtPtr parse_method();
            Ast::StmtPtr parse_lambda();
            Ast::CaseClause parse_case();

            // Helper parsers
            Ast::Block parse_block();
            void consume_newline();
            void consume_line();

        public:
            parser_class(Lexical::lexical_class& lexer);

            void parse();
            Ast::ast_class& get_ast();
    };
}

#endif
