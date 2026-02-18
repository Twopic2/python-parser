#ifndef TWOPY_OBJECTS_HPP
#define TWOPY_OBJECTS_HPP

#include <string>
#include <memory>
#include <type_traits>

#include <fmt/core.h>

/* Each of these would be local bytecode scope */
namespace TwoPy::Backend {
    //class Value;

    /* These would be polymorphic heap object and wouldn't be primitives such as floats, bools, and ints */

    /* PyObjects get compiled from the PVM and it detects whether its a default value or heap based */    

    enum class ObjectTag : uint8_t {
        NONE,       // non-object
        // LIST,
        // DICT,
        // CLASS,
        FUNCTION,   // callable object
        STRING,
    };

    /* Insprided by Derkt's ObjectBase class which allows for Polymophric virutal representation */
    struct ObjectBase {
        virtual ~ObjectBase() = default;

        virtual ObjectTag tag() const noexcept = 0;

        /* Indexing */
        /// NOTE: immutable accessor for impl. of __get__
        // virtual const Value& operator[](const Value&) const = 0;

        /// NOTE: The `vm_state` argument must hide a `VMContext*` to affect the stack of. 
        // This `void*` trick & reinterpret_cast is needed to dodge circular inclusions- What if the VM uses the Value & object types but those must know of the VM internals? @DrkWithT
        /// NOTE: for __call__(self, args)
        /* I'll implament this when I start on my vm */
        //virtual bool call([[maybe_unused]] void* vm_state, [[maybe_unused]] uint8_t arg_count) = 0;

       /// NOTE: for __str__(self) converts different types to string
        virtual std::string stringify() = 0;

        virtual bool is_truthy() const noexcept = 0;
    };

    class FunctionPyObject : public ObjectBase {
        private:
            std::string name;
            std::vector<std::string> params;
            /* Chunk func_init {};
 */
        public:
            explicit FunctionPyObject(std::string& data, std::vector<std::string>& vec) : name(std::move(data)), params(std::move(vec)) {}

            ObjectTag tag() const noexcept override {
                return ObjectTag::FUNCTION;
            }

            [[nodiscard]] std::string stringify() override {
                return fmt::format("Function {}", name);
            }

            [[nodiscard]] bool is_truthy() const noexcept override {
                return !name.empty();
            }
    };

    class StringPyObject : public ObjectBase {
        private:
            std::string m_data {};

        public:
            explicit StringPyObject(std::string data) : m_data(std::move(data)) {}

            ObjectTag tag() const noexcept override {
                return ObjectTag::STRING;
            }

            // const Value& operator[](const Value&) const override {
                
            // }

            /* [[nodiscard]] bool call([[maybe_unused]] void* vm_state, [[maybe_unused]] uint8_t arg_count) override {
                return false;
            }*/

            std::string stringify() override {
                return m_data;
            }

            // Empty strings are falsy, non-empty strings are truthy (Python behavior)
            bool is_truthy() const noexcept override {
                return !m_data.empty();
            }
    };
}

#endif
