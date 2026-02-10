#include "backend/bytecode.hpp"

#include <fmt/core.h>

using namespace TwoPyOpByteCode;

/*
After doing some research, I found two ways of handling std::variant types
One would be to use std::hold_alternative which you'll have to manually check
or use std::visit which allows for more cleaner code in the future.
*/

chunk_class::chunk_class(const Ast::Program& program)
    : m_program(program) {
    m_total_bytecode.push_back(std::make_shared<FullByteCode>());
    m_curr_bytecode = m_total_bytecode[0];
}

std::vector<std::shared_ptr<FullByteCode>> chunk_class::disassemble_program() {
    for (const auto& stmt : m_program.statements) {
        disassemble_instruction(stmt);
    }

    return m_total_bytecode;
}

void chunk_class::disassemble_instruction(const Ast::StmtPtr& stmt) {
    try {
        disassemble_stmt(*stmt);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
    }
}

void chunk_class::disassemble_stmt(const Ast::StmtNode& stmt) {
    if (std::holds_alternative<Ast::ExpressionStmt>(stmt.node)) {
        auto& expr_stmt = std::get<Ast::ExpressionStmt>(stmt.node);
        disassemble_expr(*expr_stmt.expression);
    } else if (std::holds_alternative<Ast::FunctionDef>(stmt.node)) {
        auto& func_stmt = std::get<Ast::FunctionDef>(stmt.node);
        disassemble_function_object(func_stmt);
    } else if (std::holds_alternative<Ast::ReturnStmt>(stmt.node)) {
        auto& return_stmt = std::get<Ast::ReturnStmt>(stmt.node);

        if (return_stmt.value.has_value()) {
            disassemble_expr(*return_stmt.value.value());
        }

        m_curr_bytecode->instructions.push_back({OpCode::RETURN, 0});
    }
}

void chunk_class::disassemble_function_object(const Ast::FunctionDef& func_stmt) {
    auto func_bytecode = std::make_shared<FullByteCode>();
    m_total_bytecode.push_back(func_bytecode);

    m_prev_bytecode = m_curr_bytecode;
    m_curr_bytecode = func_bytecode;
    
    is_in_function = true;

    for (const auto& body_stmt : func_stmt.body.statements) {
        try {
            disassemble_stmt(*body_stmt);
        } catch (const std::exception& e) {
            fmt::print(stderr, "Error in function body: {}\n", e.what());
        }
    }

    is_in_function = false;
    m_curr_bytecode = m_prev_bytecode;

    auto func_obj = std::make_shared<TwoObject::function_object>();
    func_obj->name = func_stmt.token.value;
    for (const auto& param : func_stmt.params.params) {
        func_obj->params.push_back(param.token.value);
    }
    func_obj->bytecode = func_bytecode;

    m_curr_bytecode->constants_pool.push_back(func_obj);
    m_curr_bytecode->instructions.push_back({
        OpCode::MAKE_FUNCTION,
        static_cast<std::uint8_t>(m_curr_bytecode->constants_pool.size() - 1)
    });

    m_curr_bytecode->vars_pool.push_back(func_stmt.token.value);
    m_curr_bytecode->instructions.push_back({
        OpCode::STORE_VARIABLE,
        static_cast<std::uint8_t>(m_curr_bytecode->vars_pool.size() - 1)
    });

    m_curr_bytecode->instructions.push_back({OpCode::PUSH_NULL});
}

