// Preprocessor definitions of functions
void extract_ids_cmdline(int processID, char *cmdline, size_t cmdline_length, char *ids, size_t ids_length);

//this golbal variable stores all the processes that will be spawned when running LIBDFT and INTEL PIN (we need to skip these since they are trusted)
char *PIN_LIBDFT_PROC_NAMES[] = {"/home/kalana/Desktop/libdft/tools/",
"/home/kalana/Desktop/dTracker/dTracker-NSL/support/",
"sh -c /usr/bin/gcc --version | egrep -o \"[1-9]{1}\\.[1-9]{1}(\\.[0-9]*)?\" | head -1",
"/usr/bin/gcc --version",
"egrep -o [1-9]{1}\\.[1-9]{1}(\\.[0-9]*)?",
"head -1",
"dbus-launch --autolaunch=75e52a6646b73cde3573c9b400000005 --binary-syntax --close-stderr"};

//this global variable will keep track of the name of the executable. S.t it wont be killed when the LIBDFT+PIN execute the program
char current_analyzing_binary[2048];

/*
* This function helps to get process name and command line arguments passed into the process
* by examining /Proc/<PID> sudo process file system mounted to Linux root
* */
void extract_ids_cmdline(int processID, char *cmdline, size_t cmdline_length, char *ids, size_t ids_length)
{

char fname1[1024], fname2[1024], buf_[1024];
int r = 0, fd, i;
FILE *f = NULL;

snprintf(fname1, sizeof(fname1), "/proc/%d/status", processID);
snprintf(fname2, sizeof(fname2), "/proc/%d/cmdline", processID);

f = fopen(fname1, "r");
fd = open(fname2, O_RDONLY);

memset(cmdline, 0, cmdline_length);
memset(ids, 0, ids_length);

while (f && fgets(buf_, sizeof(buf_), f) != NULL)
{
if (strstr(buf_, "Uid"))
{
strtok(buf_, "\n");
snprintf(ids, ids_length, "%s", buf_);
}
}

if (f)
fclose(f);

if (fd > 0)
{
r = read(fd, cmdline, cmdline_length);
close(fd);

for (i = 0; r > 0 && i < r; ++i)
{
if (cmdline[i] == 0)
cmdline[i] = ' ';
}
}
}

bool check_if_process_is_PIN_LIBDFT_related(char *executable_name)
{
int len = sizeof(PIN_LIBDFT_PROC_NAMES) / sizeof(PIN_LIBDFT_PROC_NAMES[0]);
int x = 0;

for (x = 0; x < len; x++)
{
//printf("%s -- %s [%s]) \n\n", PIN_LIBDFT_PROC_NAMES[x], executable_name, strstr(executable_name, PIN_LIBDFT_PROC_NAMES[x]));
if (strstr(executable_name, PIN_LIBDFT_PROC_NAMES[x]) != NULL) //if found
{
return true;
}
}

return false; //not found
}

void cmdLine_to_arguments(char *cmdLine, char *argv[], size_t args_length)
{
//will keep track of current location of the argv array
int pos = 0;
char *token;
//first clear the argv char* array
memset(argv, 0, args_length);

//then lets setup the argv array with sudo and libdft+pin commands
argv[0] = "/usr/bin/sudo";
argv[1] = "/home/kalana/Desktop/DIFT/DIFT-Implementation/pin";
argv[2] = "-follow_execv";
argv[3] = "-t";
argv[4] = "/home/kalana/Desktop/DIFT/DIFT-Implementation/libdft_64/tools/obj-intel64/track.so";
argv[5] = "--";

pos += 6; //so we can start filling this array with right place

/* get the first token */
token = strtok(cmdLine, " ");

/* walk through other tokens */
while (token != NULL)
{
argv[pos] = token;
token = strtok(NULL, " ");
pos++;
}
}

void spawn_analysis_processes(char *executable_name, int PID_to_kill)
{

if (executable_name[0] != 0) //why run analysis on processes without executable ?
{
printf("Spawning \n\n");
if (!check_if_process_is_PIN_LIBDFT_related(executable_name) && strcmp(executable_name, current_analyzing_binary) != 0) //if the process is not a part of LIBDFT or PIN then we gotta run analysis
{
strncpy(current_analyzing_binary, executable_name, sizeof(current_analyzing_binary));
// Make sure we are not killing the analysis program
kill(PID_to_kill, SIGKILL);
printf("Killed the process.\n");

pid_t pid = fork();
printf("---- PID: %d - analysis process\n\n", pid);

if (pid == -1)
{
perror("fork failed");
exit(EXIT_FAILURE);
}
else if (pid == 0)
{
char *argv[2048];
cmdLine_to_arguments(executable_name, argv, sizeof(argv));
printf("Running new process\n");
execv("/usr/bin/sudo", argv);
_exit(EXIT_SUCCESS);
}
/*else {

int status;
printf("Waiting\n");
(void)waitpid(pid, &status, WNOHANG);
printf("End wait \n");

}
*/
}
}
}

