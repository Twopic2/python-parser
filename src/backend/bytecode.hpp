#ifndef TWOPY_BYTECODE_HPP 
#define TWOPY_BYTECODE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <ranges>
#include <algorithm>

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

        STORE_FAST, // Local vars
        STORE_NAME, // Stuff like Classes, Functions, Dicts, Lists, etc etc

        COMPARE_OP,

        POP_JUMP_IF_FALSE,

        LOAD_FAST,  // Local vars
        LOAD_NAME,  // Module-level (mirrors STORE_NAME)
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
        std::vector<std::string> names_pool;
        std::size_t byte_offset;
    };

    struct ByteCodeProgram {
        std::string name;
        std::vector<std::shared_ptr<Chunk>> chunks;
    };

    class compiler {
    private:
        const TwoPy::Frontend::Program& m_program;

        bool is_in_function = false;

        std::map<std::string, std::uint8_t> global_vars {};

        std::shared_ptr<Chunk> m_curr_chunk {};

        ByteCodeProgram m_bytecode_program {};     

        void disassemble_instruction(const TwoPy::Frontend::StmtPtr& stmt);

        void disassemble_stmt(const TwoPy::Frontend::StmtNode& stmt);
        void disassemble_expr(const TwoPy::Frontend::ExprNode& expr);

        void disassemble_operators(const TwoPy::Frontend::OperatorsType& ops);
        void disassemble_literals(const TwoPy::Frontend::Literals& lits);

        void disassemble_function_object(const TwoPy::Frontend::FunctionDef& function);
        void disassemble_callexpr_object(const TwoPy::Frontend::CallExpr& callee);

        void disassemble_if_stmt(const TwoPy::Frontend::IfStmt& stmt);

    public:
        compiler(const TwoPy::Frontend::Program& program);

        ByteCodeProgram disassemble_program();
        
        [[nodiscard]] std::optional<ByteCodeProgram> operator()();
    };
}

#endif