#include <gtest/gtest.h>

#include <jbi/interpreter/exceptions.h>
#include <jbi/interpreter/syntactic_analyser.h>
#include <jbi/interpreter/syntax_tree/visitor.h>
#include <jbi/visitor/accept_visitor.h>

struct expression_lispifier : public jbi::expression_visitor<std::string>
{
    std::string operator()(const jbi::arithmetic_operator& op) const
    {
        return "(" + jbi::to_string(op.operation()) + " " + jbi::accept_visitor(*this, *op.left()) + " " + jbi::accept_visitor(*this, *op.right()) + ")";
    }

    template < typename T >
    std::string operator()(const jbi::numeric_literal<T>& literal) const
    {
        return jbi::to_string(literal.value());
    }

    std::string operator()(const jbi::identifier& identifier) const
    {
        return identifier.name();
    }

    std::string operator()(const jbi::range& range) const
    {
        return "(range " + jbi::accept_visitor(*this, *range.start()) + " " + jbi::accept_visitor(*this, *range.stop()) + ")";
    }
};

struct statement_lispifier : public jbi::statement_visitor<std::string>
{
    std::string operator()(const jbi::declaration_statement& var) const
    {
        return "(var " + var.identifier() + " " + jbi::accept_visitor(expression_lispifier(), *var.initializer()) + ")";
    }

    std::string operator()(const jbi::output_statement& out) const
    {
        return "(out " + jbi::accept_visitor(expression_lispifier(), *out.value()) + ")";
    }

    std::string operator()(const jbi::input_statement& in) const
    {
        return "(in " + in.identifier() + ")";
    }
};


std::string lispify(const std::string& statement)
{
    // TODO mock lexical_analyser
    jbi::syntactic_analyser parser{ jbi::lexical_analyser(statement) };
    return jbi::accept_visitor(statement_lispifier(), *parser.parse());
}

TEST(syntactic_analyser_tests, declaration_statement_test)
{
    EXPECT_EQ("(var foo a)", lispify("var foo = a"));

    EXPECT_THROW(lispify("var"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var * = a"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var 1 = a"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var out = a"), jbi::syntax_exception);
}

TEST(syntactic_analyser_tests, output_statement_test)
{
    EXPECT_EQ("(out (+ a b))", lispify("out a + b"));

    EXPECT_THROW(lispify("out"), jbi::syntax_exception);
    EXPECT_THROW(lispify("out var"), jbi::syntax_exception);
}

TEST(syntactic_analyser_tests, input_statement_test)
{
    EXPECT_EQ("(in foo)", lispify("in foo"));

    EXPECT_THROW(lispify("in"), jbi::syntax_exception);
    EXPECT_THROW(lispify("in var"), jbi::syntax_exception);
    EXPECT_THROW(lispify("in foo bar"), jbi::syntax_exception);
}

TEST(syntactic_analyser_tests, literals_test)
{
    EXPECT_EQ("(var foo (+ 1948 3.14))", lispify("var foo = 1948 + 3.14"));

    EXPECT_THROW(lispify("var foo = \"foo\""), jbi::syntax_exception);
}

TEST(syntactic_analyser_tests, associativity_test)
{
    EXPECT_EQ("(var foo (+ (+ a b) c))", lispify("var foo = a + b + c"));
    EXPECT_EQ("(var foo (- (- a b) c))", lispify("var foo = a - b - c"));
    EXPECT_EQ("(var foo (+ (- a b) c))", lispify("var foo = a - b + c"));

    EXPECT_EQ("(var foo (* (* a b) c))", lispify("var foo = a * b * c"));
    EXPECT_EQ("(var foo (/ (/ a b) c))", lispify("var foo = a / b / c"));
    EXPECT_EQ("(var foo (* (/ a b) c))", lispify("var foo = a / b * c"));

    EXPECT_EQ("(var foo (^ a (^ b c)))", lispify("var foo = a ^ b ^ c"));
}

TEST(syntactic_analyser_tests, precedence_test)
{
    EXPECT_EQ("(var foo (+ a (* b (^ c d))))", lispify("var foo = a + b * c ^ d"));
    EXPECT_EQ("(var foo (+ a (/ b (^ c d))))", lispify("var foo = a + b / c ^ d"));
    EXPECT_EQ("(var foo (- a (* b (^ c d))))", lispify("var foo = a - b * c ^ d"));
    EXPECT_EQ("(var foo (- a (/ b (^ c d))))", lispify("var foo = a - b / c ^ d"));
    EXPECT_EQ("(var foo (- a (/ b (^ c d))))", lispify("var foo = a - b / c ^ d"));
}

TEST(syntactic_analyser_tests, parentheses_test)
{
    EXPECT_EQ("(var foo (/ (* a (- b c)) d))", lispify("var foo = (a * (b - c)) / d"));

    EXPECT_THROW(lispify("var foo = a + (b * c"), jbi::syntax_exception);
}

TEST(syntactic_analyser_tests, range_test)
{
    EXPECT_EQ("(var foo (range (+ a b) (* c d)))", lispify("var foo = { a + b, c * d }"));

    EXPECT_THROW(lispify("var foo = { , b }"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var foo = { a, }"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var foo = { , }"), jbi::syntax_exception);
    EXPECT_THROW(lispify("var foo = { a b }"), jbi::syntax_exception);
}
