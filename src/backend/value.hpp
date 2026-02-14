#ifndef TWOPY_VALUE_HPP
#define TWOPY_VALUE_HPP

#include <type_traits>
#include <utility>
#include <variant>
#include <string>

#include "backend/objects.hpp"

/* Credit to Derkt for the complex template programming
    Took me some time to figure out that the constructor takes in a rvalue
 */
namespace TwoPy::Backend {
    enum class ValueTag : uint8_t {
        NONE,   // C++ std::monostate
        BOOL,   // C++ long of 0 or 1
        INT,    // C++ long
        FLOAT,  // C++ double
        REF,    // Value reference
        OBJ,    // Python object
    };

    class Value;

    using Reference = Value*;

    class Value {
    public:
        using py_object_ptr = ObjectBase*;
        using hidden_data = std::variant<std::monostate, long, double, Reference, py_object_ptr>;
    private:
        template <typename NativeType>
        struct native_type_tag {
            static constexpr auto value = ValueTag::NONE;
        };

        hidden_data m_data;

    public:
        constexpr Value() noexcept (std::is_nothrow_default_constructible_v<hidden_data>)
        : m_data {} {}

        /**
         * @brief Universal constructor for non-None Values in TwoPy. These values need an argument of a C++ type to initialize.
         * 
         * @tparam NativeT Must be accepted by `Value::hidden_data` for construction.
         * @param d 
         */
        template <typename NativeT> requires requires (hidden_data d) {d = NativeT {};}
        constexpr Value(NativeT&& native_value)
        noexcept (std::is_nothrow_constructible_v<hidden_data, NativeT>)
        : m_data (std::forward<NativeT>(native_value)) {}

        explicit constexpr Value(py_object_ptr obj_ptr) noexcept
        : m_data(obj_ptr) {}

        [[nodiscard]] constexpr bool is_truthy() const noexcept {
            if (std::holds_alternative<long>(m_data)) {
                return std::get<long>(m_data) != 0L;
            } else if (std::holds_alternative<double>(m_data)) {
                return std::get<double>(m_data) != 0.0;
            } else if (std::holds_alternative<Reference>(m_data)) {
                return std::get<Reference>(m_data) != nullptr;
            } else if (std::holds_alternative<py_object_ptr>(m_data)) {
                return std::get<py_object_ptr>(m_data) != nullptr;
            }
            return false;
        }

        constexpr long to_long(this auto&& self) noexcept {
            if (std::holds_alternative<long>(self.m_data)) {
                return std::get<long>(self.m_data);
            } else if (std::holds_alternative<double>(self.m_data)) {
                return std::get<double>(self.m_data);
            } else if (std::holds_alternative<Reference>(self.m_data)) {
                return std::get<Reference>(self.m_data)->to_long();
            }

            return 0.0;
        }

        constexpr double to_double(this auto&& self) noexcept {
            if (std::holds_alternative<long>(self.m_data)) {
                return std::get<long>(self.m_data);
            } else if (std::holds_alternative<double>(self.m_data)) {
                return std::get<double>(self.m_data);
            } else if (std::holds_alternative<Reference>(self.m_data)) {
                return std::get<Reference>(self.m_data)->to_double();
            }

            return 0.0;
        }

        std::string to_string(this auto&& self) {
            switch (self.m_tag) {
                case ValueTag::NONE:
                    return "None";
                case ValueTag::BOOL:
                    return std::get<long>(self.m_data) != 0L ? "True" : "False";
                case ValueTag::INT:
                    return std::to_string(std::get<long>(self.m_data));
                case ValueTag::FLOAT:
                    return std::to_string(std::get<double>(self.m_data));
                case ValueTag::REF: {
                    auto ref = std::get<Reference>(self.m_data);
                    return ref->to_string();
                }
                case ValueTag::OBJ: {
                    auto obj = std::get<py_object_ptr>(self.m_data);
                    if (obj) {
                        return obj->stringify();
                    }
                    return "<null>";
                }
            }
            return "";
        }

        constexpr Reference ref() const noexcept {
            if (auto reference_p = std::get_if<Reference>(&m_data); reference_p) {
                return *reference_p;
            }

            return nullptr;
        }

        constexpr py_object_ptr obj_ref() const noexcept {
            if (auto py_object_p = std::get_if<py_object_ptr>(&m_data); py_object_p) {
                return *py_object_p;
            }

            return nullptr;
        }

        constexpr ValueTag tag(this auto&& self) noexcept {
            return self.m_tag;
        }

        [[nodiscard]] constexpr bool is_truthy() const noexcept {
            switch (m_tag) {
            case ValueTag::NONE:
                return false;
            case ValueTag::BOOL:
            case ValueTag::INT:
                return std::get<long>(m_data) != 0L;
            case ValueTag::FLOAT:
                return std::get<double>(m_data) != 0.0;
            case ValueTag::REF:
                return std::get<Reference>(m_data) != nullptr;
            case ValueTag::OBJ: {
                auto obj = std::get<py_object_ptr>(m_data);
                return obj->is_truthy();
            }
            }
            return false;
        }                                 

        /* Derkt told me this will be useful for when I start making my vm and it needs to pop and push items*/
/* 
        Value operator+(const Value& other) {
            switch (m_tag) {
            case ValueTag::NONE: return Value {};
            case ValueTag::BOOL:
            case ValueTag::INT: return std::get<long>(m_data) != 0L;
            case ValueTag::FLOAT: return std::get<double>(m_data) != 0.0;
            case ValueTag::REF: return std::get<Reference>(m_data) != nullptr;
            case ValueTag::OBJ: return std::get<py_object_ptr>(m_data) != nullptr; // add abstract is_truthy() method for any ObjectBase behind py_object_ptr
            default: return false;
            }

            return *this;
        }

        Value operator-(const Value& other) {
            switch (m_tag) {
            case ValueTag::NONE: return Value {};
            case ValueTag::BOOL:
            case ValueTag::INT: return std::get<long>(m_data) != 0L;
            case ValueTag::FLOAT: return std::get<double>(m_data) != 0.0;
            case ValueTag::REF: return std::get<Reference>(m_data) != nullptr;
            case ValueTag::OBJ: return std::get<py_object_ptr>(m_data) != nullptr; // add abstract is_truthy() method for any ObjectBase behind py_object_ptr
            default: return false;
            }

            return *this;
        } */

        /// TODO: add  %, *, and / overloads with div_int()...
    };

    /// NOTE: I got some weird issues with g++ issues with 13 on WSL. After upgrading g++ to 14 it worked but there were some bugs
    template <>
    struct Value::native_type_tag<long> {
        static constexpr auto value = ValueTag::INT;
    };

    template <>
    struct Value::native_type_tag<double> {
        static constexpr auto value = ValueTag::FLOAT;
    };

    template <>
    struct Value::native_type_tag<Reference> {
        static constexpr auto value = ValueTag::REF;
    };

    template <>
    struct Value::native_type_tag<Value::py_object_ptr> {
        static constexpr auto value = ValueTag::OBJ;
    };
}

#endif