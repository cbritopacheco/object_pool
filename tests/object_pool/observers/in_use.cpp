#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "check if a pool is in use", "[object_pool]" )
{
    GIVEN( "An empty pool" )
    {
        carlosb::object_pool<string> pool;
        
        THEN ("in_use == false")
        {
            REQUIRE( pool.in_use() == false );
        }
    }

    GIVEN ("A pool of 100 objects and no acquisitions")
    {
        carlosb::object_pool<string> pool(100);

        THEN ("in_use == false")
        {
            REQUIRE( pool.in_use() == false );
        }
    }

    GIVEN ("A pool of 100 objects and 10 acquisitions")
    {
        using acquired_object = carlosb::object_pool<string>::acquired_object;

        carlosb::object_pool<string> pool(100);
        
        acquired_object arr[10];
        for (int i = 0; i < 10; ++i)
            arr[i] = pool.acquire();

        THEN ("in_use == true")
        {
            REQUIRE( pool.in_use() == true );
        }
    }

    GIVEN ("A pool of 100 objects and 10 acquisitions that have went out of scope")
    {
        using acquired_object = carlosb::object_pool<string>::acquired_object;

        carlosb::object_pool<string> pool(100);
        
        {
            acquired_object arr[10];
            for (int i = 0; i < 10; ++i)
                arr[i] = pool.acquire();
        }

        THEN ("in_use == false")
        {
            REQUIRE( pool.in_use() == false );
        }
    }
}