void chunk_class::disassemble_expr(const Ast::ExprNode& expr) {
    if (std::holds_alternative<Ast::Literals>(expr.node)) {
        auto& lits = std::get<Ast::Literals>(expr.node);
        if (std::holds_alternative<Ast::IntegerLiteral>(lits)) {
            auto& integer = std::get<Ast::IntegerLiteral>(lits);
            m_curr_bytecode->constants_pool.push_back(std::stol(integer.token.value));
            m_curr_bytecode->instructions.push_back({
                OpCode::LOAD_CONSTANT,
                static_cast<std::uint8_t>(m_curr_bytecode->constants_pool.size() - 1)
            });
        } else if (std::holds_alternative<Ast::FloatLiteral>(lits)) {
            auto& float_lit = std::get<Ast::FloatLiteral>(lits);
            m_curr_bytecode->constants_pool.push_back(std::stod(float_lit.token.value));
            m_curr_bytecode->instructions.push_back({
                OpCode::LOAD_CONSTANT,
                static_cast<std::uint8_t>(m_curr_bytecode->constants_pool.size() - 1)
            });
        } else if (std::holds_alternative<Ast::StringLiteral>(lits)) {
            auto& string_lit = std::get<Ast::StringLiteral>(lits);
            m_curr_bytecode->constants_pool.push_back(string_lit.token.value);
            m_curr_bytecode->instructions.push_back({
                OpCode::LOAD_CONSTANT,
                static_cast<std::uint8_t>(m_curr_bytecode->constants_pool.size() - 1)
            });
        }
    } else if (std::holds_alternative<Ast::Identifier>(expr.node)) {
        auto& ident = std::get<Ast::Identifier>(expr.node);
        m_curr_bytecode->vars_pool.push_back(ident.token.value);
        m_curr_bytecode->instructions.push_back({
            is_in_function ? OpCode::LOAD_FAST : OpCode::LOAD_VARIABLE,
            static_cast<std::uint8_t>(m_curr_bytecode->vars_pool.size() - 1)
        });
    } else if (std::holds_alternative<Ast::OperatorsType>(expr.node)) {
        auto& ops = std::get<Ast::OperatorsType>(expr.node);

        if (std::holds_alternative<Ast::AssignmentOp>(ops)) {
            auto& assign = std::get<Ast::AssignmentOp>(ops);
            disassemble_expr(*assign.value);

            auto* ident = std::get_if<Ast::Identifier>(&assign.target->node);
            if (ident) {
                m_curr_bytecode->vars_pool.push_back(ident->token.value);
                m_curr_bytecode->instructions.push_back({
                    is_in_function ? OpCode::STORE_FAST : OpCode::STORE_VARIABLE,
                    static_cast<std::uint8_t>(m_curr_bytecode->vars_pool.size() - 1)
                });
            }

        } else if (std::holds_alternative<Ast::TermOp>(ops)) {
            auto& term = std::get<Ast::TermOp>(ops);
            disassemble_expr(*term.left);
            disassemble_expr(*term.right);

            if (term.op.value == "+") {
                m_curr_bytecode->instructions.push_back({OpCode::ADD, 0});
            } else if (term.op.value == "-") {
                m_curr_bytecode->instructions.push_back({OpCode::SUB, 0});
            }

        } else if (std::holds_alternative<Ast::FactorOp>(ops)) {
            auto& factor = std::get<Ast::FactorOp>(ops);
            disassemble_expr(*factor.left);
            disassemble_expr(*factor.right);

            if (factor.op.value == "*") {
                m_curr_bytecode->instructions.push_back({OpCode::MUL, 0});
            } else if (factor.op.value == "/") {
                m_curr_bytecode->instructions.push_back({OpCode::DIV, 0});
            }
        } else if (std::holds_alternative<Ast::PowerOp>(ops)) {
            auto& power = std::get<Ast::PowerOp>(ops);
            disassemble_expr(*power.base);
            disassemble_expr(*power.exponent);
            
            m_curr_bytecode->instructions.push_back({OpCode::BINARY_POWER});
        }
    } else if (std::holds_alternative<Ast::CallExpr>(expr.node)) {
        auto& call = std::get<Ast::CallExpr>(expr.node);

        disassemble_expr(*call.callee);

        for (const auto& arg : call.arguments) {
            disassemble_expr(*arg);
        }

        m_curr_bytecode->instructions.push_back({
            OpCode::CALL_FUNCTION,
            static_cast<std::uint8_t>(call.arguments.size())
        });
    }
}
