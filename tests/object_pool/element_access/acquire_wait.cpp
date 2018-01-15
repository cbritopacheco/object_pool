#include "catch.hpp"
#include "object_pool.hpp"

#include <string>
#include <thread>
#include <chrono>

using namespace std;

SCENARIO( "you can wait until there is an acquisition or a timeout", "[object_pool]" )
{
    GIVEN( "A pool of 1 string and 2 threads that attempt to acquire the object" )
    {
        carlosb::object_pool<string> pool(1, "Hello World!");
        REQUIRE(pool.size() == 1);

        THEN ("We can wait indefinitely until acquisition.")
        {
            // thread that acquires the object first
            thread t1([&pool]()
            {
                auto obj = pool.acquire_wait();
                REQUIRE(obj);
                REQUIRE(pool.size() == 0);
                REQUIRE(pool.empty());
                REQUIRE(*obj == "Hello World!");

                *obj = "Modified from t1";
                this_thread::sleep_for(chrono::milliseconds(3000));
            });

            // thread that acquires the object after t1 releases it
            thread t2([&pool]()
            {
                this_thread::sleep_for(chrono::milliseconds(500)); // wait so t1 gets the object first
                auto obj = pool.acquire_wait();
                REQUIRE(obj);
                REQUIRE(pool.size() == 0);
                REQUIRE(pool.empty());
                REQUIRE(*obj == "Modified from t1");
            });

            t1.join();
            t2.join();
        }

        THEN ("We can wait until timeout for acquisition.")
        {

            // thread that acquires the object first
            thread t1([&pool]()
            {
                auto obj = pool.acquire_wait();
                REQUIRE(obj);
                REQUIRE(pool.size() == 0);
                REQUIRE(pool.empty());
                REQUIRE(*obj == "Hello World!");

                *obj = "Modified from t1";
                this_thread::sleep_for(chrono::milliseconds(3000));
            });

            // thread that acquires the object after t1 releases it
            thread t2([&pool]()
            {
                this_thread::sleep_for(chrono::milliseconds(500)); // wait so t1 gets the object first
                
                // At this point t1 should have the object and will sleep for 3 secs
                // before returning the object. However, we set a timeout of 1 second
                // so this should trigger a timeout.
                auto obj = pool.acquire_wait(chrono::milliseconds(1000));
                REQUIRE(!obj);
            });

            t1.join();
            t2.join();
        }
    }
}