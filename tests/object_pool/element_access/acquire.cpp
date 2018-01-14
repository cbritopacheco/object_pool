#include "catch.hpp"
#include "object_pool.hpp"

#include <string>

using namespace std;

SCENARIO( "objects can be acquired instanteanously", "[object_pool]" )
{
    GIVEN( "A non empty pool of strings" )
    {
        carlosb::object_pool<string> pool(10, "Hello World!");
        REQUIRE(pool.size() == 10);

        THEN ("We can acquire an object.")
        {
            auto obj = pool.acquire();
            REQUIRE(static_cast<bool>(obj));
            REQUIRE(*obj == "Hello World!");
            REQUIRE(pool.size() == 9);
            REQUIRE(pool.in_use());
            REQUIRE(static_cast<bool>(pool));
        }

        THEN ("We can acquire an object, which returns to the pool automatically.")
        {
            {
                auto obj = pool.acquire();
                REQUIRE(static_cast<bool>(obj));
                REQUIRE(*obj == "Hello World!");
                REQUIRE(pool.size() == 9);
                REQUIRE(pool.in_use());
                REQUIRE(static_cast<bool>(pool));
                // object will go out of scope here
            }

            REQUIRE(pool.size() == 10);
            REQUIRE(! pool.in_use());
            REQUIRE(static_cast<bool>(pool));
        }
    }

    GIVEN( "An empty pool of strings" )
    {
        carlosb::object_pool<string> pool;
        REQUIRE(pool.size() == 0);

        THEN ("We will acquire an empty object.")
        {
            auto obj = pool.acquire();
            REQUIRE(! static_cast<bool>(obj));
            REQUIRE(pool.size() == 0);
            REQUIRE(! pool.in_use());
            REQUIRE(! static_cast<bool>(pool));
        }
    }

    GIVEN( "An pool of 1 string" )
    {
        carlosb::object_pool<string> pool(1, "Hello World");
        REQUIRE(pool.size() == 1);

        THEN ("We can only acquire the number of elements that are in the pool. Subsequent elements are just emtpy.")
        {
            {
                auto obj1 = pool.acquire(); // hello world
                auto obj2 = pool.acquire(); // empty object

                *obj1 = "Modified";
                REQUIRE(pool.size() == 0);
                REQUIRE(pool.in_use());
                REQUIRE(! static_cast<bool>(pool)); // no more free elements
                
                REQUIRE(static_cast<bool>(obj1)); // obj1 is initialized
                REQUIRE(*obj1 == "Modified");

                REQUIRE(! static_cast<bool>(obj2)); // obj1 is initialized
            }

            {
                auto obj3 = pool.acquire();

                REQUIRE(pool.size() == 0);
                REQUIRE(pool.in_use());
                REQUIRE(! static_cast<bool>(pool)); // no more free elements
                REQUIRE(*obj3 == "Modified");
            }

            REQUIRE(pool.size() == 1);
            REQUIRE(! pool.in_use());
            REQUIRE(static_cast<bool>(pool));
        }
    }
}