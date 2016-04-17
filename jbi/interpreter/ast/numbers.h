#ifndef JBI_INTERPRETER_AST_NUMBERS_H
#define JBI_INTERPRETER_AST_NUMBERS_H

#include <jbi/interpreter/ast/expression.h>

namespace jbi
{

    namespace detail
    {

        template < typename T >
        class number : public expression
        {
        private:
            T _value;

        public:
            explicit number(T value)
                : _value(value)
            { }

            T value() const { return _value; }
        };

    }

    using integer_number = detail::number<int>;
    using floating_point_number = detail::number<double>;

}

#endif
