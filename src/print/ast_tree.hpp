#ifndef AST_TREE_HPP
#define AST_TREE_HPP

#include <string>
#include <variant>
#include <fmt/core.h>
#include "frontend/ast.hpp"

namespace AstPrinter {

inline void print_indent(int depth) {
    for (int i = 0; i < depth; ++i) {
        fmt::print("  ");
    }
}

inline std::string token_value(const Token::token_class& token) {
    return std::string(token.value);
}

// Forward declarations
inline void print_expr(const Ast::ExprPtr& expr, int depth);
inline void print_stmt(const Ast::StmtPtr& stmt, int depth);
inline void print_block(const Ast::Block& block, int depth);

// Expression printers
inline void print_expr_node(const Ast::IntegerLiteral& node, int depth) {
    print_indent(depth);
    fmt::print("IntegerLiteral: {}\n", token_value(node.token));
}

inline void print_expr_node(const Ast::FloatLiteral& node, int depth) {
    print_indent(depth);
    fmt::print("FloatLiteral: {}\n", token_value(node.token));
}

inline void print_expr_node(const Ast::StringLiteral& node, int depth) {
    print_indent(depth);
    fmt::print("StringLiteral: {}\n", token_value(node.token));
}

inline void print_expr_node(const Ast::BoolLiteral& node, int depth) {
    print_indent(depth);
    fmt::print("BoolLiteral: {}\n", token_value(node.token));
}

inline void print_expr_node(const Ast::Identifier& node, int depth) {
    print_indent(depth);
    fmt::print("Identifier: {}\n", token_value(node.token));
}

inline void print_expr_node(const Ast::FactorOp& node, int depth) {
    print_indent(depth);
    fmt::print("FactorOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::TermOp& node, int depth) {
    print_indent(depth);
    fmt::print("TermOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::BitwiseOp& node, int depth) {
    print_indent(depth);
    fmt::print("BitwiseOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::EqualityOp& node, int depth) {
    print_indent(depth);
    fmt::print("EqualityOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::ComparisonOp& node, int depth) {
    print_indent(depth);
    fmt::print("ComparisonOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::PowerOp& node, int depth) {
    print_indent(depth);
    fmt::print("PowerOp: {}\n", token_value(node.op));
    if (node.base) print_expr(node.base, depth + 1);
    if (node.exponent) print_expr(node.exponent, depth + 1);
}

inline void print_expr_node(const Ast::AndOp& node, int depth) {
    print_indent(depth);
    fmt::print("AndOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::OrOp& node, int depth) {
    print_indent(depth);
    fmt::print("OrOp: {}\n", token_value(node.op));
    if (node.left) print_expr(node.left, depth + 1);
    if (node.right) print_expr(node.right, depth + 1);
}

inline void print_expr_node(const Ast::AssignmentOp& node, int depth) {
    print_indent(depth);
    fmt::print("AssignmentOp: {}\n", token_value(node.token));
    print_indent(depth + 1);
    fmt::print("target:\n");
    if (node.target) print_expr(node.target, depth + 2);
    print_indent(depth + 1);
    fmt::print("value:\n");
    if (node.value) print_expr(node.value, depth + 2);
}

inline void print_expr_node(const Ast::AugmentedAssignmentOp& node, int depth) {
    print_indent(depth);
    fmt::print("AugmentedAssignmentOp: {}\n", token_value(node.op));
    print_indent(depth + 1);
    fmt::print("target:\n");
    if (node.target) print_expr(node.target, depth + 2);
    print_indent(depth + 1);
    fmt::print("value:\n");
    if (node.value) print_expr(node.value, depth + 2);
}

inline void print_expr_node(const Ast::CallExpr& node, int depth) {
    print_indent(depth);
    fmt::print("CallExpr\n");
    print_indent(depth + 1);
    fmt::print("callee:\n");
    print_expr(node.callee, depth + 2);
    if (!node.arguments.empty()) {
        print_indent(depth + 1);
        fmt::print("arguments:\n");
        for (const auto& arg : node.arguments) {
            print_expr(arg, depth + 2);
        }
    }
}

inline void print_expr_node(const Ast::ConstructorCallExpr& node, int depth) {
    print_indent(depth);
    fmt::print("ConstructorCallExpr\n");
    print_indent(depth + 1);
    fmt::print("callee:\n");
    print_expr(node.constructor, depth + 2);
    if (!node.arguments.empty()) {
        print_indent(depth + 1);
        fmt::print("arguments:\n");
        for (const auto& arg : node.arguments) {
            print_expr(arg, depth + 2);
        }
    }
}

inline void print_expr_node(const Ast::AttributeExpr& node, int depth) {
    print_indent(depth);
    fmt::print("AttributeExpr\n");
    print_indent(depth + 1);
    fmt::print("constructor: {}\n", token_value(node.constructor.token));
    print_indent(depth + 1);
    fmt::print("attribute: {}\n", token_value(node.attribute.token));
}

inline void print_expr_node(const Ast::ListExpr& node, int depth) {
    print_indent(depth);
    fmt::print("ListExpr\n");
    for (const auto& elem : node.elements) {
        print_expr(elem, depth + 1);
    }
}

inline void print_expr_node(const Ast::DictExpr& node, int depth) {
    print_indent(depth);
    fmt::print("DictExpr\n");
    for (const auto& [key, value] : node.entries) {
        print_indent(depth + 1);
        fmt::print("entry:\n");
        print_indent(depth + 2);
        fmt::print("key:\n");
        print_expr(key, depth + 3);
        print_indent(depth + 2);
        fmt::print("value:\n");
        print_expr(value, depth + 3);
    }
}

inline void print_expr_node(const Ast::SelfExpr& node, int depth) {
    print_indent(depth);
    if (node.attribute) {
        fmt::print("SelfExpr.{}\n", token_value(node.attribute->token));
    } else {
        fmt::print("SelfExpr\n");
    }
}

// Visitor for Literals variant
inline void print_literal(const Ast::Literals& lit, int depth) {
    std::visit([depth](const auto& node) {
        print_expr_node(node, depth);
    }, lit);
}

// Visitor for Operators variant
inline void print_operator(const Ast::OperatorsType& op, int depth) {
    std::visit([depth](const auto& node) {
        print_expr_node(node, depth);
    }, op);
}

// Main expression printer
inline void print_expr(const Ast::ExprPtr& expr, int depth) {
    if (!expr) {
        print_indent(depth);
        fmt::print("(null)\n");
        return;
    }

    std::visit([depth](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Ast::Literals>) {
            print_literal(node, depth);
        } else if constexpr (std::is_same_v<T, Ast::OperatorsType>) {
            print_operator(node, depth);
        } else {
            print_expr_node(node, depth);
        }
    }, expr->node);
}

// Block printer
inline void print_block(const Ast::Block& block, int depth) {
    print_indent(depth);
    fmt::print("Block:\n");
    for (const auto& stmt : block.statements) {
        print_stmt(stmt, depth + 1);
    }
}

// Statement printers
inline void print_stmt_node(const Ast::ReturnStmt& node, int depth) {
    print_indent(depth);
    fmt::print("ReturnStmt\n");
    if (node.value) {
        print_expr(node.value, depth + 1);
    }
}

inline void print_stmt_node(const Ast::PassStmt& node, int depth) {
    print_indent(depth);
    fmt::print("PassStmt\n");
}

inline void print_stmt_node(const Ast::BreakStmt& node, int depth) {
    print_indent(depth);
    fmt::print("BreakStmt\n");
}

inline void print_stmt_node(const Ast::ContinueStmt& node, int depth) {
    print_indent(depth);
    fmt::print("ContinueStmt\n");
}

inline void print_stmt_node(const Ast::IfStmt& node, int depth) {
    print_indent(depth);
    fmt::print("IfStmt\n");
    print_indent(depth + 1);
    fmt::print("condition:\n");
    print_expr(node.condition, depth + 2);
    print_indent(depth + 1);
    fmt::print("body:\n");
    print_block(node.body, depth + 2);

    for (const auto& elif : node.elifs) {
        print_indent(depth + 1);
        fmt::print("elif:\n");
        print_indent(depth + 2);
        fmt::print("condition:\n");
        print_expr(elif.condition, depth + 3);
        print_block(elif.body, depth + 2);
    }

    if (node.else_branch) {
        print_indent(depth + 1);
        fmt::print("else:\n");
        print_block(node.else_branch->body, depth + 2);
    }
}

inline void print_stmt_node(const Ast::WhileStmt& node, int depth) {
    print_indent(depth);
    fmt::print("WhileStmt\n");
    print_indent(depth + 1);
    fmt::print("condition:\n");
    print_expr(node.condition, depth + 2);
    print_indent(depth + 1);
    fmt::print("body:\n");
    print_block(node.body, depth + 2);
}

inline void print_stmt_node(const Ast::ForStmt& node, int depth) {
    print_indent(depth);
    fmt::print("ForStmt\n");
    print_indent(depth + 1);
    fmt::print("variable: {}\n", token_value(node.variable.token));
    print_indent(depth + 1);
    fmt::print("iterable:\n");
    print_expr(node.iterable, depth + 2);
    print_indent(depth + 1);
    fmt::print("body:\n");
    print_block(node.body, depth + 2);
}

inline void print_stmt_node(const Ast::CaseStmt& node, int depth) {
    print_indent(depth);
    fmt::print("CaseStmt\n");
    print_indent(depth + 1);
    fmt::print("pattern:\n");
    print_expr(node.pattern, depth + 2);
    print_block(node.body, depth + 1);
}

inline void print_stmt_node(const Ast::MatchStmt& node, int depth) {
    print_indent(depth);
    fmt::print("MatchStmt\n");
    print_indent(depth + 1);
    fmt::print("subject:\n");
    print_expr(node.subject, depth + 2);
    for (const auto& case_stmt : node.cases) {
        print_stmt_node(case_stmt, depth + 1);
    }
}

inline void print_stmt_node(const Ast::TryStmt& node, int depth) {
    print_indent(depth);
    fmt::print("TryStmt\n");
    print_block(node.body, depth + 1);
    if (node.except_branch) {
        print_indent(depth + 1);
        fmt::print("except:\n");
        print_block(node.except_branch->body, depth + 2);
    }
    if (node.finally_branch) {
        print_indent(depth + 1);
        fmt::print("finally:\n");
        print_block(node.finally_branch->body, depth + 2);
    }
    if (node.else_branch) {
        print_indent(depth + 1);
        fmt::print("else:\n");
        print_block(node.else_branch->body, depth + 2);
    }
}

inline void print_stmt_node(const Ast::FunctionDef& node, int depth) {
    print_indent(depth);
    fmt::print("FunctionDef: {}\n", token_value(node.token));
    if (!node.params.params.empty()) {
        print_indent(depth + 1);
        fmt::print("params: ");
        for (size_t i = 0; i < node.params.params.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("{}", token_value(node.params.params[i].token));
        }
        fmt::print("\n");
    }
    print_block(node.body, depth + 1);
}

inline void print_stmt_node(const Ast::MethodDef& node, int depth) {
    print_indent(depth);
    fmt::print("MethodDef: {}\n", token_value(node.token));
    if (!node.params.params.empty()) {
        print_indent(depth + 1);
        fmt::print("params: ");
        for (size_t i = 0; i < node.params.params.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("{}", token_value(node.params.params[i].token));
        }
        fmt::print("\n");
    }
    print_block(node.body, depth + 1);
}

inline void print_stmt_node(const Ast::ClassDef& node, int depth) {
    print_indent(depth);
    fmt::print("ClassDef: {}\n", token_value(node.token));
    print_block(node.body, depth + 1);
}

inline void print_stmt_node(const Ast::LambdaStmt& node, int depth) {
    print_indent(depth);
    fmt::print("LambdaStmt\n");
    if (!node.params.params.empty()) {
        print_indent(depth + 1);
        fmt::print("params: ");
        for (size_t i = 0; i < node.params.params.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("{}", token_value(node.params.params[i].token));
        }
        fmt::print("\n");
    }
    print_indent(depth + 1);
    fmt::print("body:\n");
    for (const auto& stmt : node.body) {
        print_stmt(stmt, depth + 2);
    }
}

inline void print_stmt_node(const Ast::Block& node, int depth) {
    print_block(node, depth);
}

inline void print_stmt_node(const Ast::ExpressionStmt& node, int depth) {
    print_indent(depth);
    fmt::print("ExpressionStmt\n");
    if (node.expression) {
        print_expr(node.expression, depth + 1);
    }
}

// Main statement printer
inline void print_stmt(const Ast::StmtPtr& stmt, int depth) {
    if (!stmt) {
        print_indent(depth);
        fmt::print("(null)\n");
        return;
    }

    std::visit([depth](const auto& node) {
        print_stmt_node(node, depth);
    }, stmt->node);
}

// Program printer
inline void print_program(const Ast::Program& program) {
    fmt::print("Program\n");
    for (const auto& stmt : program.statements) {
        print_stmt(stmt, 1);
    }
}

// Convenience function
inline void print_ast(const Ast::Program& program) {
    print_program(program);
}

} // namespace AstPrinter

#endif
