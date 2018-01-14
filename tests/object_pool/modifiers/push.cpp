#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "objects can be pushed stringo the pool", "[object_pool]" )
{

    GIVEN( "An empty pool of strings" )
    {
        carlosb::object_pool<string> pool;
        
        REQUIRE( pool.size() == 0 );
        REQUIRE( pool.capacity() >= 0 );
        
        WHEN ( "an l-value is pushed" )
        {
            string lv = "l-value";
            pool.push(lv);
            
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

        WHEN ( "an r-value is pushed" )
        {
            pool.push(string("r-value"));
            
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

        WHEN ( "an l-value is pushed" )
        {
            DefaultConstructible lv("l-value");
            pool.push(lv);
            
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

        WHEN ( "an r-value is pushed" )
        {
            pool.push(DefaultConstructible("r-value"));
            
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

        carlosb::object_pool<DefaultConstructible> pool(100, string("default initialized"));

        REQUIRE( pool.size() == 100 );
        REQUIRE( pool.capacity() >= 100 );

        WHEN ( "an l-value is pushed" )
        {
            DefaultConstructible lv("l-value");
            pool.push(lv);
            
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

        WHEN ( "an r-value is pushed" )
        {
            pool.push(DefaultConstructible("r-value"));
            
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

        WHEN ( "When a bunch of objects are pushed (n=150)" )
        {

            // add r-value
            for (int i = 0; i < 75; ++i)
                pool.push(DefaultConstructible("r-value"));

            // add l-value
            auto lv = DefaultConstructible("l-value");
            for (int i = 0; i < 75; ++i)
                pool.push(lv);
            
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