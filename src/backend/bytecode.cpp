#include "backend/bytecode.hpp"

#include <stdexcept>
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
        auto module_chunk = std::make_shared<Chunk>();
        m_bytecode_program.chunks.push_back(module_chunk);
        m_curr_chunk = module_chunk;
        m_prev_chunk = m_curr_chunk;
    }

    ByteCodeProgram compiler::disassemble_program() {
        for (const auto& ptr : m_program.statements) {
            disassemble_instruction(ptr);
        }

        m_bytecode_program.chunks.at(0)->code.push_back({OpCode::RETURN});
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
            } else {
                throw std::runtime_error("Something went wrong");
            }
        }

        if (auto* func_def = std::get_if<TwoPy::Frontend::FunctionDef>(&stmt.node)) {
            disassemble_function_object(*func_def);
        }
    }

    void compiler::disassemble_expr(const TwoPy::Frontend::ExprNode& expr) {
        if (auto* callee = std::get_if<TwoPy::Frontend::CallExpr>(&expr.node)) {
            disassemble_callexpr_object(*callee);
        }

         /// NOTE: Okay as DerekT Suggested Nested Variants is a horrible idea and should never be used in production! 
        /// I'm going to fix this inside my Ast.hpp where there will be no more nested variants. 
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
                    std::uint8_t var_index;

                    if (!global_vars.contains(ident->token.value)) {
                        m_curr_chunk->names_pool.push_back(ident->token.value);
                        var_index = static_cast<std::uint8_t>(m_curr_chunk->names_pool.size() - 1);
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
            auto str_obj = std::make_shared<StringPyObject>(string_lit->token.value);

            m_curr_chunk->consts_pool.emplace_back(str_obj);

            std::uint8_t const_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size() - 1);

            m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, const_index});
            return;
        }
    }

    /// TODO: Since I'm Lazy, I forgot to add STORE/LOAD_FAST for local vars 
    void compiler::disassemble_function_object(const TwoPy::Frontend::FunctionDef& function) {
        auto func_chunk = std::make_shared<Chunk>();
        m_bytecode_program.chunks.push_back(func_chunk);
        std::uint8_t func_chunk_index = static_cast<std::uint8_t>(m_bytecode_program.chunks.size() - 1);

        m_prev_chunk = m_curr_chunk;
        m_curr_chunk = func_chunk;

        for (const auto& stmt : function.body.statements) {
            disassemble_instruction(stmt);
        }
        m_curr_chunk->code.push_back({OpCode::RETURN});

        m_curr_chunk = m_prev_chunk;

        std::vector<std::string> param_names;
        for (const auto& param : function.params.params) {
            param_names.push_back(param.token.value);
        }

        auto func_obj = std::make_shared<FunctionPyObject>(
            function.token.value, std::move(param_names), func_chunk_index
        );

        std::uint8_t code_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size());
        m_curr_chunk->consts_pool.emplace_back(func_obj);
        m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, code_index});

        auto name_obj = std::make_shared<StringPyObject>(function.token.value);
        std::uint8_t name_index = static_cast<std::uint8_t>(m_curr_chunk->consts_pool.size());
        m_curr_chunk->consts_pool.emplace_back(name_obj);
        m_curr_chunk->code.push_back({OpCode::LOAD_CONSTANT, name_index});

        m_curr_chunk->code.push_back({OpCode::MAKE_FUNCTION, 0});

        std::uint8_t var_index;
        if (!global_vars.contains(function.token.value)) {
            m_curr_chunk->names_pool.push_back(function.token.value);
            var_index = static_cast<std::uint8_t>(m_curr_chunk->names_pool.size() - 1);
            global_vars.insert({function.token.value, var_index});
        } else {
            var_index = global_vars.at(function.token.value);
        }
        m_curr_chunk->code.push_back({OpCode::STORE_NAME, var_index});
        m_curr_chunk->consts_pool.emplace_back(name_obj);
    }

    void compiler::disassemble_callexpr_object(const TwoPy::Frontend::CallExpr& callee) {
        auto* ident = std::get_if<TwoPy::Frontend::Identifier>(&callee.callee->node);
        std::string func_name = ident->token.value;

        if (global_vars.contains(func_name)) {
            std::uint8_t var_index = global_vars.at(func_name);
            m_curr_chunk->code.push_back({OpCode::LOAD_NAME, var_index});
        } else {
            m_curr_chunk->names_pool.push_back(func_name);
            std::uint8_t var_index = static_cast<std::uint8_t>(m_curr_chunk->names_pool.size() - 1);
            m_curr_chunk->code.push_back({OpCode::LOAD_NAME, var_index});
        }

        for (const auto& arg : callee.arguments) {
            disassemble_expr(*arg);
        }

        std::uint8_t arg_count = static_cast<std::uint8_t>(callee.arguments.size());
        m_curr_chunk->code.push_back({OpCode::CALL_FUNCTION, arg_count});
    }
}
