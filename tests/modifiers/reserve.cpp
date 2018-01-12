#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "space can be reserved", "[object_pool]" )
{

    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        REQUIRE( pool.size() == 0 );
        REQUIRE( pool.capacity() >= 0 );
        
        WHEN ( "10 spaces are reserved" )
        {
            pool.reserve(10);
            
            THEN ( "the size doesn't change" )
            {
                REQUIRE( pool.size() == 0 );
                
            }

            THEN ( "the capacity changes" )
            {
                REQUIRE( pool.capacity() == 10 );
            }

            THEN ( "the pool is empty" )
            {
                REQUIRE ( pool.empty() == true );
                REQUIRE ( !static_cast<bool>(pool) );
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

        size_t prev_capacity = pool.capacity();

        REQUIRE( pool.size() == 100 );
        REQUIRE( pool.capacity() >= 100 );


        WHEN ( "10 spaces are reserved" )
        {
            pool.reserve(10);
            
            THEN ( "the size does not change" )
            {
                REQUIRE( pool.size() == 100 );
            }

            THEN ( "the capacity does not change" )
            {
                REQUIRE( pool.capacity() == prev_capacity );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }
}