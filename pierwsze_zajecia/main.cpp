#include <iostream>
#include <unistd.h>


int main()
{
    std::cout << "Hello parent world!" << std::endl;

    // try creating child process
    if (0 == fork()) {
        std::cout << "Hello child world!" << std::endl;
    }

    sleep(1);

    return 0;
}
