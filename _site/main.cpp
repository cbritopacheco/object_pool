#include <iostream>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

int main()
{   
    object_pool<int> pool(10, 15);

    if (auto obj = pool.acquire())
        cout << "We acquired: " << *obj << "\n";
    else
        cout << "We didn't acquire an object" << "\n";

    return 0;
}