#ifndef JBI_INTERPRETER_EVALUATOR_VALUE_PRINTER_H
#define JBI_INTERPRETER_EVALUATOR_VALUE_PRINTER_H

#include <jbi/core/exceptions.h>
#include <jbi/interpreter/iterminal.h>
#include <jbi/interpreter/types/numeric_range.h>
#include <jbi/variant/static_visitor.h>

#include <memory>

namespace jbi
{

    class value_printer : public static_visitor<>
    {
    private:
        std::shared_ptr<iterminal> _terminal;

    public:
        explicit value_printer(std::shared_ptr<iterminal> terminal)
            : _terminal(std::move(terminal))
        {
            JBI_THROW_IF(!_terminal, argument_exception("terminal"));
        }

        template < typename T >
        void operator()(const T& value)
        {
            _terminal->write_line(to_string(value));
        }

        template < typename T >
        void operator()(const numeric_range<T>& range)
        {
            _terminal->write_line("[" + to_string(range.start()) + ", " + to_string(range.stop()) + ")");
        }
    };

}

#endif
