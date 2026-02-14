#ifndef TWOPY_BYTECODE_HPP 
#define TWOPY_BYTECODE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

#include "backend/value.hpp"
#include "frontend/ast.hpp"

namespace TwoPy::Backend {
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

    /* Inside Python's bytecode 3.6 documentation. Use 2 bytes for each instruction. Previously the number of bytes varied by instruction.*/
    struct Instruction {
        OpCode opcode;          // VM opcode
        std::uint8_t argument;  // index to a certain constant or local variable slot
    };

    struct Chunk {
        std::vector<Instruction> code;
        std::vector<Value> consts_pool;
        std::vector<std::string> vars_pool;
    };

    struct ByteCodeProgram {
        std::string name;
        std::vector<Chunk> chunks;
    };

    class compiler {
    private:
        const Ast::Program& m_program;

        std::map<std::string, std::uint8_t> global_vars {};

        Chunk* m_curr_chunk {};
        Chunk* m_prev_chunk {};

        std::vector<std::unique_ptr<ObjectBase>> object_pool;

        ByteCodeProgram m_bytecode_program {};

        void disassemble_instruction(const Ast::StmtPtr& stmt);

        void disassemble_stmt(const Ast::StmtNode& stmt);
        void disassemble_expr(const Ast::ExprNode& expr);

        void disassemble_operators(const Ast::OperatorsType& ops);
        void disassemble_literals(const Ast::Literals& lits);

        void disassemble_function_object(const Ast::FunctionDef& function);

    public:
        compiler(const Ast::Program& program);

        ByteCodeProgram disassemble_program();
        
        [[nodiscard]] std::optional<ByteCodeProgram> operator()();
    };
}

#endif