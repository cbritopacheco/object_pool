#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "check if a pool contains no free objects", "[object_pool]" )
{
    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        THEN ("empty == true")
        {
            REQUIRE( pool.empty() == true );
        }
    }

    GIVEN ("A pool of 100 objects")
    {
        carlosb::object_pool<string> pool(100);

        THEN ("empty == false")
        {
            REQUIRE( pool.empty() == false );
        }
    }
}