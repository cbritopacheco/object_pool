#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "size can be obtained", "[object_pool]" )
{
    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        THEN ("size == 0")
        {
            REQUIRE( pool.size() == 0 );
        }
    }

    GIVEN ("A pool of 100 objects")
    {
        carlosb::object_pool<string> pool(100);

        THEN ("size == 100")
        {
            REQUIRE( pool.size() == 100 );
        }
    }
}