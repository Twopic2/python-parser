#include <fstream>
#include <sstream>
#include <string_view>
#include <string>
#include <fmt/core.h>

#include "frontend/lexical.hpp"
#include "frontend/parser.hpp"

void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        fmt::print("  ");
    }
}

void print_expr(const Ast::Expr& expr, int indent);
void print_stmt(const Ast::Stmt& stmt, int indent);
void print_block(const Ast::Block& block, int indent);

void print_literal(const Ast::Literal& lit, int indent) {
    std::visit(Ast::overloaded{
        [indent](const Ast::IntegerLiteral& i) {
            print_indent(indent);
            fmt::print("IntegerLiteral: {} [line {}, col {}]\n", i.token.value, i.token.line, i.token.column);
        },
        [indent](const Ast::FloatLiteral& f) {
            print_indent(indent);
            fmt::print("FloatLiteral: {} [line {}, col {}]\n", f.token.value, f.token.line, f.token.column);
        },
        [indent](const Ast::StringLiteral& s) {
            print_indent(indent);
            fmt::print("StringLiteral: \"{}\" [line {}, col {}]\n", s.token.value, s.token.line, s.token.column);
        },
        [indent](const Ast::BytesLiteral& b) {
            print_indent(indent);
            fmt::print("BytesLiteral: {} [line {}, col {}]\n", b.token.value, b.token.line, b.token.column);
        },
        [indent](const Ast::BoolLiteral& b) {
            print_indent(indent);
            fmt::print("BoolLiteral: {} [line {}, col {}]\n", b.value ? "True" : "False", b.token.line, b.token.column);
        },
        [indent](const Ast::NoneLiteral& n) {
            print_indent(indent);
            fmt::print("NoneLiteral [line {}, col {}]\n", n.token.line, n.token.column);
        }
    }, lit);
}

void print_expr(const Ast::Expr& expr, int indent) {
    std::visit(Ast::overloaded{
        [indent](const Ast::Literal& lit) {
            print_literal(lit, indent);
        },
        [indent](const Ast::Identifier& id) {
            print_indent(indent);
            fmt::print("Identifier: {} [line {}, col {}]\n", id.token.value, id.token.line, id.token.column);
        },
        [indent](const Ast::BinaryOp& bin) {
            print_indent(indent);
            fmt::print("BinaryOp: {} [line {}, col {}]\n", bin.op.value, bin.op.line, bin.op.column);
            if (bin.left) print_expr(*bin.left, indent + 1);
            if (bin.right) print_expr(*bin.right, indent + 1);
        },
        [indent](const Ast::UnaryOp& un) {
            print_indent(indent);
            fmt::print("UnaryOp: {} [line {}, col {}]\n", un.op.value, un.op.line, un.op.column);
            if (un.operand) print_expr(*un.operand, indent + 1);
        },
        [indent](const Ast::CallExpr& call) {
            print_indent(indent);
            fmt::print("CallExpr [line {}, col {}]\n", call.token.line, call.token.column);
            if (call.callee) {
                print_indent(indent + 1);
                fmt::print("Callee:\n");
                print_expr(*call.callee, indent + 2);
            }
            if (!call.args.empty()) {
                print_indent(indent + 1);
                fmt::print("Arguments:\n");
                for (const auto& arg : call.args) {
                    print_expr(*arg, indent + 2);
                }
            }
        },
        [indent](const Ast::AttributeExpr& attr) {
            print_indent(indent);
            fmt::print("AttributeExpr: .{} [line {}, col {}]\n", attr.attr, attr.token.line, attr.token.column);
            if (attr.object) print_expr(*attr.object, indent + 1);
        },
        [indent](const Ast::SubscriptExpr& sub) {
            print_indent(indent);
            fmt::print("SubscriptExpr [line {}, col {}]\n", sub.token.line, sub.token.column);
            if (sub.object) print_expr(*sub.object, indent + 1);
            if (sub.index) print_expr(*sub.index, indent + 1);
        },
        [indent](const Ast::ListExpr& list) {
            print_indent(indent);
            fmt::print("ListExpr [line {}, col {}]\n", list.token.line, list.token.column);
            for (const auto& elem : list.elements) {
                print_expr(*elem, indent + 1);
            }
        },
        [indent](const Ast::DictExpr& dict) {
            print_indent(indent);
            fmt::print("DictExpr [line {}, col {}]\n", dict.token.line, dict.token.column);
            for (size_t i = 0; i < dict.keys.size(); i++) {
                print_indent(indent + 1);
                fmt::print("Key:\n");
                print_expr(*dict.keys[i], indent + 2);
                if (i < dict.values.size()) {
                    print_indent(indent + 1);
                    fmt::print("Value:\n");
                    print_expr(*dict.values[i], indent + 2);
                }
            }
        },
        [indent](const Ast::SetExpr& set) {
            print_indent(indent);
            fmt::print("SetExpr [line {}, col {}]\n", set.token.line, set.token.column);
            for (const auto& elem : set.elements) {
                print_expr(*elem, indent + 1);
            }
        },
        [indent](const Ast::TupleExpr& tuple) {
            print_indent(indent);
            fmt::print("TupleExpr [line {}, col {}]\n", tuple.token.line, tuple.token.column);
            for (const auto& elem : tuple.elements) {
                print_expr(*elem, indent + 1);
            }
        },
        [indent](const Ast::SelfExpr& self) {
            print_indent(indent);
            if (self.attr) {
                fmt::print("SelfExpr.{} [line {}, col {}]\n", *self.attr, self.token.line, self.token.column);
            } else {
                fmt::print("SelfExpr [line {}, col {}]\n", self.token.line, self.token.column);
            }
        }
    }, expr.data);
}

