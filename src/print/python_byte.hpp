#ifndef PYTHON_BYTE_HPP
#define PYTHON_BYTE_HPP

#include <fmt/core.h>
#include "backend/bytecode.hpp"

namespace BytePrinter {

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
            case TwoPyOpByteCode::OpCode::STORE_VARIABLE:  return "STORE_NAME";
            case TwoPyOpByteCode::OpCode::LOAD_VARIABLE:   return "LOAD_NAME";
            case TwoPyOpByteCode::OpCode::LOAD_CONSTANT:   return "LOAD_CONST";
        }
        return "UNKNOWN";
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
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
        } else if constexpr (std::is_same_v<T, TwoObject::RuntimeDetection>) {
            return "<function>";
        }
    }, val);
}

inline void print_bytecode(const TwoPyOpByteCode::FullByteCode& bytecode) {
    fmt::print("== Bytecode ==\n");
    for (std::size_t i = 0; i < bytecode.instructions.size(); ++i) {
        const auto& entry = bytecode.instructions[i];
        fmt::print("{:>4}  {:<16} {:>4}", i * 2, opcode_name(entry.opcode), entry.argument);

        if (entry.opcode == TwoPyOpByteCode::OpCode::LOAD_CONSTANT && entry.argument < bytecode.constants_pool.size()) {
            fmt::print("    ({})", value_to_string(bytecode.constants_pool[entry.argument]));
        } else if ((entry.opcode == TwoPyOpByteCode::OpCode::STORE_VARIABLE ||
                    entry.opcode == TwoPyOpByteCode::OpCode::LOAD_VARIABLE) && entry.argument < bytecode.vars_pool.size()) {
            fmt::print("    ({})", bytecode.vars_pool[entry.argument]);
        }

        fmt::print("\n");
    }
}

}

#endif
