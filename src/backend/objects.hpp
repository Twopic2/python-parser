#ifndef OBJECTS_HPP
#define OBJECT_HPP

#include <string>
#include <vector>

namespace TwoObject {
    enum class RuntimeDetection {
        FUNCTION,
    };

    /* Insprided by Derkt's ObjectBase class which allows for Polymophric virutal representation */
    struct ObjectBase {
        RuntimeDetection object_type;
        virtual ~ObjectBase() = default;
    };

    class function_object : ObjectBase {
        public:
            std::string name;
            std::vector<std::string> parmas;
    };

}

#endif