#ifndef OPCODE_HPP 
#define OPCODE_HPP

#include <cstddef>
#include <vector>
#include <variant>
#include <deque>
#include <memory>

#include "frontend/ast.hpp"
#include "backend/objects.hpp"

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

        MAKE_FUNCTION,
        CALL_FUNCTION,
        PUSH_NULL,		// Prepares the stack for a function call.
        BINARY_POWER,

        STORE_VARIABLE,
        STORE_FAST, // Local vars

        LOAD_VARIABLE,
        LOAD_FAST, // Local vars
        LOAD_CONSTANT,
    };

    //  Argument used as an index to map to a certain consts or vars pool 

    /* Inside Python's bytecode 3.6 documentation. Use 2 bytes for each instruction. Previously the number of bytes varied by instruction.*/
    struct ByteCode {
        OpCode opcode;
        std::uint8_t argument;
    };

    using Value = std::variant<std::monostate, long, double, std::string, std::shared_ptr<TwoObject::function_object>>;

    struct FullByteCode {
        std::vector<ByteCode> instructions;
        std::vector<Value> constants_pool;
        std::vector<std::string> vars_pool;
    };

    class chunk_class {
        private:
            std::vector<std::shared_ptr<FullByteCode>> m_total_bytecode {};
            std::shared_ptr<FullByteCode> m_curr_bytecode {};
            std::shared_ptr<FullByteCode> m_prev_bytecode {};

            bool is_in_function = false;

            const Ast::Program& m_program;

            void disassemble_instruction(const Ast::StmtPtr& stmt);
            void disassemble_expr(const Ast::ExprNode& expr);
            void disassemble_stmt(const Ast::StmtNode& stmt);

            void disassemble_function_object(const Ast::FunctionDef& func_stmt);

        public:
            chunk_class(const Ast::Program& program);

            std::vector<std::shared_ptr<FullByteCode>> disassemble_program();
    };
}

#endif