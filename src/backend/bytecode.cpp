#include "backend/bytecode.hpp"

#include <fmt/core.h>

/*
After doing some research, I found two ways of handling std::variant types
One would be to use std::hold_alternative which you'll have to manually check
or use std::visit which allows for more cleaner code in the future.
*/
namespace TwoPy::Backend {
    compiler::compiler(const TwoPy::Frontend::Program& program)
        : m_program(program) {
        m_bytecode_program.name = "<module>";
        m_bytecode_program.chunks.push_back(Chunk{});
        m_curr_chunk = &m_bytecode_program.chunks.back();
    }

    ByteCodeProgram compiler::disassemble_program() {
        for (const auto& ptr : m_program.statements) {
            disassemble_instruction(ptr);
        }

        m_bytecode_program.chunks.at(0).code.push_back({OpCode::RETURN});
        return m_bytecode_program;
    }

    void compiler::disassemble_instruction(const TwoPy::Frontend::StmtPtr& stmt) {
        try {
            disassemble_stmt(*stmt);
        } catch (const std::exception& e) {
            fmt::print("Error: {}\n", e.what());
        }
    }

    void compiler::disassemble_stmt(const TwoPy::Frontend::StmtNode& stmt) {
        if (auto* expr_stmt = std::get_if<TwoPy::Frontend::ExpressionStmt>(&stmt.node)) {
            if (expr_stmt->expression) {
                disassemble_expr(*expr_stmt->expression);
            }
        }
    }

    void compiler::disassemble_expr(const TwoPy::Frontend::ExprNode& expr) {
        if (auto* lits = std::get_if<TwoPy::Frontend::Literals>(&expr.node)) {
            disassemble_literals(*lits);
        }

        if (auto* ops = std::get_if<TwoPy::Frontend::OperatorsType>(&expr.node)) {
            disassemble_operators(*ops);
        }
    }

    void compiler::disassemble_operators(const TwoPy::Frontend::OperatorsType& ops) {
        if (auto* assign = std::get_if<TwoPy::Frontend::AssignmentOp>(&ops)) {
            if (assign->value) {
                disassemble_expr(*assign->value);
            }

            if (assign->target) {
                if (auto* ident = std::get_if<TwoPy::Frontend::Identifier>(&assign->target->node)) {
                    uint8_t var_index;

                    if (!global_vars.contains(ident->token.value)) {
                        m_curr_chunk->vars_pool.push_back(ident->token.value);
                        var_index = static_cast<uint8_t>(m_curr_chunk->vars_pool.size() - 1);
                        global_vars.insert({ident->token.value, var_index});
                    } else {
                        var_index = global_vars.at(ident->token.value);
                    }

                    m_curr_chunk->code.push_back({OpCode::STORE_VARIABLE, var_index});
                }
            }
        }

        if (auto* term = std::get_if<TwoPy::Frontend::TermOp>(&ops)) {
            if (term->left) disassemble_expr(*term->left);
            if (term->right) disassemble_expr(*term->right);

            std::string op = term->op.value;
            if (op == "+") {
                m_curr_chunk->code.push_back({OpCode::ADD});
            } else if (op == "-") {
                m_curr_chunk->code.push_back({OpCode::SUB});
            }
            return;
        }

        if (auto* factor = std::get_if<TwoPy::Frontend::FactorOp>(&ops)) {
            if (factor->left) disassemble_expr(*factor->left);
            if (factor->right) disassemble_expr(*factor->right);

            std::string op = factor->op.value;
            if (op == "*") {
                m_curr_chunk->code.push_back({OpCode::MUL});
            } else if (op == "/") {
                m_curr_chunk->code.push_back({OpCode::DIV});
            }
            return;
        }
    }

    void compiler::disassemble_literals(const TwoPy::Frontend::Literals& lits) {
        if (auto* int_lit = std::get_if<TwoPy::Frontend::IntegerLiteral>(&lits)) {
            m_curr_chunk->consts_pool.emplace_back(std::stol(int_lit->token.value));
            std::uint8_t const_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size() - 1);

            m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, const_index});
            return;
        }

        if (auto* float_lit = std::get_if<TwoPy::Frontend::FloatLiteral>(&lits)) {
            m_curr_chunk->consts_pool.emplace_back(std::stod(float_lit->token.value));
            std::uint8_t const_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size() - 1);

            m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, const_index});
            return;
        }

        if (auto* string_lit = std::get_if<TwoPy::Frontend::StringLiteral>(&lits)) {
            auto str_obj = std::make_unique<StringPyObject>(string_lit->token.value);
            
            auto raw_ptr = str_obj.get();

            m_curr_chunk->consts_pool.emplace_back(raw_ptr);

            object_pool.push_back(std::move(str_obj));

            std::uint8_t const_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size() - 1);

            m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, const_index});
            return;
        } 
    }

    /* void disassemble_function_object(const TwoPy::Frontend::FunctionDef& function) {
        auto* func_heap = std::get_if<TwoPy::Frontend::FuntionDef>(&function);
    } */

}
