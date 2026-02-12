#ifndef TWOPY_OBJECTS_HPP
#define TWOPY_OBJECTS_HPP

#include <string>

/* Each of these would be local bytecode scope */
namespace TwoPy::Backend {
    class Value;

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

        /// NOTE: mutable accessor for impl. of __get__
        virtual Value& operator[](const Value&) = 0;

        /// NOTE: immutable accessor for impl. of __get__
        virtual const Value& operator[](const Value&) const = 0;

        /// NOTE: The `vm_state` argument must hide a `VMContext*` to affect the stack of. This `void*` trick & reinterpret_cast is needed to dodge circular inclusions- What if the VM uses the Value & object types but those must know of the VM internals? @DrkWithT
        /// NOTE: for __call__(self, args)
        virtual bool invoke([[maybe_unused]] void* vm_state, [[maybe_unused]] uint8_t arg_count) = 0;

        /// NOTE: for __str__(self)
        virtual std::string stringify() = 0;
    };

    class function_object : public ObjectBase {

    };

    class StringPyObject : public ObjectBase {

    };
}

#endif
