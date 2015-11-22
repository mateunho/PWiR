#include <atomic>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>


using std::async;
using std::atomic_uint;
using std::cout;
using std::endl;
using std::future;
using std::lock_guard;
using std::mutex;
using std::thread;
using std::vector;

typedef unsigned uint;


mutex mtx;
uint pcounter_wild;
uint pcounter_mutexed;
uint pcounter_guarded;
atomic_uint pcounter_atomic(0); // same as `atomic<uint>`

const unsigned method = 5;

uint count_primes_serial(uint p_min, uint p_max);
void count_primes_threaded_wild(uint p_min, uint p_max);
void count_primes_threaded_mutexed(uint p_min, uint p_max);
void count_primes_threaded_guarded_balanced(uint p_min, uint p_max, uint thId, uint thNum);
void count_primes_threaded_atomic_balanced(uint p_min, uint p_max, uint thId, uint thNum);
uint count_primes_async_balanced(uint p_min, uint p_max, uint thId, uint thNum);


int main(void)
{
    uint p_min = 0, p_max = 1000000;
    int start = time(NULL);

    switch (method) {
    case 1:
        cout << "serial algorithm\n";
        cout << "primes: " << count_primes_serial(p_min, p_max) << endl;
        break;
    case 2: {
        cout << "parallel with no data race protection\n";
        uint s = (p_max - p_min) / 4;
        vector<thread> vec;
        vec.emplace_back(thread(count_primes_threaded_wild, p_min, 1*s));
        vec.emplace_back(thread(count_primes_threaded_wild, 1*s+1, 2*s));
        vec.emplace_back(thread(count_primes_threaded_wild, 2*s+1, 3*s));
        vec.emplace_back(thread(count_primes_threaded_wild, 3*s+1, p_max));
        for (auto& th : vec) { th.join(); }
        cout << "primes: " << pcounter_wild << endl;
        break;
    }
    case 3: {
        cout << "parallel with mutex protection\n";
        int s = (p_max - p_min) / 4;
        vector<thread> vec;
        vec.emplace_back(thread(count_primes_threaded_mutexed, p_min, 1*s));
        vec.emplace_back(thread(count_primes_threaded_mutexed, 1*s+1, 2*s));
        vec.emplace_back(thread(count_primes_threaded_mutexed, 2*s+1, 3*s));
        vec.emplace_back(thread(count_primes_threaded_mutexed, 3*s+1, p_max));
        for (auto& th : vec) { th.join(); }
        cout << "primes: " << pcounter_mutexed << endl;
        break;
    }
    case 4: {
        cout << "parallel with guard lock protection (load balanced)\n";
        vector<thread> vec;
        for (auto i : {0, 1, 2, 3}) {
            vec.emplace_back(thread(count_primes_threaded_guarded_balanced,p_min, p_max, i, 4));
        }
        for (auto& th : vec) { th.join(); }
        cout << "primes: " << pcounter_guarded << endl;
        break;
    }
    case 5: {
        cout << "parallel with atomicity protection (load balanced)\n";
        vector<thread> vec;
        for (auto i : {0, 1, 2, 3}) {
            vec.emplace_back(thread(count_primes_threaded_atomic_balanced, p_min, p_max, i, 4));
        }
        for (auto& th : vec) { th.join(); }
        cout << "primes: " << pcounter_atomic << endl;
        break;
    }
    case 6: {
        cout << "parallel with thread-local variable to global sum (load balanced)\n";
        uint pcounter_async = 0;
        vector< future<uint> > vec;
        for (auto i : {0, 1, 2, 3}) {
            vec.emplace_back(async(std::launch::async, count_primes_async_balanced, p_min, p_max, i, 4));
        }
        for (auto& th : vec) { pcounter_async += th.get(); }
        cout << "primes: " << pcounter_async << endl;
        break;
    }
    default:
        cout << "method out of range\n";
        return 0;
    }

    cout << "time spent: " << time(NULL) - start << "sec\n";
    return 0;
}


uint count_primes_serial(uint p_min, uint p_max)
{
    uint pcounter = 0;
    if (p_min < 3) {
        pcounter++;
        p_min = 3;
    } else if (0 == p_min % 2) {
        p_min++;
    }

    for (; p_min <= p_max; p_min += 2) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        pcounter++;
not_prime:;
    }
    return pcounter;
}


void count_primes_threaded_wild(uint p_min, uint p_max)
{
    if (p_min < 3) {
        pcounter_wild++;
        p_min = 3;
    } else if (0 == p_min % 2) {
        p_min++;
    }

    for (; p_min <= p_max; p_min += 2) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        pcounter_wild++;
not_prime:;
    }
}


void count_primes_threaded_mutexed(uint p_min, uint p_max)
{
    if (p_min < 3) {
        mtx.lock();
        pcounter_mutexed++;
        mtx.unlock();
        p_min = 3;
    } else if (p_min % 2 == 0) {
        p_min++;
    }

    for (; p_min <= p_max; p_min += 2) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        mtx.lock();
        pcounter_mutexed++;
        mtx.unlock();
not_prime:;
    }
}


void count_primes_threaded_guarded_balanced(uint p_min, uint p_max, uint thId, uint thNum)
{
    p_min += thId * 2;

    if (2 == p_min) {
        lock_guard<mutex> lg(mtx);
        pcounter_guarded++;
    }
    if (p_min % 2 == 0) {
        p_min++;
    }
    if (1 == p_min) {
        p_min += 2 * thNum;
    }

    for (; p_min <= p_max; p_min += 2 * thNum) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        {
        lock_guard<mutex> lg(mtx);
        pcounter_guarded++;
        }
not_prime:;
    }
}


void count_primes_threaded_atomic_balanced(uint p_min, uint p_max, uint thId, uint thNum)
{
    p_min += thId * 2;

    if (2 == p_min) {
        pcounter_atomic++;
    }
    if (p_min % 2 == 0) {
        p_min++;
    }
    if (1 == p_min) {
        p_min += 2 * thNum;
    }

    for (; p_min <= p_max; p_min += 2 * thNum) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        pcounter_atomic++;
not_prime:;
    }
}


uint count_primes_async_balanced(uint p_min, uint p_max, uint thId, uint thNum)
{
    uint pcounter = 0;
    p_min += thId * 2;

    if (2 == p_min) {
        pcounter++;
    }
    if (p_min % 2 == 0) {
        p_min++;
    }
    if (1 == p_min) {
        p_min += 2 * thNum;
    }

    for (; p_min <= p_max; p_min += 2 * thNum) {
        for (uint i = 3; i <= p_min / 2; i += 2) {
            if (p_min % i == 0)
                goto not_prime;
        }
        pcounter++;
not_prime:;
    }
    return pcounter;
}