void print_block(const Ast::Block& block, int indent) {
    for (const auto& stmt : block) {
        print_stmt(*stmt, indent);
    }
}

void print_stmt(const Ast::Stmt& stmt, int indent) {
    std::visit(Ast::overloaded{
        [indent](const Ast::ExpressionStmt& es) {
            print_indent(indent);
            fmt::print("ExpressionStmt\n");
            if (es.expr) print_expr(*es.expr, indent + 1);
        },
        [indent](const Ast::Assignment& assign) {
            print_indent(indent);
            fmt::print("Assignment [line {}, col {}]\n", assign.token.line, assign.token.column);
            if (assign.target) {
                print_indent(indent + 1);
                fmt::print("Target:\n");
                print_expr(*assign.target, indent + 2);
            }
            if (assign.value) {
                print_indent(indent + 1);
                fmt::print("Value:\n");
                print_expr(*assign.value, indent + 2);
            }
        },
        [indent](const Ast::AugmentedAssignment& aug) {
            print_indent(indent);
            fmt::print("AugmentedAssignment: {} [line {}, col {}]\n", aug.token.value, aug.token.line, aug.token.column);
            if (aug.target) print_expr(*aug.target, indent + 1);
            if (aug.value) print_expr(*aug.value, indent + 1);
        },
        [indent](const Ast::ReturnStmt& ret) {
            print_indent(indent);
            fmt::print("ReturnStmt [line {}, col {}]\n", ret.token.line, ret.token.column);
            if (ret.value && *ret.value) print_expr(**ret.value, indent + 1);
        },
        [indent](const Ast::BreakStmt& brk) {
            print_indent(indent);
            fmt::print("BreakStmt [line {}, col {}]\n", brk.token.line, brk.token.column);
        },
        [indent](const Ast::ContinueStmt& cont) {
            print_indent(indent);
            fmt::print("ContinueStmt [line {}, col {}]\n", cont.token.line, cont.token.column);
        },
        [indent](const Ast::PassStmt& pass) {
            print_indent(indent);
            fmt::print("PassStmt [line {}, col {}]\n", pass.token.line, pass.token.column);
        },
        [indent](const Ast::IfStmt& if_stmt) {
            print_indent(indent);
            fmt::print("IfStmt [line {}, col {}]\n", if_stmt.token.line, if_stmt.token.column);
            print_indent(indent + 1);
            fmt::print("Condition:\n");
            if (if_stmt.condition) print_expr(*if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(if_stmt.body, indent + 2);
            for (const auto& elif : if_stmt.elifs) {
                print_indent(indent + 1);
                fmt::print("Elif:\n");
                if (elif.condition) print_expr(*elif.condition, indent + 2);
                print_block(elif.body, indent + 2);
            }
            if (if_stmt.else_body) {
                print_indent(indent + 1);
                fmt::print("Else:\n");
                print_block(*if_stmt.else_body, indent + 2);
            }
        },
        [indent](const Ast::WhileStmt& while_stmt) {
            print_indent(indent);
            fmt::print("WhileStmt [line {}, col {}]\n", while_stmt.token.line, while_stmt.token.column);
            print_indent(indent + 1);
            fmt::print("Condition:\n");
            if (while_stmt.condition) print_expr(*while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(while_stmt.body, indent + 2);
        },
        [indent](const Ast::ForStmt& for_stmt) {
            print_indent(indent);
            fmt::print("ForStmt: {} [line {}, col {}]\n", for_stmt.var_name, for_stmt.token.line, for_stmt.token.column);
            print_indent(indent + 1);
            fmt::print("Iterable:\n");
            if (for_stmt.iterable) print_expr(*for_stmt.iterable, indent + 2);
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(for_stmt.body, indent + 2);
        },
        [indent](const Ast::TryStmt& try_stmt) {
            print_indent(indent);
            fmt::print("TryStmt [line {}, col {}]\n", try_stmt.token.line, try_stmt.token.column);
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(try_stmt.body, indent + 2);
            if (try_stmt.except) {
                print_indent(indent + 1);
                fmt::print("Except:\n");
                print_block(try_stmt.except->body, indent + 2);
            }
            if (try_stmt.finally_body) {
                print_indent(indent + 1);
                fmt::print("Finally:\n");
                print_block(*try_stmt.finally_body, indent + 2);
            }
            if (try_stmt.else_body) {
                print_indent(indent + 1);
                fmt::print("Else:\n");
                print_block(*try_stmt.else_body, indent + 2);
            }
        },
        [indent](const Ast::MatchStmt& match_stmt) {
            print_indent(indent);
            fmt::print("MatchStmt [line {}, col {}]\n", match_stmt.token.line, match_stmt.token.column);
            print_indent(indent + 1);
            fmt::print("Subject:\n");
            if (match_stmt.subject) print_expr(*match_stmt.subject, indent + 2);
            for (const auto& case_clause : match_stmt.cases) {
                print_indent(indent + 1);
                fmt::print("Case:\n");
                if (case_clause.pattern) print_expr(*case_clause.pattern, indent + 2);
                print_block(case_clause.body, indent + 2);
            }
        },
        [indent](const Ast::FunctionDef& func) {
            print_indent(indent);
            fmt::print("FunctionDef: {} [line {}, col {}]\n", func.name, func.token.line, func.token.column);
            print_indent(indent + 1);
            fmt::print("Parameters:\n");
            for (const auto& param : func.params) {
                print_indent(indent + 2);
                fmt::print("Parameter: {}\n", param.name);
            }
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(func.body, indent + 2);
        },
        [indent](const Ast::MethodDef& method) {
            print_indent(indent);
            fmt::print("MethodDef: {} [line {}, col {}]\n", method.name, method.token.line, method.token.column);
            print_indent(indent + 1);
            fmt::print("Parameters:\n");
            for (const auto& param : method.params) {
                print_indent(indent + 2);
                fmt::print("Parameter: {}\n", param.name);
            }
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(method.body, indent + 2);
        },
        [indent](const Ast::ClassDef& cls) {
            print_indent(indent);
            fmt::print("ClassDef: {} [line {}, col {}]\n", cls.name, cls.token.line, cls.token.column);
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(cls.body, indent + 2);
        },
        [indent](const Ast::LambdaExpr& lambda) {
            print_indent(indent);
            fmt::print("LambdaExpr [line {}, col {}]\n", lambda.token.line, lambda.token.column);
            print_indent(indent + 1);
            fmt::print("Parameters:\n");
            for (const auto& param : lambda.params) {
                print_indent(indent + 2);
                fmt::print("Parameter: {}\n", param.name);
            }
            print_indent(indent + 1);
            fmt::print("Body:\n");
            print_block(lambda.body, indent + 2);
        }
    }, stmt.data);
}

void print_program(const Ast::Program* program) {
    if (!program) return;
    fmt::print("Program [line {}, col {}]\n", program->token.line, program->token.column);
    print_block(program->statements, 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fmt::print(stderr, "Usage: {} <file.py>\n", argv[0]);
        fmt::print(stderr, "Example: {} test.py\n", argv[0]);
        return 1;
    }

    fmt::print("=== Parsing file: {} ===\n\n", argv[1]);

    try {
        const std::string& source_code = Lexical::read_file(argv[1]);
        Lexical::lexical_class lexer(source_code);

        Parser::parser_class parser(lexer);
        parser.parse();

        fmt::print("\n=== ABSTRACT SYNTAX TREE ===\n");

        auto& ast_tree = parser.get_ast();
        if (!ast_tree.is_empty()) {
            print_program(ast_tree.get_root());
        } else {
            fmt::print("(empty AST)\n");
        }

        fmt::print("\n=== PARSING COMPLETE ===\n");

    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }

    return 0;
}
