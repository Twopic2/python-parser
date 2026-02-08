#ifndef OPCODE_HPP 
#define OPCODE_HPP

#include <cstddef>
#include <vector>
#include <variant>
#include <deque>

#include "frontend/ast.hpp"

namespace TwoPyOpByteCode {
    /* 
    Opcode (add, subtract, jump, whatever)
    Vars/Constant (for your STORE_VARIABLE and LOAD_CONSTANT)
     */
    enum class OpCode : std::uint8_t {
        RETURN,
        CALL,
        PRINT,
        ADD,
        SUB,
        MUL,
        DIV,
        POP,
        PUSH,
        STORE_VARIABLE,
        LOAD_VARIABLE,
        LOAD_CONSTANT
    };

    // Todo: Make an Object file which madles lists functions and code objects

    //  Argument used as an index to map to a certain consts or vars pool 
    struct ByteCode {
        OpCode opcode;
        std::uint16_t argument;
    };

    using Value = std::variant<std::monostate, long, double, std::string>;

    struct FullByteCode {
        std::vector<ByteCode> instructions;
        std::vector<Value> constants_pool;
        std::vector<std::string> vars_pool;
    };

    class chunk_class {
        private:
            FullByteCode m_bytecode {};
            const Ast::Program& m_program;

            void disassemble_instruction(const Ast::StmtPtr& stmt);
            void disassemble_expr(const Ast::ExprNode& expr);
            void disassemble_stmt(const Ast::StmtNode& stmt);

        public:
            chunk_class(const Ast::Program& program);

            FullByteCode disassemble_program();
    };
}

#endif