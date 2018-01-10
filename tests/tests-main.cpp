#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "object_pool.hpp"

struct AggregateObject
{};

template class carlosb::object_pool<AggregateObject>;