/*
	A file to implement a command "sb" (short for "squashbug") in the
	shell that will create a child which does the job of detecting malwares. This command
	starts from a given process id (that the user identifies from htop as a suspected process
	and supplies as argument) and displays theparent,grandparent, ...of the given process.
	Note that the actual malware can be any of the parent, grandparent, of the given process,
	or the given process itself.
	Keep a flag "-suggest" which will additionally, based on a
	heuristic, detect which process id is the root of all trouble (one way: check the time
	spent for each process and / or the number of child processes that a process has
	spawned, find anything suspicious).

	Heuristic used: Since the malware spawns multiple child processes over time,
	the squashbug analyses the growth in children of the parents of the process with given pid.
	If a suitable process with pid sufficiently high (so it isn't a system process) and greater-than-normal number 
	of child processes is found, it is suggested to be a potential malware.
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <string.h>
#include <dirent.h>
#include <bits/stdc++.h>

#define int long long
using namespace std;
long long cnt = 1;
// wait time for the squashbug, before running another iteration
#define WAIT_TIME 20
// Number of iterations the squashbug would run
#define NUM_ITER 3

map <int, vector <int>> child;
map <int, int> visited;
float MAX_RATIO = 1.2;

// function to return the parent pid of a given process
int parent_pid(pid_t pid)
{
	// open the stat file of the given process
	string pid_str = to_string(pid);
	string stat_file = "/proc/" + pid_str + "/stat";
	ifstream stat(stat_file.c_str());
	if (!stat.is_open())
	{
		cerr << "Could not open " << stat_file << endl;
		return -1;
	}

	// read the stat file
	string line;
	getline(stat, line);
	stat.close();

	// get the 4th field of the stat file
	int i = 0, j = 0;
	while (i < 3)
	{
		if (line[j] == ' ')
			++i;
		++j;
	}

	// get the parent pid
	int parent_pid = 0;
	while (line[j] != ' ')
	{
		parent_pid = parent_pid * 10 + (line[j] - '0');
		++j;
	}
	// return parent's pid
	return parent_pid;
}

// update the child map
void update_child()
{
	// open /proc
	DIR * dir = opendir("/proc");
	if (dir == NULL)
	{
		perror("Error in opening dir.");
		return ;
	}

	// for all processes in /proc, 
	// go to /proc/pid/status and get the parent pid
	// then append the new children to the corresponding parent's vector
	struct dirent * e;
	while ((e = readdir(dir)) != NULL)
	{
		if (e->d_type != DT_DIR) continue;
		char * end;
		pid_t pid = strtol(e->d_name, &end, 10);
		if (*end != '\0') continue;

		char str[64];
		sprintf(str, "/proc/%d/status", pid);

		FILE * stat = fopen(str, "r");
		if (stat == NULL) continue;

		char line[256];
		while (fgets(line, sizeof(line), stat) != NULL)
		if (!strncmp(line, "PPid:", 5))
		{
			pid_t parent_pid;
			sscanf(line + 5, "%d", &parent_pid);
			if (! visited[pid])
			{
				child[parent_pid].push_back(pid);
				visited[pid] = 1;
			}
			break;
		}

		fclose(stat);
	}

	closedir(dir);
}

// Sum up the number of children of a given process
int num_child(pid_t pid)
{
	int sum = 0;
	if (child[pid].size() == 0) return 1;
	for (auto v : child[pid]) sum += num_child(v);
	return sum + 1;
}

signed main(int32_t argc, char *argv[])
{
	// check if the number of arguments is correct
	if ((argc > 3 || argc < 2) || (argc == 3 && string(argv[2]) != "-suggest"))
	{
		cerr << "Usage: " << argv[0] << " <pid> -suggest[optional flag]" << endl;
		return 1;
	}

	// get the pid from the command line argument
	string pid = argv[1];

	// check if the given pid is valid
	if (access(("/proc/" + pid).c_str(), F_OK) == -1)
	{
		cerr << "Error: Invalid pid " << pid << endl;
		return 1;
	}

	vector<int> pids;
	pids.push_back(stoi(pid));

	// recursively get the parent pid of the given process
	// until you reach the root process (init)
	while (stoi(pid) > 1)
	{
		// get the parent pid of the given process
		int ppid = parent_pid(stoi(pid));

		// if -1 was returned, then the process does not exist
		// i.e. there was a problem in opening the stat file
		if (ppid == -1)
		{
			cout << "Error: Could not get parent pid of " << pid << endl;
			return 1;
		}

		pids.push_back(ppid);
		cout << "Parent pid of " << pid << " => " << ppid << endl;

		// update the pid
		pid = to_string(ppid);
	}

	// work on the heuristic to detect the root of all trouble
	// if the flag "-suggest" is given
	if (argc == 3 && !strcmp(argv[2], "-suggest"))
	{
		cout << endl << "Finding potential malwares..." << endl;
		int c = 0, ans = 0;
		float max_ratio = MAX_RATIO;
		map <int, float> ratio;
		map <int, int> temp_num_child;

		update_child();
		for (auto v : pids)
		{
			temp_num_child[v] = num_child(v);
			cout << "Initial PID : " << v << " | Number of child processes : " << temp_num_child[v] << endl;
		}
		cout << endl;

		while(1)
		{
			update_child();
			for (auto v : pids)
			{
				int tmp = num_child(v);
				cout << "PID : " << v << " | Number of child processes : " << tmp << endl;
				if (ratio.find(v) == ratio.end() || tmp == 0) ratio[v] = 0;
				ratio[v] = (float)tmp / (float)temp_num_child[v];
				temp_num_child[v] = tmp;
			}

			c++;
			cout << endl;
			if (c >= NUM_ITER) break;
			sleep(WAIT_TIME);
		}

		// find the process with the highest ratio (> MAX_RATIO)
		for (auto v : ratio)
		if (v.second > max_ratio && v.first > 6000)
		{
			max_ratio = v.second;
			ans = v.first;
		}
		
		// if there's a process with a suspicious ratio of growth in number of children, print it
		if (ans) 
		{
			cout << "Potential malware detected! PID : ";
			printf("\033[1;31m");
			cout << ans ;
			printf("\033[0m");
			cout << " | child ratio produced: " << max_ratio << endl;
			cout << endl;
		}
		else cout << "No potential malwares detected!" << endl;
	}

	return 0;
}