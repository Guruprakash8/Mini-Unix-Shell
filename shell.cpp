/*
----------------------------------
Operating Systems Laboratory (CS39002)
Assignment : 2

Submitted by:-
    Group No. : 12 {20CS10041, 20CS30002, 20CS30018, 20CS30058}
----------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <glob.h>
#include <ncurses.h>
#include <termios.h>
#include <fstream>
#include <deque>
#include <list>
#include <libgen.h>
#include <limits.h>


using namespace std;

#define MAXLEN 1000
#define INP_LEN 1000
signed RUN_BGM = 0;

struct termios newtio, oldtio;

// History Maintaining
class history
{
private:
    history()
    {
        ifstream readstream(history_file, ios::in);
        if (!readstream.fail())
        {
            string s;
            while (getline(readstream, s))
            {
                add_history(s);
            }
        }
        readstream.close();
    }
    static const int max_history_cnt;
    static const string history_file;
    deque<string> history_list;

public:
    static history &get_instance()
    {
        static history instance;
        return instance;
    }
    void add_history(const string &s)
    {
        history_list.push_front(s);
        if (history_list.size() > max_history_cnt)
            history_list.pop_back();
    }
    void save_history()
    {
        ofstream writestream(history_file, ios::out | ios::trunc);
        for (int i = get_history_list_size() - 1; i >= 0; i--)
            writestream << history_list[i] << endl;
        writestream.close();
    }
    const deque<string> &get_history_list() const
    {
        return history_list;
    }
    int get_history_list_size()
    {
        return static_cast<int>(history_list.size());
    }
};

const int history::max_history_cnt = 1000;
const string history::history_file = ".history";

void print_bash()
{
    // print in light yellow color
    printf("\033[1;35m");
    char * username = getenv("USER");
    char logname[HOST_NAME_MAX];
    gethostname(logname, HOST_NAME_MAX);
    cout << username << "@" << logname << ":~";

    // get the current working directory
    char work_dir[1024];
    getcwd(work_dir, sizeof(work_dir));
    printf("\033[1;33m");
    printf("%s$ ", work_dir);
    printf("\033[0m");
    return;
}

// ctrl +C
void handle_SIGINT(int sig)
{
    signal(SIGINT, handle_SIGINT);
}

//ctrl+Z
void handle_SIGTSTP(int sig)
{
    signal(SIGTSTP, handle_SIGTSTP);
}

void handle_SIGINT_prompt(int sig)
{
    cout << endl;
    print_bash();
    fflush(stdout); // flushing
    signal(SIGINT, handle_SIGINT_prompt);
}

// tokenize

int tokenize(char *input, char **args, const char *c)
{

    char *token = strtok(input, c);
    int i = 0;

    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, c);
    }

    args[i] = NULL;
    return i;
}

// expands the wildcards

vector<string> expand_wildcards(vector<string> &args)
{
    std::vector<std::string> expanded_args;
    for (const auto &arg : args)
    {
        glob_t results;
        int ret = glob(arg.c_str(), 0, NULL, &results);
        if (ret == 0)
        {
            for (int i = 0; i < results.gl_pathc; i++)
                expanded_args.push_back(results.gl_pathv[i]);
            globfree(&results);
        }
        else
            expanded_args.push_back(arg);
    }
    return expanded_args;
}

// main

signed main(int argc, char *argv[])
{
    char path[PATH_MAX];
    realpath(argv[0], path);
    char *DIR_NAME = dirname(path);

    // start shell
    char *commands[100];
    char *args[MAXLEN];

    signal(SIGINT, handle_SIGINT);
    signal(SIGTSTP, handle_SIGTSTP);

    tcgetattr(STDIN_FILENO, &newtio);
    oldtio = newtio;
    newtio.c_lflag &= (~ICANON & ~ECHO);
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newtio);

    char input[INP_LEN];
    list<char> intake;
    list<char> entered_comm;

    // keep on taking input until exit is typed
    while (true)
    {
        print_bash();
        int background = 0;
        int pipe_fd[2];     //pipe 

        // signal
        signal(SIGINT, handle_SIGINT_prompt);

        // reading input line

        int count = 0;
        int current = 0;
        char ch = 0, prev_char = '\0';
        auto pos = intake.begin();
        do
        {
            ch = getchar();
            if (ch == '\n')
            {
                printf("\n");
                break;
            }
            else if (ch == 127) // backspace
            {
                if (intake.size() > 0)
                {
                    printf("\b \b");
                    pos--;
                    pos = intake.erase(pos);
                }
                if (intake.size() == 0)
                    current = 0;
                entered_comm = intake;
            }
            else if (ch == 27)
            {
                char ch1 = getchar();
                char ch2 = getchar();
                if (ch1 == 91)
                {
                    if (ch2 == 65) // up arrow
                    {
                        if (current != history::get_instance().get_history_list_size())
                            current++;
                        for (auto it = pos; it != intake.end(); it++)
                            printf("%c", *it);
                        for (int i = 0; i < intake.size(); i++)
                            printf("\b \b");
                        intake.clear();
                        pos = intake.begin();
                        if (current)
                        {
                            for (auto c : history::get_instance().get_history_list()[current - 1])
                                intake.insert(pos, c);
                            for (auto c : intake)
                                printf("%c", c);
                        }
                    }
                    else if (ch2 == 66) // down arrow
                    {
                        if (current != 0)
                            current--;
                        for (auto it = pos; it != intake.end(); it++)
                            printf("%c", *it);
                        for (int i = 0; i < intake.size(); i++)
                            printf("\b \b");
                        intake.clear();
                        pos = intake.begin();
                        if (current)
                        {
                            for (auto c : history::get_instance().get_history_list()[current - 1])
                                intake.insert(pos, c);
                            for (auto c : intake)
                                printf("%c", c);
                        }
                        else
                        {
                            for (auto c : entered_comm)
                            {
                                intake.insert(pos, c);
                                printf("%c", c);
                            }
                        }
                    }
                    else if (ch2 == 67) // right arrow
                    {
                        if (pos != intake.end())
                        {
                            printf("%c", *pos);
                            pos++;
                        }
                    }
                    else if (ch2 == 68) // left arrow
                    {
                        if (pos != intake.begin())
                        {
                            printf("\b");
                            pos--;
                        }
                    }
                }
            }
            else if (ch == 1)
            {
                for (auto it = intake.begin(); it != pos; it++)
                    printf("\b");
                pos = intake.begin();
            }
            else if (ch == 5)
            {
                for (auto it = pos; it != intake.end(); it++)
                    printf("%c", *it);
                pos = intake.end();
            }
            
            else
            {   
                //< or > space 
                int symb=0;
                if(ch=='<' || ch=='>') symb=1;
                if(symb) intake.insert(pos, ' ');
                intake.insert(pos, ch);
                if(symb) intake.insert(pos, ' ');

                printf("%c", ch);
                for (auto it = pos; it != intake.end(); it++)
                    printf("%c", *it);
                for (auto it = pos; it != intake.end(); it++)
                    printf("\b");
                entered_comm = intake;
            }

        } while (intake.size() < INP_LEN - 1);

        for (int i = 0; i < INP_LEN; i++)
            input[i] = 0;

        count = 0;
        for (auto it = intake.begin(); it != intake.end(); it++)
            input[count++] = *it;
        input[count] = 0;
        intake.clear();

        // if command is entered 
        if (ch == '\n')
        {
            while (count && input[count - 1] == ' ')
            {
                input[count - 1] = '\0';
                count--;
            }

            if (count == 0)
                continue;

            if (count)
            {
                history::get_instance().add_history(input);
                history::get_instance().save_history();
            }

            // to break the loop
            if (strcmp(input, "exit") == 0)
            {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
                printf("Exiting shell\n");
                break;
            }

            // replacing new line with \0
            input[strcspn(input, "\n")] = '\0';

            
            // background checking
            int len = strlen(input);
            if (len >= 2 && input[len - 1] == '&')
            {
                background = 1;
                input[len - 1] = '\0';
            }

            /** tokenize base on '|' **/
            int num_commands = tokenize(input, commands, "|");

            // for each command in piped commands 
            for (int j = 0; j < num_commands; j++)
            {

                // cout << commands[j] << endl;
                char *args[MAXLEN];
                char *inputfile = NULL, *outputfile = NULL;
                int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
                int infile_idx = -1, outfileidx = -1;

                // tokenize based on space and return it into args

                int numargs = tokenize(commands[j], args, " ");

                // Extract the '<' and '>' symbols
                // Extract the filenames :: (outputfile after '>' or inputfile after '<')

                int i = 0;
                while (i < numargs - 1 && args[i + 1] != NULL)
                {
                    if (!strcmp(args[i], "<"))
                    {
                        inputfile = (char *)malloc((strlen(args[i + 1]) + 1) * sizeof(char));
                        strcpy(inputfile, args[i + 1]);
                        infile_idx = i + 1;
                        args[i] = NULL;
                    }

                    else if (!strcmp(args[i], ">"))
                    {
                        outputfile = (char *)malloc((strlen(args[i + 1]) + 1) * sizeof(char));
                        strcpy(outputfile, args[i + 1]);
                        outfileidx = i + 1;
                        args[i] = NULL;
                    }
                    i++;
                }

                // remove null and other from the args if there exist (may be inserted due to < or >) 
                // and reconstruct args as args2 

                i = 0;
                char **args2;
                args2 = (char **)malloc((numargs + 1) * sizeof(char *));
                for (int k = 0; k < numargs; k++)
                {
                    args2[k] = (char *)malloc(100 * sizeof(char));
                }
                args2[numargs] = NULL;

                int k = 0;
                while (i < numargs)
                {
                    if (args[i] != NULL && i != infile_idx && i != outfileidx)
                    {
                        // cout << "args-" << args[i] << "\n";
                        strcpy(args2[k++], args[i]);
                    }
                    i++;
                }


                // checking to replcae * and ? symbols if present (wildcard entries) by calling function expand_wildcards

                vector<string> args3(args2, args2 + k);
                vector<string> expanded_args = expand_wildcards(args3);
                vector<char *> exec_args = vector<char *>();
                for (const auto &arg : expanded_args)
                {
                    exec_args.push_back(const_cast<char *>(arg.c_str()));
                }
                exec_args.push_back(nullptr);


                // if input file is  mentioned, get its file descriptor fd
                if (inputfile != NULL)
                {
                    input_fd = open(inputfile, O_RDONLY, 0777);
                    if (input_fd == -1)
                    {
                        perror("File can't be opened!\n");
                        exit(EXIT_FAILURE);
                    }
                }

                // if  output file is mentioned get its ,file descriptor fd
                if (outputfile != NULL)
                {
                    output_fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (output_fd == -1)
                    {
                        perror("File can't be opened!\n");
                        exit(EXIT_FAILURE);
                    }
                }

                //<<<<<<<<< pipe >>>>>>>>>>>>>
                // if not the first command i.e for all other commands other than first

                if (j > 0)
                    input_fd = pipe_fd[0];

                // except last command construct pipe for all commands

                if (j < num_commands - 1)
                {
                    int perr = pipe(pipe_fd);
                    if (perr == -1)
                        perror("Can't create pipe");
                    output_fd = pipe_fd[1];
                }

                // if command is cd
                if (strcmp(exec_args[0], "cd") == 0)
                {
                    //HOME 
                    if(exec_args.size()==2 && exec_args[1]==NULL) chdir(getenv("HOME"));

                    else if (chdir(exec_args[1]) != 0)
                    {
                        perror("Invalid directory");
                    }
                    
                }
                // if command is pwd
                else if(strcmp(exec_args[0], "pwd") == 0){
                    char work_dir_[1024];
                    getcwd(work_dir_, sizeof(work_dir_));
                    printf("%s\n",work_dir_);
                }
                else
                {
                    //<<<<<<<<<<<<<<<<<<< fork >>>>>>>>>>>>>>>>>>>>>>
                    // forking a child process
                    int pid = fork();
                    if (pid == 0)
                    {
                        // child process
                        // if input file mentioned replace stdin with file descriptor
                        fflush(stdout);

                        if (input_fd != STDIN_FILENO)
                        {
                            int er1 = dup2(input_fd, STDIN_FILENO);
                            if (er1 == -1)
                            {
                                perror("dup2 can't be done");
                                exit(EXIT_FAILURE);
                            }
                            close(input_fd);
                        }

                        // output file
                        if (output_fd != STDOUT_FILENO)
                        {
                            int er2 = dup2(output_fd, STDOUT_FILENO);
                            if (er2 == -1)
                            {
                                perror("dup2 can't be done");
                                exit(EXIT_FAILURE);
                            }
                            close(output_fd);
                        }

                        // calling execvp function
                        execvp(exec_args[0], exec_args.data());
                        perror("Execvp error");
                        exit(EXIT_FAILURE);
                    }
                    else if (pid == -1)
                    {

                        perror("error in forking!\n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        // // parent process
                        
                        if (!background)
                        {
                            int status;
                            waitpid(pid, &status, WUNTRACED | WCONTINUED);
                            if (WIFSTOPPED(status))
                                kill(pid, SIGCONT);
                            fflush(stdout);
                        }
                        else
                        {
                            cout << "Running in background...\n";
                        }
                    }
                }

                // freeing memory allocated
                for (int k = 0; k < numargs; k++)
                {
                    free(args2[k]);
                }
                free(args2);


                // close outfd if not last command
                if (j < num_commands - 1)
                    close(output_fd);
            }
        }
    }

    return 0;
}