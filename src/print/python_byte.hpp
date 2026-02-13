#ifndef PYTHON_BYTE_HPP
#define PYTHON_BYTE_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <iomanip>

#include <fmt/core.h>
#include "backend/bytecode.hpp"
#include "backend/objects.hpp"
#include "backend/value.hpp"

namespace BytePrinter {
    using namespace TwoPy::Backend;

    inline std::string opcode_to_string(OpCode op) {
        switch (op) {
            case OpCode::RETURN: return "RETURN";
            case OpCode::CALL: return "CALL";
            case OpCode::PRINT: return "PRINT";
            case OpCode::ADD: return "ADD";
            case OpCode::SUB: return "SUB";
            case OpCode::MUL: return "MUL";
            case OpCode::DIV: return "DIV";
            case OpCode::POP: return "POP";
            case OpCode::PUSH: return "PUSH";
            case OpCode::MAKE_FUNCTION: return "MAKE_FUNCTION";
            case OpCode::CALL_FUNCTION: return "CALL_FUNCTION";
            case OpCode::PUSH_NULL: return "PUSH_NULL";
            case OpCode::BINARY_POWER: return "BINARY_POWER";
            case OpCode::STORE_VARIABLE: return "STORE_VARIABLE";
            case OpCode::STORE_FAST: return "STORE_FAST";
            case OpCode::LOAD_VARIABLE: return "LOAD_VARIABLE";
            case OpCode::LOAD_FAST: return "LOAD_FAST";
            case OpCode::LOAD_CONSTANT: return "LOAD_CONSTANT";
            default: return "UNKNOWN";
        }
    }

    inline std::string value_to_string(const Value& val) {
        switch (val.tag()) {
            case ValueTag::NONE: return "None";
            case ValueTag::BOOL: return val.to_bool() ? "True" : "False";
            case ValueTag::INT: return std::to_string(val.to_long());
            case ValueTag::FLOAT: return std::to_string(val.to_double());
            case ValueTag::REF: return "<ref>";
            case ValueTag::OBJ: {
                auto obj = val.obj_ref();
                if (!obj) {
                    return "<null>";
                }
                // For strings, show them with quotes (Python-style)
                if (obj->tag() == ObjectTag::STRING) {
                    return "\"" + obj->stringify() + "\"";
                }
                // For other objects, just use their string representation
                return "<" + obj->stringify() + ">";
            }
            default: return "<unknown>";
        }
    }

    inline void print_instruction(const Instruction& instr, size_t offset,
                                  const Chunk& chunk) {
        std::string opname = opcode_to_string(instr.opcode);

        fmt::print("{:>6}  {:<20}", offset * 2, opname);

        switch (instr.opcode) {
            case OpCode::LOAD_CONSTANT:
                if (instr.argument < chunk.consts_pool.size()) {
                    fmt::print(" {:>3}  ({})",
                              instr.argument,
                              value_to_string(chunk.consts_pool[instr.argument]));
                } else {
                    fmt::print(" {:>3}  <invalid constant index>", instr.argument);
                }
                break;

            case OpCode::STORE_VARIABLE:
            case OpCode::LOAD_VARIABLE:
            case OpCode::STORE_FAST:
            case OpCode::LOAD_FAST:
                if (instr.argument < chunk.vars_pool.size()) {
                    fmt::print(" {:>3}  ({})",
                              instr.argument,
                              chunk.vars_pool[instr.argument]);
                } else {
                    fmt::print(" {:>3}  <invalid variable index>", instr.argument);
                }
                break;

            case OpCode::CALL_FUNCTION:
            case OpCode::CALL:
                fmt::print(" {:>3}  (arg count)", instr.argument);
                break;

            default:
                if (instr.argument != 0) {
                    fmt::print(" {:>3}", instr.argument);
                }
                break;
        }

        fmt::print("\n");
    }

    inline void disassemble_chunk(const Chunk& chunk, const std::string& name = "<chunk>") {
        fmt::print("Disassembly of {}:\n", name);
        fmt::print("Constants: [");
        for (size_t i = 0; i < chunk.consts_pool.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("{}", value_to_string(chunk.consts_pool[i]));
        }
        fmt::print("]\n");

        fmt::print("Variables: [");
        for (size_t i = 0; i < chunk.vars_pool.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("'{}'", chunk.vars_pool[i]);
        }
        fmt::print("]\n\n");

        fmt::print("Offset  Opcode               Arg  Details\n");
        fmt::print("------  -------------------  ---  -------\n");

        for (size_t i = 0; i < chunk.code.size(); ++i) {
            print_instruction(chunk.code[i], i, chunk);
        }

        fmt::print("\n");
    }

    inline void disassemble_program(const ByteCodeProgram& program) {
        fmt::print("=== Bytecode Program: {} ===\n\n", program.name);

        for (size_t i = 0; i < program.chunks.size(); ++i) {
            std::string chunk_name = (i == 0) ? "<module>" : fmt::format("<chunk {}>", i);
            disassemble_chunk(program.chunks[i], chunk_name);
        }

        fmt::print("=== End of {} ===\n", program.name);
    }
}

#endif
