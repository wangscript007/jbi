#ifndef JBI_INTERPRETER_SYNTAX_TREE_ARITHMETIC_OPERATOR
#define JBI_INTERPRETER_SYNTAX_TREE_ARITHMETIC_OPERATOR

#include <jbi/interpreter/syntax_tree/expression.h>
#include <jbi/interpreter/arithmetic_operation.h>

#include <memory>

namespace jbi
{

    class arithmetic_operator : public expression
    {
    private:
        arithmetic_operation        _operation;

        std::unique_ptr<expression> _left;
        std::unique_ptr<expression> _right;

    public:
        arithmetic_operator(arithmetic_operation operation, std::unique_ptr<expression> left, std::unique_ptr<expression> right)
            : _operation(operation), _left(std::move(left)), _right(std::move(right))
        {
            JBI_THROW_IF(!_left, argument_exception("left"));
            JBI_THROW_IF(!_right, argument_exception("right"));
        }

        arithmetic_operation operation() const noexcept { return _operation; }

        const std::unique_ptr<expression>& left() const noexcept  { return _left; }
        const std::unique_ptr<expression>& right() const noexcept { return _right; }

        JBI_DEFINE_VISITABLE(arithmetic_operator)
    };

}

#endif
