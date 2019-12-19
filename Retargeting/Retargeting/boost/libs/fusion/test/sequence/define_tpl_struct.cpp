/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/support.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/static_assert.hpp>
#include <iostream>
#include <string>

BOOST_FUSION_DEFINE_TPL_STRUCT(
    (X)(Y),
    (ns),
    point,
    (X, x)
    (Y, y)
)

BOOST_FUSION_DEFINE_TPL_STRUCT((M), BOOST_PP_EMPTY(), s, (M, m))

BOOST_FUSION_DEFINE_TPL_STRUCT((M), BOOST_PP_EMPTY(), empty_struct, )

int
main()
{
    using namespace boost::fusion;

    typedef ns::point<int, int> point;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        BOOST_MPL_ASSERT_NOT((traits::is_view<point>));
        BOOST_STATIC_ASSERT(!traits::is_view<point>::value);
        point p(123, 456);

        std::cout << at_c<0>(p) << std::endl;
        std::cout << at_c<1>(p) << std::endl;
        std::cout << p << std::endl;
        BOOST_TEST(p == make_vector(123, 456));

        at_c<0>(p) = 6;
        at_c<1>(p) = 9;
        BOOST_TEST(p == make_vector(6, 9));

        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<point>::value == 2);
        BOOST_STATIC_ASSERT(!boost::fusion::result_of::empty<point>::value);

        BOOST_TEST(front(p) == 6);
        BOOST_TEST(back(p) == 9);
    }

    {
        vector<int, float> v1(4, 2.f);
        point v2(5, 3);
        vector<long, double> v3(5, 4.);
        BOOST_TEST(v1 < v2);
        BOOST_TEST(v1 <= v2);
        BOOST_TEST(v2 > v1);
        BOOST_TEST(v2 >= v1);
        BOOST_TEST(v2 < v3);
        BOOST_TEST(v2 <= v3);
        BOOST_TEST(v3 > v2);
        BOOST_TEST(v3 >= v2);
    }

    {
        // conversion from point to vector
        point p(5, 3);
        vector<int, long> v(p);
        v = p;
    }

    {
        // conversion from point to list
        point p(5, 3);
        list<int, long> l(p);
        l = p;
    }

    { // begin/end
        using namespace boost::fusion;

        typedef boost::fusion::result_of::begin<s<int> >::type b;
        typedef boost::fusion::result_of::end<s<int> >::type e;
        // this fails
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::next<b>::type, e>));
    }


    {
        point p = make_list(5,3);
        BOOST_TEST(p == make_vector(5,3));

        p = make_list(3,5);
        BOOST_TEST(p == make_vector(3,5));
    }

    return boost::report_errors();
}

