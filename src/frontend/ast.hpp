#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <string_view>
#include <cstddef>
#include <optional>

#include "frontend/token.hpp"

namespace Ast {
    /* Forward Decl - Expressions */
    struct ExprNode;
    using ExprPtr = std::unique_ptr<ExprNode>;

    struct IntegerLiteral;
    struct FloatLiteral;
    struct StringLiteral;
    struct Identifier;
    struct FactorOp;
    struct TermOp;
    struct AssignmentOp;
    struct AugmentedAssignmentOp;
    struct BitwiseOp;
    struct EqualityOp;
    struct ComparisonOp;
    struct PowerOp;
    struct CallExpr;
    struct AttributeExpr;
    struct ListExpr;
    struct DictExpr;
    struct SelfExpr;
    struct ConstructorCallExpr;
    struct OrOp;
    struct AndOp;

    /* Forward Decl - Statements */
    struct StmtNode;
    using StmtPtr = std::unique_ptr<StmtNode>;

    struct ReturnStmt;
    struct PassStmt;
    struct BreakStmt;
    struct ContinueStmt;
    struct IfStmt;
    struct ElifStmt;
    struct ElseStmt;
    struct WhileStmt;
    struct ForStmt;
    struct MatchStmt;
    struct CaseStmt;
    struct TryStmt;
    struct ExceptStmt;
    struct FinallyStmt;
    struct FunctionDef;
    struct MethodDef;
    struct ClassDef;
    struct LambdaStmt;
    struct Block;
    struct ExpressionStmt;

    /* why is std::variant such a pain to debug */
    using Literals = std::variant<IntegerLiteral, FloatLiteral, StringLiteral>;
    using OperatorsType = std::variant<AssignmentOp, AugmentedAssignmentOp, FactorOp, 
            TermOp, BitwiseOp, EqualityOp, ComparisonOp, PowerOp, OrOp, AndOp>;

    /* Begin Node define */

    struct IntegerLiteral {
        Token::token_class token;
    };

    struct FloatLiteral {
        Token::token_class token;
    };

    struct StringLiteral {
        Token::token_class token;
    };

    struct Identifier {
        Token::token_class token;
    };

    struct AndOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct OrOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct FactorOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct BitwiseOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct EqualityOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct ComparisonOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct PowerOp {
        Token::token_class op;
        ExprPtr base;
        ExprPtr exponent;
    };

    struct TermOp {
        Token::token_class op;
        ExprPtr left;
        ExprPtr right;
    };

    struct CallExpr {
        Token::token_class token;
        ExprPtr callee;
        std::vector<ExprPtr> arguments;
    };
   
    struct ConstructorCallExpr {
        Token::token_class token;
        ExprPtr constructor;
        std::vector<ExprPtr> arguments;
    };
    
    struct AttributeExpr {
        Token::token_class token;
        Identifier constructor;
        Identifier attribute;
    };

    struct ListExpr {
        Token::token_class token;
        std::vector<ExprPtr> elements;
    };

    struct DictExpr {
        Token::token_class token;
        std::vector<std::pair<ExprPtr, ExprPtr>> entries;
    };

    struct SelfExpr {
        Token::token_class token;
        std::optional<Identifier> attribute;
    };

    struct Block {
        Token::token_class token;
        std::vector<StmtPtr> statements;
    };

    struct Parameter {
        Token::token_class token;
    };

    struct ParameterList {
        std::vector<Parameter> params;
    };

    struct AssignmentOp {
        Token::token_class token;
        ExprPtr target;
        ExprPtr value;
    };

    struct AugmentedAssignmentOp {
        Token::token_class op;
        ExprPtr target;
        ExprPtr value;
    };

    struct ReturnStmt {
        Token::token_class token;
        std::optional<ExprPtr> value;
    };

    struct PassStmt {
        Token::token_class token;
    };

    struct BreakStmt {
        Token::token_class token;
    };

    struct ContinueStmt {
        Token::token_class token;
    };

    struct ElifStmt {
        Token::token_class token;
        ExprPtr condition;
        Block body;
    };

    struct ElseStmt {
        Token::token_class token;
        Block body;
    };

    struct IfStmt {
        Token::token_class token;
        ExprPtr condition;
        Block body;
        std::vector<ElifStmt> elifs;
        std::optional<ElseStmt> else_branch;
    };

    struct WhileStmt {
        Token::token_class token;
        ExprPtr condition;
        Block body;
    };

    struct ForStmt {
        Token::token_class token;
        Identifier variable;
        ExprPtr iterable;
        Block body;
    };

    struct CaseStmt {
        Token::token_class token;
        ExprPtr pattern;
        Block body;
    };

    struct MatchStmt {
        Token::token_class token;
        ExprPtr subject;
        std::vector<CaseStmt> cases;
    };

    struct ExceptStmt {
        Token::token_class token;
        Block body;
    };

    struct FinallyStmt {
        Token::token_class token;
        Block body;
    };

    struct TryStmt {
        Token::token_class token;
        Block body;
        std::optional<ExceptStmt> except_branch;
        std::optional<FinallyStmt> finally_branch;
        std::optional<ElseStmt> else_branch;
    };

    struct FunctionDef {
        Token::token_class token;
        ParameterList params;
        Block body;
    };

    struct MethodDef {
        Token::token_class token;
        ParameterList params;
        Block body;
    };

    struct ClassDef {
        Token::token_class token;
        Block body;
    };

    struct LambdaStmt {
        Token::token_class token;
        ParameterList params;
        std::vector<StmtPtr> body;
    };

    struct ExpressionStmt {
        Token::token_class token;
        ExprPtr expression;
    };

    /* Where the program inits */
    struct Program {
        std::vector<StmtPtr> statements;
    };

    /* Expr node */
    struct ExprNode {
        std::variant<
            Literals,
            OperatorsType,
            Identifier,
            CallExpr,
            ConstructorCallExpr,
            AttributeExpr,
            ListExpr,
            DictExpr,
            SelfExpr> node;
    };

     /* Statement Node */
    struct StmtNode {
        std::variant<
            ReturnStmt,
            PassStmt,
            BreakStmt,
            ContinueStmt,
            IfStmt,
            WhileStmt,
            ForStmt,
            MatchStmt,
            TryStmt,
            FunctionDef,
            MethodDef,
            ClassDef,
            LambdaStmt,
            CaseStmt,
            Block,
            ExpressionStmt> node;
    };
}

#endif