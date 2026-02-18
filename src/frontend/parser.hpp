#ifndef PARSER_HPP
#define PARSER_HPP

#include <utility>
#include <string>
#include <format>
#include <iostream>
#include <concepts>

#include <fmt/core.h>

#include "frontend/ast.hpp"
#include "frontend/lexical.hpp"

/* Basically anylisis the syntax of a group of phrases */

/* Old notes sucks but its basically a top-down or LL parser*/


/*
Left-Associative example:
a - b - c  ≡  (a - b) - c
Expr → Term { op Term }

Right-Associative example:
a ** b ** c  ≡  a ** (b ** c)
Expr → Base [ op Expr ]   (uses recursion, not loop)
*/


namespace TwoPy::Frontend {
    /* I decided upon a recursive descent appoarch */
    class parser_class {
        private:
            /* Should make is a vector of bools */
            bool valid_constructor = false;
            
            std::vector<token_class> tokens {};
            std::size_t current_pos {};
            std::size_t m_previous_pos {};
            
            token_class& current_token();
            token_class& previous_token();

            int m_error_count {};

            bool match(const token_type& type);

            // Might be useful to track the amount of errors
            void debug_syntax_error() {
                m_error_count++;
                throw std::runtime_error {
                    fmt::format("Syntax Error at: line {} column {} ", std::to_string(current_token().line), std::to_string(current_token().column))
                };
            }

            // Special thanks to DerkT for fixing up my code!
            template <typename TokenType, typename ... Rest> requires (std::same_as<TokenType, token_type>)
            bool match(TokenType first_type, Rest ... more_types) noexcept {
                const auto current_tag = tokens.at(current_pos).type;
                return ((current_tag == first_type) || ... || (current_tag == more_types));
            }

            bool is_at_end();

            // Special thanks to DerkT for fixing up my code!
            template <typename ... TokenTypes>
            void consume(TokenTypes ... types) {
                if constexpr (sizeof...(types) < 1) {
                    m_previous_pos = current_pos;
                    current_pos++;
                    return;
                } else {
                    // Basically if i consumed the wrong type of the current type like consume(Colon) != match(Newline)
                    // it has a runtime error
                    if (const auto& current_token_ref = tokens.at(current_pos); !match(types...)) {
                        throw std::runtime_error(
                            std::format(
                                "Parse Error at source:{}:{}: Unexpected token.\n",
                                current_token_ref.line,
                                current_token_ref.column
                            )
                        );
                    }
                }

                m_previous_pos = current_pos;
                current_pos++;
            }

            // Something that produces a value
            ExprPtr parse_expression_types();
            // Performs a action ex: if, while, for, return
            StmtPtr parse_statement();

            StmtPtr parse_function_def();
            StmtPtr parse_class();
            StmtPtr parse_if_stmt();
            StmtPtr parse_while_stmt();
            StmtPtr parse_for_stmt();
            StmtPtr parse_match_stmt();
            StmtPtr parse_return_stmt();
            StmtPtr parse_pass();
            StmtPtr parse_try();
            StmtPtr parse_break();
            StmtPtr parse_continue();
            StmtPtr parse_method();
            StmtPtr parse_lambda();

            StmtPtr parse_expression_stmt(const auto& token);

            ExprPtr parse_list();
            ExprPtr parse_dict();

            ExprPtr parse_call_expr(ExprPtr callee);
            ExprPtr parse_constructor_call(ExprPtr constructor);
            ExprPtr parse_attribute_expr();
            ExprPtr parse_self();

            ExprPtr parse_term();
            ExprPtr parse_factor();
            ExprPtr parse_power();
            StmtPtr parse_case();
            ExprPtr parse_equality();
            ExprPtr parse_comparator();
            ExprPtr parse_assignment();
            ExprPtr parse_bitwise();
            ExprPtr parse_logical_or();
            ExprPtr parse_logical_and();

            Block parse_block();

            void consume_newline();
            void consume_line();

        public:
            parser_class(lexical_class& lexer);

            /* Parses the file */
            Program parse();
    };
}

#endif
