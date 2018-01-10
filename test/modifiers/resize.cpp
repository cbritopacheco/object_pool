#include "catch.hpp"
#include "object_pool.hpp"

SCENARIO( "object_pool can be resized incrementally", "[object_pool]" )
{

    GIVEN( "An empty pool of ints" )
    {
        carlosb::object_pool<int> pool;
        
        REQUIRE( pool.size() == 0 );
        REQUIRE( pool.capacity() >= 0 );
        
        WHEN ( "the pool is resized with count > size()" )
        {
            pool.resize(100);
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 100 );
                REQUIRE( pool.capacity() >= 100 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }

        WHEN ( "the pool is resized with count > size() and a default value" )
        {
            pool.resize(100, 42);
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 100 );
                REQUIRE( pool.capacity() >= 100 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }

    GIVEN( "A pool of 100 ints" )
    {
        carlosb::object_pool<int> pool(100);
        
        REQUIRE( pool.size() == 100 );
        REQUIRE( pool.capacity() >= 100 );
        
        WHEN ( "the pool is resized with count = 100" )
        {
            pool.resize(100);
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 100 );
                REQUIRE( pool.capacity() >= 100 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }

        WHEN ( "the pool is resized with count < size() nothing happens" )
        {
            pool.resize(1);

            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 100 );
                REQUIRE( pool.capacity() >= 100 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }

        WHEN ( "the pool is resized with count < size() and a default value nothing happens" )
        {
            pool.resize(100, 42);
            
            THEN ( "the size changes" )
            {
                REQUIRE( pool.size() == 100 );
                REQUIRE( pool.capacity() >= 100 );
            }

            THEN ( "the pool is not empty" )
            {
                REQUIRE ( pool.empty() == false );
                REQUIRE ( static_cast<bool>(pool) );
            }
        }
    }
}
