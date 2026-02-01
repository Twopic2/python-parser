#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>

#include "frontend/token.hpp"

namespace Ast {

    struct BinaryOp;
    struct UnaryOp;
    struct CallExpr;
    struct AttributeExpr;
    struct SubscriptExpr;
    struct ListExpr;
    struct DictExpr;
    struct SetExpr;
    struct TupleExpr;
    struct SelfExpr;
    struct LambdaExpr;

    struct IntegerLiteral {
        Token::token_class token;
    };

    struct FloatLiteral {
        Token::token_class token;
    };

    struct StringLiteral {
        Token::token_class token;
    };

    struct BytesLiteral {
        Token::token_class token;
    };

    struct BoolLiteral {
        bool value;
        Token::token_class token;
    };

    struct NoneLiteral {
        Token::token_class token;
    };

    struct Identifier {
        Token::token_class token;
    };

    using Literal = std::variant<
        IntegerLiteral,
        FloatLiteral,
        StringLiteral,
        BytesLiteral,
        BoolLiteral,
        NoneLiteral
    >;

    struct Expr;
    using ExprPtr = std::unique_ptr<Expr>;

    struct BinaryOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct UnaryOp {
        Token::token_class op;
        ExprPtr operand;
    };

    struct CallExpr {
        ExprPtr callee;
        std::vector<ExprPtr> args;
        Token::token_class token;
    };

    struct AttributeExpr {
        ExprPtr object;
        std::string attr;
        Token::token_class token;
    };

    struct SubscriptExpr {
        ExprPtr object;
        ExprPtr index;
        Token::token_class token;
    };

    struct ListExpr {
        std::vector<ExprPtr> elements;
        Token::token_class token;
    };

    struct DictExpr {
        std::vector<ExprPtr> keys;
        std::vector<ExprPtr> values;
        Token::token_class token;
    };

    struct SetExpr {
        std::vector<ExprPtr> elements;
        Token::token_class token;
    };

    struct TupleExpr {
        std::vector<ExprPtr> elements;
        Token::token_class token;
    };

    struct SelfExpr {
        std::optional<std::string> attr;
        Token::token_class token;
    };

    struct Expr {
        std::variant<
            Literal,
            Identifier,
            BinaryOp,
            UnaryOp,
            CallExpr,
            AttributeExpr,
            SubscriptExpr,
            ListExpr,
            DictExpr,
            SetExpr,
            TupleExpr,
            SelfExpr
        > data;

        template<typename T>
        Expr(T&& value) : data(std::forward<T>(value)) {}
    };

    template<typename T>
    ExprPtr make_expr(T&& value) {
        return std::make_unique<Expr>(std::forward<T>(value));
    }

    struct Stmt;
    using StmtPtr = std::unique_ptr<Stmt>;
    using Block = std::vector<StmtPtr>;

    struct Parameter {
        std::string name;
        Token::token_class token;
    };

    struct ExpressionStmt {
        ExprPtr expr;
    };

    struct Assignment {
        ExprPtr target;
        ExprPtr value;
        Token::token_class token;
    };

    struct AugmentedAssignment {
        ExprPtr target;
        ExprPtr value;
        Token::token_class token; 
    };

    struct ReturnStmt {
        std::optional<ExprPtr> value;
        Token::token_class token;
    };

    struct BreakStmt {
        Token::token_class token;
    };

    struct ContinueStmt {
        Token::token_class token;
    };

    struct PassStmt {
        Token::token_class token;
    };

    struct ElifClause {
        ExprPtr condition;
        Block body;
        Token::token_class token;
    };

    struct IfStmt {
        ExprPtr condition;
        Block body;
        std::vector<ElifClause> elifs;
        std::optional<Block> else_body;
        Token::token_class token;
    };

    struct WhileStmt {
        ExprPtr condition;
        Block body;
        Token::token_class token;
    };

    struct ForStmt {
        std::string var_name;
        ExprPtr iterable;
        Block body;
        Token::token_class token;
    };

    struct ExceptClause {
        Block body;
        Token::token_class token;
    };

    struct TryStmt {
        Block body;
        std::optional<ExceptClause> except;
        std::optional<Block> finally_body;
        std::optional<Block> else_body;
        Token::token_class token;
    };

    struct CaseClause {
        ExprPtr pattern;
        Block body;
        Token::token_class token;
    };

    struct MatchStmt {
        ExprPtr subject;
        std::vector<CaseClause> cases;
        Token::token_class token;
    };

    struct FunctionDef {
        std::string name;
        std::vector<Parameter> params;
        Block body;
        Token::token_class token;
    };

    struct MethodDef {
        std::string name;
        std::vector<Parameter> params;  
        Block body;
        Token::token_class token;
    };

    struct ClassDef {
        std::string name;
        Block body;
        Token::token_class token;
    };

    struct LambdaExpr {
        std::vector<Parameter> params;
        Block body;
        Token::token_class token;
    };

    struct Stmt {
        std::variant<
            ExpressionStmt,
            Assignment,
            AugmentedAssignment,
            ReturnStmt,
            BreakStmt,
            ContinueStmt,
            PassStmt,
            IfStmt,
            WhileStmt,
            ForStmt,
            TryStmt,
            MatchStmt,
            FunctionDef,
            MethodDef,
            ClassDef,
            LambdaExpr
        > data;

        template<typename T>
        Stmt(T&& value) : data(std::forward<T>(value)) {}
    };

    template<typename T>
    StmtPtr make_stmt(T&& value) {
        return std::make_unique<Stmt>(std::forward<T>(value));
    }

    struct Program {
        Block statements;
        Token::token_class token;
    };

    class ast_class {
        std::unique_ptr<Program> root;
    public:
        void set_root(std::unique_ptr<Program> r) { root = std::move(r); }
        bool is_empty() const { return root == nullptr || root->statements.empty(); }
        Program* get_root() const { return root.get(); }
    };

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

}

#endif
