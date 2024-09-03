/*
 * A test file which spawns a process which locks a certain file  
 * and tries to write to it in a while loop
 */

#include <iostream>
#include <sys/file.h>
#include <unistd.h>

using namespace std;
#define FILE_NAME "out.txt"

signed main()
{
    cout << "File locker up and running at pid = "<<  getpid() << endl;
    fflush(stdout);

    // Open the file in append mode
    int fd = open(FILE_NAME, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd == -1)
    {
        cout << "Error opening file!" << endl;
        return 1;
    }

    // Lock the file
    if (flock(fd, LOCK_SH | LOCK_NB) != 0)
    {
        cout << "Error locking file!" << endl;
        return 1;
    }

    // Recursively write to the file
    while (1)
    {
        write(fd, NULL, 0);
        sleep(1);
    }

    return 0;
}