#ifndef JBI_VARIANT_VARIANT_H
#define JBI_VARIANT_VARIANT_H

#include <jbi/algorithm/pp.h>
#include <jbi/core/exceptions.h>
#include <jbi/core/type_traits.h>
#include <jbi/variant/static_visitor.h>

#include <memory>
#include <typeinfo>

namespace jbi
{

    namespace detail
    {

        template < typename... Ts >
        struct variant_storage
        {
        private:
            aligned_union_t<0, Ts...>   _storage;

        public:
            void* address()
            {
                return std::addressof(_storage);
            }

            const void* address() const
            {
                return std::addressof(_storage);
            }

            template < typename T >
            T& value() &
            {
                return static_cast<T&>(*static_cast<T*>(address()));
            }

            template < typename T >
            const T& value() const &
            {
                return static_cast<const T&>(*static_cast<const T*>(address()));
            }

            template < typename T >
            T&& value() &&
            {
                return static_cast<T&&>(*static_cast<T*>(address()));
            }
        };


        template < typename Visitor, typename Storage, typename Head, typename... Tail >
        return_type_of_decayed<Visitor> apply_visitor(Visitor&& visitor, Storage&& storage, std::size_t index, parameter_pack<Head, Tail...>)
        {
            if (index == 0)
                return std::forward<Visitor>(visitor)(std::forward<Storage>(storage).template value<Head>());

            return apply_visitor(std::forward<Visitor>(visitor), std::forward<Storage>(storage), index - 1, parameter_pack<Tail...>());
        };

        template < typename Visitor, typename Storage >
        return_type_of_decayed<Visitor> apply_visitor(Visitor&&, Storage&&, std::size_t, parameter_pack<>)
        {
            JBI_THROW(std::logic_error("Should never get here"));
        };


        class value_initializer : public static_visitor<>
        {
        private:
            void* _address;

        public:
            explicit value_initializer(void* address)
                : _address(address)
            { }

            template < typename T >
            void operator()(T&& value)
            {
                using decayed_T = decay_t<T>;

                new(_address) decayed_T(std::forward<T>(value));
            }
        };

        class value_assigner : public static_visitor<>
        {
        private:
            void* _address;

        public:
            explicit value_assigner(void* address)
                : _address(address)
            { }

            template < typename T >
            void operator()(T&& value)
            {
                using decayed_T = decay_t<T>;

                static_cast<decayed_T&>(*static_cast<decayed_T*>(_address)) = std::forward<T>(value);
            }
        };


        struct variant_destroyer : public static_visitor<>
        {
            template < typename T >
            void operator()(T& value)
            {
                value.~T();
            }
        };

    }


    template < typename... Ts >
    class variant
    {
    private:
        std::size_t                     _which;
        detail::variant_storage<Ts...>  _storage;

    public:
        variant()
            : _which(0)
        {
            using T = pp::front<Ts...>;

            new(_storage.address()) T();
        }

        template < typename T, enable_if_t<!std::is_same<decay_t<T>, variant>::value, void>* = nullptr >
        variant(T&& value)
            : _which(pp::index_of<decay_t<T>, Ts...>::value)
        {
            using decayed_T = decay_t<T>;

            // TODO check convertibility instead of exact type
            static_assert(pp::index_of<decayed_T, Ts...>::value != pp::npos, "T should be one of the Ts");
            new(_storage.address()) decayed_T(std::forward<T>(value));
        }

        variant(const variant& other)
            : _which(other._which)
        {
            initialize_value(other);
        }

        variant(variant&& other)
            : _which(other._which)
        {
            initialize_value(other);
        }

        ~variant()
        {
            destroy_value();
        }

        variant& operator=(const variant& other)
        {
            assign(other);
        }

        variant& operator=(variant&& other)
        {
            assign(other);
        }

        std::size_t which() const
        {
            return _which;
        }

        template < typename Visitor >
        return_type_of_decayed<Visitor> apply_visitor(Visitor&& visitor) &
        {
            return detail::apply_visitor(std::forward<Visitor>(visitor), _storage, _which, parameter_pack<Ts...>());
        }

        template < typename Visitor >
        return_type_of_decayed<Visitor> apply_visitor(Visitor&& visitor) const &
        {
            return detail::apply_visitor(std::forward<Visitor>(visitor), _storage, _which, parameter_pack<Ts...>());
        }

        template < typename Visitor >
        return_type_of_decayed<Visitor> apply_visitor(Visitor&& visitor) &&
        {
            return detail::apply_visitor(std::forward<Visitor>(visitor), std::move(_storage), _which, parameter_pack<Ts...>());
        }

