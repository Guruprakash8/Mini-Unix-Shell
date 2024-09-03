// a simple file which works like filelocker
// but does not lock the file
#include <iostream>
#include <sys/file.h>
#include <unistd.h>

using namespace std;
#define FILE_NAME "test.txt"

signed main()
{
    cout << "Non locker up and running at pid = "<<  getpid() << endl;
    fflush(stdout);

    // Open the file in append mode
    int fd = open(FILE_NAME, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd == -1)
    {
        cout << "Error opening file!" << endl;
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