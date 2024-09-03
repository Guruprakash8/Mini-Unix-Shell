/*
    A file to implement a command "delep" (short for delete
    with extreme prejudice) which takes a filepath as argument and spawns a child process.
    The child process will help to list all the process pids which have this file open as well as
    all the process pids which are holding a lock over the file. Then the parent process will
    show it to the user and ask permission to kill each of those processes using a yes/no
    prompt. On putting yes, all of those processes will be killed using signal and then the file
    will be deleted. 
*/

#include <iostream>
#include <vector>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>
#include <signal.h>

using namespace std;

vector<int> get_pids(const string& filepath) {
    // iterate through all running processes :: 
    // go to /proc/ folder and return names of all numbered files in a vector

    vector <int> pids;
    vector <int> out;

    // open /proc/
    DIR * dir;
    struct dirent * ent;

    if ((dir = opendir("/proc/")) != NULL)
    {
        while ((ent = readdir(dir)) != NULL) {
            // std::cout << ent->d_name << std::endl;
            if (((string)ent->d_name).find_first_not_of("0123456789") == string::npos) pids.push_back(atoi(ent->d_name));
        }
        closedir(dir);
    }  

    // go to each /proc/pid/fd/ and check if the file is open
    for (auto pid: pids)
    {
        // go to /proc/pid/fd
        string path = "/proc/" + to_string(pid) + "/fd/";
        DIR * dir;
        struct dirent * ent;

        // now go to each file in /proc/pid/fd and check if it is a symlink to given filepath
        if ((dir = opendir(path.c_str())) != NULL)
        {
            while ((ent = readdir(dir)) != NULL) {
                // std::cout << ent->d_name << std::endl;
                if (((string)ent->d_name).find_first_not_of("0123456789") == string::npos) {
                    // now check if this file is a symlink to given filepath
                    char linkname[PATH_MAX];
                    ssize_t r = readlink((path + ent->d_name).c_str(), linkname, PATH_MAX);
                    if (r != -1) 
                    {
                        linkname[r] = '\0';
                        
                        if (filepath == (string)linkname) {
                            // cout << "pid " << pid << " has file " << filepath << " open" << endl;
                            out.push_back(pid);

                            // now check if this file is locked by this process
                            // go to /proc/pid/fdinfo
                            string path = "/proc/" + to_string(pid) + "/fdinfo/";
                            DIR * dir;
                            struct dirent * ent;

                            // now go to each file in /proc/pid/fdinfo and check if it is a symlink to given filepath and see if there
                            // is a lock value set to 1
                            for (int i = 0; i <= 3; ++i)
                            {
                                string path = "/proc/" + to_string(pid) + "/fdinfo/" + to_string(i);
                                ifstream file(path);
                                string line;
                                while (getline(file, line))
                                if (line.find("lock:") != string::npos && line.find("1") != string::npos)
                                {
                                    out[out.size() - 1] = -out[out.size() - 1];
                                    break;
                                }
                            }
                            continue;
                        }
                    }
                }
            }
            closedir(dir);
        }
    }

    return out;
}

signed main(int argc, char *argv[])
{
    if (argc != 2) {
        cout << "Usage: delep <filepath>" << endl;
        return 0;
    }

    vector <int> pids = get_pids((string)argv[1]);
    // cout << pids.size() << endl;
    if (pids.size() > 0)
    {
        printf("The following processes have the file %s open:-\n(process ids having the file locked are highlighted in red)\n", argv[1]);
        for (int i = 0; i < pids.size(); ++i) 
        {
            // check if this file is locked by this process
            if(pids[i] < 0) 
            {
                pids[i] = -pids[i];
                printf("\033[1;31m");
            }
            else printf("\033[0m");
            cout << pids[i] << endl;
        }
        printf("\033[0m");
        // now ask for permission to kill each of these processes
        for (auto pid : pids)
        {
            cout << "Do you want to kill process " << pid << "? (y/n) ";
            char c;
            cin >> c;
            if (c == 'y') kill(pid, 9);
        }

        // delete the said file
        cout << endl << "Delete file " << argv[1] << "? (y/n) ";
        char c;
        cin >> c;
        if (c == 'y') 
        {
            if (remove(argv[1]) != 0) cout << "Error deleting file!" << endl;
            else cout << "File successfully deleted!" << endl;
        }
        else cout << "File not deleted." << endl;
    }
    else cout << "No processes have the file " << argv[1] << " open or the file does not exist!" << endl;
    return 0;
}