#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "objects can be allocated", "[object_pool]" )
{
    GIVEN( "A non empty pool of strings" )
    {
        carlosb::object_pool<string> pool(10, "Hello World!");
        REQUIRE(pool.size() == 10);

        THEN ("We can allocate an object.")
        {
            {
                auto obj = pool.allocate("New object");
                REQUIRE(static_cast<bool>(obj));
                REQUIRE(*obj == "Hello World!");
                REQUIRE(pool.size() == 9);
                REQUIRE(pool.in_use());
                REQUIRE(static_cast<bool>(pool));
            }
            REQUIRE(pool.size() == 10);

            
        }
    }

    GIVEN( "An empty pool of strings" )
    {
        carlosb::object_pool<string> pool;
        REQUIRE(pool.size() == 0);

        THEN ("We can allocate an object.")
        {
            {
                auto obj = pool.allocate("New object");
                REQUIRE(static_cast<bool>(obj));
                REQUIRE(pool.size() == 0);
                REQUIRE(pool.in_use());
                REQUIRE(! static_cast<bool>(pool));
            }

            REQUIRE(pool.size() == 1);
            REQUIRE(! pool.in_use());
            REQUIRE(static_cast<bool>(pool));
        }
    }

    GIVEN( "An pool of 1 string" )
    {
        carlosb::object_pool<string> pool(1, "Hello World");
        REQUIRE(pool.size() == 1);

        THEN ("Two consecutive allocations will result in a pool of 2 strings")
        {
            {
                auto obj1 = pool.allocate("Allocated from object1"); // hello world
                auto obj2 = pool.allocate("Allocated from object2"); // empty object

                REQUIRE(pool.size() == 0);
                REQUIRE(pool.in_use());
                REQUIRE(! static_cast<bool>(pool)); // no more free elements
                REQUIRE(static_cast<bool>(obj1)); // obj1 is initialized
                REQUIRE(static_cast<bool>(obj2)); // obj2 is initialized
                REQUIRE(*obj1 == "Hello World"); // no more free elements
                REQUIRE(*obj2 == "Allocated from object2"); // no more free elements
            }

            REQUIRE(pool.size() == 2);
            REQUIRE(! pool.in_use());
            REQUIRE(static_cast<bool>(pool));
        }
    }
}