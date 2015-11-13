#include <iostream>
#include <cstdio>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

#include <sys/wait.h>

using namespace std;

/**
 * @brief process_tree
 * 1/3 of main features
 * Creates a simple tree of processes
 */
void process_tree();

/**
 * @brief process_chain
 * 2/3 of main features
 * Creates a small chain of processes
 */
void process_chain();

/**
 * @brief process_control
 * 3/3 of main features
 * Enables spawning new processes of GStreamer test window
 * and closing them all at once
 */
void process_control();

/**
 * @brief sigchld_handler
 * Handles system signal `SIGCHLD` sent by killed child process
 */
void sigchld_handler(int);

/**
 * @brief new_process
 * Spawns a new child process of GStreamer
 * @return PID of the newly created process
 */
void new_process();


int main()
{
    unsigned part = 0;

    switch (part) {
    case 0:
        process_tree();
        break;
    case 1:
        process_chain();
        break;
    case 2:
        process_control();
        break;
    }

    return 0;
}


void sigchld_handler(int)
{
    int status;
    cout << "SIGCHLD: child " << wait(&status);
    cout << " terminated with status: " << status << endl;
}


void new_process()
{
    if (0 == fork()) {
        cout << "child: " << getpid() << endl;
        execl("/usr/bin/gst-launch-0.10", "gst-launch",
              "videotestsrc", "!", "ximagesink", (char*)NULL);
    }
}


void process_tree()
{
    if (0 == fork()) {
        cout << "child 1: " << getpid() << endl;
    } else if (0 == fork()) {
        cout << "child 2: " << getpid() << endl;
    } else {
        cout << "parent: " << getpid() << endl;
    }

    sleep(60);
}


void process_chain()
{
    if (0 == fork()) {
        cout << "child 1: " << getpid() << endl;
        if (0 == fork()) {
            cout << "child 1.1: " << getpid() << endl;
        }
    } else {
        cout << "parent: " << getpid() << endl;
    }

    sleep(60);
}


void process_control()
{
    bool isLooping = true;
    int opt;

    // register signal handler
    signal(SIGCHLD, sigchld_handler);

    while (isLooping) {
        cout << "opts:" << endl;
        cout << "1: nowy proces" << endl;
        cout << "2: koniec" << endl;
        scanf("%d", &opt);
        switch (opt) {
        case 1:
            // spawn new child process
            new_process();
            break;
        default:
            isLooping = false;
            break;
        }
    }
}
