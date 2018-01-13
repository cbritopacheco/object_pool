#include <iostream>
#include <string>
#include <thread>
#include "src/object_pool.hpp"

using namespace std;
using namespace carlosb;

int main(int argc, char const *argv[])
{
    object_pool<string> pool(1, "hello");

    for (int i = 0; i < 10000; ++i)
    {
        cout << '\n' << i << '\n';

        thread t1([&pool]() {
            *(pool.acquire_wait()) = string("thread1");
        });

        thread t2([&pool]() {
            *(pool.acquire_wait()) = string("thread2");
        });

        // cout << *pool.acquire_wait() << endl;

        t1.join();
        t2.join();
    }
    return 0;
}