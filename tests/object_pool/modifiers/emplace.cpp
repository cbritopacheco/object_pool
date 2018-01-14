#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "objects can be emplaced into the pool", "[object_pool]" )
{

    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        REQUIRE( pool.size() == 0 );
        REQUIRE( pool.capacity() >= 0 );
        
        WHEN ( "an object is constructed in place" )
        {
            pool.emplace("a new object");
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 1 );
                REQUIRE( pool.capacity() >= 1 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }

    GIVEN ("A pool of 100 objects that are default constructible")
    {
        struct DefaultConstructible
        {
            DefaultConstructible() : v("") {}
            DefaultConstructible(string _v) : v(_v) {}
            string v;
        };

        carlosb::object_pool<DefaultConstructible> pool(100);

        REQUIRE( pool.size() == 100 );
        REQUIRE( pool.capacity() >= 100 );

        WHEN ( "a value is constructed in place" )
        {
            pool.emplace("a new object");
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 101 );
                REQUIRE( pool.capacity() >= 101 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }

    GIVEN ("A pool of 100 objects that are default initialized")
    {
        struct DefaultConstructible
        {
            DefaultConstructible() : v(0) {}
            DefaultConstructible(string _v) : v(_v) {}
            string v;
        };

        carlosb::object_pool<DefaultConstructible> pool(100, string("default initialized strings"));

        REQUIRE( pool.size() == 100 );
        REQUIRE( pool.capacity() >= 100 );

        WHEN ( "a value is constructed in place" )
        {
            pool.emplace("constructed in place");
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 101 );
                REQUIRE( pool.capacity() >= 101 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }

    GIVEN ("An empty pool of DefaultConstructible objects")
    {
        struct DefaultConstructible
        {
            DefaultConstructible() : v(0) {}
            DefaultConstructible(string _v) : v(_v) {}
            string v;
        };

        carlosb::object_pool<DefaultConstructible> pool;

        REQUIRE( pool.size() == 0 );
        REQUIRE( pool.capacity() >= 0 );

        WHEN ( "When a bunch of objects are emplaced (n=150)" )
        {
            for (int i = 0; i < 150; ++i)
                pool.emplace("hello");
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 150 );
                REQUIRE( pool.capacity() >= 150 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }
}