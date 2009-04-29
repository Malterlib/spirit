/*=============================================================================
    Copyright (c) 2001-2009 Hartmut Kaiser
    Copyright (c) 2001-2009 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_PASS_CONTAINER_MAR_15_2009_0114PM)
#define SPIRIT_PASS_CONTAINER_MAR_15_2009_0114PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    // has_same_elements: utility to check if the RHS attribute
    // is an STL container and that its value_type is convertible
    // to the LHS.

    template <typename RHS, typename LHSAttribute
      , bool IsContainer = traits::is_container<LHSAttribute>::value>
    struct has_same_elements : mpl::false_ {};

    template <typename RHS, typename LHSAttribute>
    struct has_same_elements<RHS, LHSAttribute, true>
      : mpl::or_<
            is_convertible<typename LHSAttribute::value_type, RHS>
          , is_same<typename LHSAttribute::value_type, hold_any>
        > {};

#define BOOST_SPIRIT_IS_CONVERTIBLE(z, N, data)                               \
        is_convertible<BOOST_PP_CAT(T, N), RHS>::value ||                     \
        is_same<BOOST_PP_CAT(T, N), hold_any>::value ||                       \
    /***/

    // Note: variants are treated as containers if one of the held types is a
    //       container (see support/container.hpp).
    template <typename RHS, BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct has_same_elements<
            RHS, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, true>
      : mpl::bool_<BOOST_PP_REPEAT(BOOST_VARIANT_LIMIT_TYPES
          , BOOST_SPIRIT_IS_CONVERTIBLE, _) false> {};

#undef BOOST_SPIRIT_IS_CONVERTIBLE

    // This function handles the case where the attribute (Attr) given
    // to the sequence is an STL container. This is a wrapper around F.
    // The function F does the actual generating.
    template <typename F, typename Attr>
    struct pass_container
    {
        typedef typename F::context_type context_type;

        pass_container(F const& f, Attr& attr)
          : f(f), attr(attr), iter(traits::begin(attr)) {}

        // this is for the case when the current element expects an attribute
        // which is taken from the next entry in the container
        template <typename Component>
//        bool dispatch_attribute_element(Component const& component, mpl::false_) const
        bool dispatch_attribute(Component const& component, mpl::true_) const
        {
            // get the next value to generate from container
            if (!traits::compare(iter, traits::end(attr)) &&
                !f(component, traits::deref(iter))) 
            {
                // needs to return false as long as everything is ok
                traits::next(iter);
                return false;
            }
            // either no elements available any more or generation failed
            return true;
        }

//        // this is for the case when the current element expects an attribute
//        // which is a container itself, this element will get the rest of the 
//        // attribute container
//        template <typename Component>
//        bool dispatch_attribute_element(Component const& component, mpl::true_) const
//        {
//            typedef typename 
//                traits::attribute_of<Component, context_type>::type
//            attribute_type;
//            typedef typename 
//                traits::container_type<attribute_type>::type
//            container_type;
// 
//            bool result = f(component, container_type(iter, traits::end(attr)));
//            if (result)
//                iter = traits::end(attr);     // adjust current iter to the end 
//            return result;
//        }
// 
//        // This handles the distinction between elements in a sequence expecting
//        // containers themselves and elements expecting non-containers as their 
//        // attribute. Note: is_container treats optional<T>, where T is a 
//        // container as a container as well.
//        template <typename Component>
//        bool dispatch_attribute(Component const& component, mpl::true_) const
//        {
//            typedef traits::is_container<
//                typename traits::attribute_of<Component, context_type>::type
//            > predicate;
// 
//            return dispatch_attribute_element(component, predicate());
//        }

        // this is for the case when the current element doesn't expect an 
        // attribute
        template <typename Component>
        bool dispatch_attribute(Component const& component, mpl::false_) const
        {
            return f(component, unused);
        }

        // This handles the case where the attribute of the component
        // is not a STL container or which elements are not 
        // convertible to the target attribute's (Attr) value_type.
        template <typename Component>
        bool dispatch_main(Component const& component, mpl::false_) const
        {
            // we need to dispatch again depending on the type of the attribute
            // of the current element (component). If this is has no attribute
            // we shouldn't use an element of the container but unused_type as
            // well
            typedef traits::is_not_unused<
                typename traits::attribute_of<Component, context_type>::type
            > predicate;

            return dispatch_attribute(component, predicate());
        }

        // This handles the case where the attribute of the component is
        // an STL container *and* its value_type is convertible to the
        // target attribute's (Attr) value_type.
        template <typename Component>
        bool dispatch_main(Component const& component, mpl::true_) const
        {
            return f(component, attr);
        }

        // Dispatches to dispatch_main depending on the attribute type
        // of the Component
        template <typename Component>
        bool operator()(Component const& component) const
        {
            typedef typename traits::result_of::value<Attr>::type rhs;
            typedef typename traits::attribute_of<
                Component, context_type>::type lhs_attribute;

            return dispatch_main(component
              , has_same_elements<rhs, lhs_attribute>());
        }

        F f;
        Attr const& attr;
        mutable typename traits::result_of::iterator<Attr>::type iter;
    };

    // Utility function to make a pass_container
    template <typename F, typename Attr>
    pass_container<F, Attr>
    inline make_pass_container(F const& f, Attr& attr)
    {
        return pass_container<F, Attr>(f, attr);
    }

}}}}

#endif