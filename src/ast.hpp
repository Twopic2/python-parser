#ifndef AST_HPP
#define AST_HPP 

#include <memory>

namespace Ast {
    class ast_class {
        private:
            template<typename T>
            struct Node {
                T data;
                std::unique_ptr<Node> left;
                std::unique_ptr<Node> right;
            };
        public:
    };
}

#endif