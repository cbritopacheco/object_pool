#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "capacity can be obtained", "[object_pool]" )
{
    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        THEN ("capacity >= 0")
        {
            REQUIRE( pool.capacity() >= 0 );
        }
    }

    GIVEN ("A pool of 100 objects")
    {
        carlosb::object_pool<string> pool(100);

        THEN ("capacity >= 100")
        {
            REQUIRE( pool.capacity() == 100 );
        }
    }
}