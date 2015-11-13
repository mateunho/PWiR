#include <iostream>
#include <cstdio>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

#include <sys/wait.h>

#include <vector>

using namespace std;

/**
 * @brief process_control
 * 1/2 of main features
 * Enables spawning new processes of GStreamer test window
 * and closing them all at once
 */
void process_control();

/**
 * @brief piping
 * 2/2 of main features
 * Shows an example communication between parent and child process
 *
 */
void piping();

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
int new_process();

/**
 * @brief close_all
 * Closes all processes described by param `vec`
 * @param vec Vector of child processes PIDs
 */
void close_all(vector<int>& vec);



int main()
{
    bool part = 0;

    part
        ? process_control()
        : piping();

    return 0;
}


void sigchld_handler(int)
{
    int status;
    cout << "SIGCHLD: child " << wait(&status);
    cout << " terminated with status: " << status << endl;
}


int new_process()
{
    int pid = fork();
    if (0 == pid) {
        cout << "child: " << getpid() << endl;

        int dev_null = open("/dev/null", O_RDWR);
        if (0 > dev_null) {
            return 1;
        }

        // redirect process IO from parent context
        dup2(dev_null, 0);
        dup2(dev_null, 1);
        dup2(dev_null, 2);

        execl("/usr/bin/gst-launch-0.10", "gst-launch",
              "videotestsrc", "!", "ximagesink", (char*)NULL);
        return 0;
    } else if (0 < pid) {
        // PID to be remembered by parent
        return pid;
    } else {
        return -1;
    }
}


void close_all(vector<int>& vec)
{
    for (int i = 0; i < vec.size(); ++i) {
        kill(vec[i], SIGTERM);
    }
    vec.clear();
}


void process_control()
{
    vector<int> children; // contains children PIDs
    bool isLooping = true;
    int opt;

    // register signal handler
    signal(SIGCHLD, sigchld_handler);

    while (isLooping) {
        cout << "opts:" << endl;
        cout << "1: nowy proces" << endl;
        cout << "2: zamknij wszystkie okna" << endl;
        cout << "3: koniec" << endl;
        scanf("%d", &opt);
        switch (opt) {
        case 1: {
            // spawn new child process and remember its PID
            children.push_back( new_process() );
            break;
        }
        case 2:
            // close all remembered child processes
            close_all(children);
            break;
        default:
            isLooping = false;
            break;
        }
    }
}


void piping()
{
    // helper struct
    struct bufXY {
        double x, y;
        bufXY(double _x, double _y) : x(_x), y(_y) {}
    };

    // parent_to_child and child_to_parent pipes
    int PtoC[2], CtoP[2];

    // prepare transport pipes
    if (0 != pipe(PtoC)
        || 0 != pipe(CtoP)) {
        cerr << "Piping error\n";
        return 1;
    }

    int chldpid = fork(); // create child process

    if (0 < chldpid) /* parent part */ {
        // close unused pipes drains
        close(PtoC[0]);
        close(CtoP[1]);

        bufXY sendBuf(3.6, 4.5); // prepare package

        write(PtoC[1], sendBuf, sizeof(bufXY)); // send package
        cout << "sent: " << sendBuf.x << ", " << sendBuf.y << endl;

        double recBuf;
        read(CtoP[0], &recBuf, sizeof(double)); // receive response package
        cout << "received: " << recBuf << endl;

        // close used pipes drains
        close(PtoC[1]);
        close(CtoP[0]);
    } else if (0 == chldpid) /* child part */ {
        // close unused pipes drains
        close(PtoC[1]);
        close(CtoP[0]);

        bufXY recBuf;
        read(PtoC[0], recBuf, sizeof(bufXY)); // receive request package

        double sendBuf = recBuf.x * recBuf.y; // prepare response
        write(CtoP[1], &sendBuf, sizeof(double)); // send response package

        // close used pipes drains
        close(PtoC[0]);
        close(CtoP[1]);
    } else {
        cerr << "Forking error\n";
    }
}
