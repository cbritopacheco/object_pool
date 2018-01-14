#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "check if a pool contains free objects", "[object_pool]" )
{
    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        THEN ("operator bool == false")
        {
            REQUIRE( static_cast<bool>(pool) == false );
        }
    }

    GIVEN ("A pool of 100 objects and no acquisitions")
    {
        carlosb::object_pool<string> pool(100);

        THEN ("operator bool == true")
        {
            REQUIRE( static_cast<bool>(pool) == true );
        }
    }
}