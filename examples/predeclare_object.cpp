#include <iostream>
#include <string>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

int main()
{   
    // Predeclare an acquired_object
    acquired_object<string> obj;

    {
        // Construct object_pool with 10 strings that say 'Hello World!'
        object_pool<string> pool(10, "Hello World!");

        // Acquire and print the object
        obj = pool.acquire();
        cout << *obj << "\n";

        // Modify object
        *obj =  "The object will still be alive!";
    }

    // Access object after pool is out of scope
    cout << *obj << "\n";

    return 0;
}