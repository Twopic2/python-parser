#ifndef PYTHON_BYTE_HPP
#define PYTHON_BYTE_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>

#include <fmt/core.h>
#include "backend/bytecode.hpp"
#include "backend/objects.hpp"

namespace BytePrinter {

/* Claude code saved me so much time */

inline const char* opcode_name(TwoPyOpByteCode::OpCode op) {
    try {
        switch (op) {
            case TwoPyOpByteCode::OpCode::RETURN:         return "RETURN";
            case TwoPyOpByteCode::OpCode::CALL:            return "CALL";
            case TwoPyOpByteCode::OpCode::PRINT:           return "PRINT";
            case TwoPyOpByteCode::OpCode::ADD:             return "ADD";
            case TwoPyOpByteCode::OpCode::SUB:             return "SUB";
            case TwoPyOpByteCode::OpCode::MUL:             return "MUL";
            case TwoPyOpByteCode::OpCode::DIV:             return "DIV";
            case TwoPyOpByteCode::OpCode::POP:             return "POP";
            case TwoPyOpByteCode::OpCode::PUSH:            return "PUSH";
            case TwoPyOpByteCode::OpCode::MAKE_FUNCTION:   return "MAKE_FUNCTION";
            case TwoPyOpByteCode::OpCode::PUSH_NULL:       return "PUSH_NULL";
            case TwoPyOpByteCode::OpCode::CALL_FUNCTION:   return "CALL_FUNCTION";
            case TwoPyOpByteCode::OpCode::STORE_VARIABLE:  return "STORE_NAME";
            case TwoPyOpByteCode::OpCode::STORE_FAST:      return "STORE_FAST";
            case TwoPyOpByteCode::OpCode::LOAD_VARIABLE:   return "LOAD_NAME";
            case TwoPyOpByteCode::OpCode::LOAD_FAST:       return "LOAD_FAST";
            case TwoPyOpByteCode::OpCode::LOAD_CONSTANT:   return "LOAD_CONST";
            case TwoPyOpByteCode::OpCode::BINARY_POWER:    return "BINARY_POWER";
        }
        return "UNKNOWN";
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return "ERROR";
    }
}

inline std::string value_to_string(const TwoPyOpByteCode::Value& val) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "";
        } else if constexpr (std::is_same_v<T, long>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + v + "\"";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<TwoObject::function_object>>) {
            return "<function " + v->name + ">";
        }
        return "<unknown>";
    }, val);
}

inline void print_bytecode(const TwoPyOpByteCode::FullByteCode& bytecode, const std::string& name = "") {
    if (!name.empty()) {
        fmt::print("\nDisassembly of {}:\n", name);
    } else {
        fmt::print("\nDisassembly:\n");
    }

    for (std::size_t i = 0; i < bytecode.instructions.size(); ++i) {
        const auto& entry = bytecode.instructions[i];

        fmt::print("{:>4} ", i * 2);

        fmt::print("{:<20}", opcode_name(entry.opcode));

        if (entry.opcode == TwoPyOpByteCode::OpCode::LOAD_CONSTANT ||
            entry.opcode == TwoPyOpByteCode::OpCode::LOAD_VARIABLE ||
            entry.opcode == TwoPyOpByteCode::OpCode::STORE_VARIABLE ||
            entry.opcode == TwoPyOpByteCode::OpCode::LOAD_FAST ||
            entry.opcode == TwoPyOpByteCode::OpCode::STORE_FAST ||
            entry.opcode == TwoPyOpByteCode::OpCode::MAKE_FUNCTION ||
            entry.opcode == TwoPyOpByteCode::OpCode::CALL_FUNCTION) {

            fmt::print("{:>4}", entry.argument);

            if (entry.opcode == TwoPyOpByteCode::OpCode::LOAD_CONSTANT &&
                entry.argument < bytecode.constants_pool.size()) {
                fmt::print(" ({})", value_to_string(bytecode.constants_pool[entry.argument]));
            } else if ((entry.opcode == TwoPyOpByteCode::OpCode::STORE_VARIABLE ||
                        entry.opcode == TwoPyOpByteCode::OpCode::LOAD_VARIABLE ||
                        entry.opcode == TwoPyOpByteCode::OpCode::STORE_FAST ||
                        entry.opcode == TwoPyOpByteCode::OpCode::LOAD_FAST) &&
                       entry.argument < bytecode.vars_pool.size()) {
                fmt::print(" ({})", bytecode.vars_pool[entry.argument]);
            } else if (entry.opcode == TwoPyOpByteCode::OpCode::MAKE_FUNCTION &&
                       entry.argument < bytecode.constants_pool.size()) {
                fmt::print(" ({})", value_to_string(bytecode.constants_pool[entry.argument]));
            }
        }

        fmt::print("\n");
    }
}

inline void print_all_bytecode(const std::vector<std::shared_ptr<TwoPyOpByteCode::FullByteCode>>& bytecode_list) {
    for (std::size_t i = 0; i < bytecode_list.size(); ++i) {
        std::string name = (i == 0) ? "<module>" : fmt::format("<function #{}>", i);
        print_bytecode(*bytecode_list[i], name);
    }
}

}

#endif
