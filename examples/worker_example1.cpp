#include <thread>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

object_pool<string> pool;   // Shared pool of strings
mutex io_mutex;             // Used to control access to std::cout


void worker1()
{
    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Acquiring objects...\n";
    }

    // acquire object
    auto obj = pool.lock_acquire();

    {
        lock_guard<mutex> lock_cout(io_mutex);
        
        cout
            << "[Worker 1]: I have acquired this from the pool: \'" 
            << *obj
            << "\'\n";
    }

    // modify object
    *obj = "Modified from Worker 1";

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Sleeping for 5 seconds..." << "\n";
    }

    // sleep for 5 seconds
    this_thread::sleep_for(chrono::seconds(5));

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Waking up!" << "\n";
    }
}

void worker2()
{
    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 2]: Sleeping for 1 second..." << "\n";
    }

    // sleep for 1 seconds so worker1 gets a hold of the object first
    this_thread::sleep_for(chrono::seconds(1));

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 2]: Waking up!" << "\n";
        cout << "[Worker 2]: Acquiring objects...\n";
    }

    auto obj = pool.lock_acquire();

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout
            << "[Worker 2]: I have acquired this from the pool: \'" 
            << *obj
            << "\'\n";
    }
}

int main(int argc, char const *argv[])
{   
    // declare two threads
    std::thread t1(worker1), t2(worker2);

    // push the string into the pool
    pool.push("Hello World!");

    // join the threads
    t1.join();
    t2.join();

    return 0;
}