    private:
        template < typename Variant >
        void initialize_value(Variant&& other)
        {
            detail::value_initializer initializer(_storage.address());
            std::forward<Variant>(other).apply_visitor(initializer);
        }

        void destroy_value()
        {
            detail::variant_destroyer destroyer;
            apply_visitor(destroyer);
        }

        // FIXME not enough exception safe
        template < typename Variant >
        void assign(Variant&& other)
        {
            if (_which == other._which)
            {
                detail::value_assigner assigner(_storage.address());
                std::forward<Variant>(other).apply_visitor(assigner);
            }
            else
            {
                destroy_value();

                initialize_value(std::forward<Variant>(other));

                _which = other._which;
            }
        }
    };


    namespace detail
    {

        template < typename >
        struct is_variant : std::false_type
        { };

        template < typename... Ts >
        struct is_variant<variant<Ts...>> : std::true_type
        { };

    }

    template < typename Visitor, typename Variant >
    enable_if_t<
        detail::is_variant<decay_t<Variant>>::value, return_type_of_decayed<Visitor>
    > apply_visitor(Visitor&& visitor, Variant&& variant)
    {
        return std::forward<Variant>(variant).apply_visitor(std::forward<Visitor>(visitor));
    };


    namespace detail
    {

        template < typename Visitor, typename Variant >
        class multivisitor : public static_visitor<return_type_of_decayed<Visitor>>
        {
        public:
            using return_type = return_type_of_decayed<Visitor>;

        private:
            template < typename Left >
            class applier : public static_visitor<return_type>
            {
            private:
                Visitor&&   _visitor;
                Left&&      _left;

            public:
                applier(Visitor&& visitor, Left&& left)
                    : _visitor(std::forward<Visitor>(visitor)), _left(std::forward<Left>(left))
                { }

                template < typename Right >
                return_type operator()(Right&& right) const
                {
                    return std::forward<Visitor>(_visitor)(std::forward<Left>(_left), std::forward<Right>(right));
                }
            };

        private:
            Visitor&&   _visitor;
            Variant&&   _right;

        public:
            multivisitor(Visitor&& visitor, Variant&& right)
                : _visitor(std::forward<Visitor>(visitor)), _right(std::forward<Variant>(right))
            { }

            template < typename Left >
            return_type operator()(Left&& left) const
            {
                applier<Left> applier(std::forward<Visitor>(_visitor), std::forward<Left>(left));
                return apply_visitor(applier, std::forward<Variant>(_right));
            }
        };

    }

    template < typename Visitor, typename Variant >
    enable_if_t<
        detail::is_variant<decay_t<Variant>>::value, return_type_of_decayed<Visitor>
    > apply_visitor(Visitor&& visitor, Variant&& left, Variant&& right)
    {
        detail::multivisitor<Visitor, Variant> multivisitor(std::forward<Visitor>(visitor), std::forward<Variant>(right));
        return apply_visitor(multivisitor, std::forward<Variant>(left));
    };


    // TODO add descriptive message
    class bad_get : public std::bad_cast
    { };

    namespace detail
    {

        template < typename T >
        struct value_getter : public static_visitor<T>
        {
            T operator()(T value)
            {
                return static_cast<T>(value);
            }

            template < typename U >
            T operator()(const U&)
            {
                JBI_THROW(bad_get());
            }
        };

    }

    template < typename T, typename... Ts >
    T& get(variant<Ts...>& variant)
    {
        detail::value_getter<T&> getter;
        return apply_visitor(getter, variant);
    }

    template < typename T, typename... Ts >
    const T& get(const variant<Ts...>& variant)
    {
        detail::value_getter<const T&> getter;
        return apply_visitor(getter, variant);
    }

    template < typename T, typename... Ts >
    T&& get(variant<Ts...>&& variant)
    {
        detail::value_getter<T&&> getter;
        return apply_visitor(getter, std::move(variant));
    }


    namespace detail
    {

        template < typename... Ts >
        class value_equality_comparer : public static_visitor<bool>
        {
        private:
            const variant<Ts...>&  _variant;

        public:
            explicit value_equality_comparer(const variant<Ts...>& variant)
                : _variant(variant)
            { }

            template < typename T >
            bool operator()(const T& value)
            {
                return get<T>(_variant) == value;
            }
        };

    }

    template < typename... Ts >
    bool operator==(const variant<Ts...>& left, const variant<Ts...>& right)
    {
        if (left.which() != right.which())
            return false;

        detail::value_equality_comparer<Ts...> comparer(left);
        return apply_visitor(comparer, right);
    }

    template < typename... Ts >
    bool operator!=(const variant<Ts...>& left, const variant<Ts...>& right)
    {
        return !(left == right);
    }

}

#endif
