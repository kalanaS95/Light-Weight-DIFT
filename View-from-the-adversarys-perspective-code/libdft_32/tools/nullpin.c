/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

/*! @file
 * A test for callbacks around fork in jit mode.
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "pin.H"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <list>
#include <iterator>

using namespace std;


INT32 Usage()
{
    cerr <<
        "This pin tool registers callbacks around fork().\n"
        "\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


pid_t parent_pid;
PIN_LOCK lock;

// list to store all the outputs we need to print at the end
list<string> output_list;

static KNOB<string> logpath(KNOB_MODE_WRITEONCE, "pintool", "l", "processtrace.txt", "");

VOID get_command_line_args(int PID) {
	char cmd[1024];
	char buffer[128];
	std::string result = "";
	
	//cat /proc/%d/cmdline
	snprintf(cmd,1024, "ps -p %d -o ppid, -o pid, -o user, -o group, -o args", PID);
	FILE* pipe = popen(cmd, "r");
	if(!pipe) {
		throw std::runtime_error("popen failed\n");
		exit(-1);
	}
	
	try {
		while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
			result += buffer;
		}
	} catch(...) {
		pclose(pipe);
		throw;
	}
	
	pclose(pipe);
	
	
	//cout <<result.erase(0, result.find("\n")+1) <<endl << endl;
	result.erase(0, result.find("\n")+1);
	//cout << "*" << result << endl  << endl;
	//output_list.push_back(result);
	
	FILE *logfile;
	
	
	if ((logfile = fopen(logpath.Value().c_str(), "a")) != NULL) {
		(void)fprintf(logfile, result.c_str());
		//(void)fprintf(logfile, "\n");
		(void)fclose(logfile);
	}
	
}

VOID BeforeFork(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    PIN_GetLock(&lock, threadid+1);
    //cerr << "TOOL: Before fork." << endl;
    PIN_ReleaseLock(&lock);
    parent_pid = PIN_GetPid();
    //cout << "Parent PID: " << parent_pid << endl;
    get_command_line_args(parent_pid);
    
}

VOID AfterForkInParent(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    PIN_GetLock(&lock, threadid+1);
    //cerr << "TOOL: After fork in parent." << endl;
    PIN_ReleaseLock(&lock);
    
    //cout << "After Fork in parent: " << PIN_GetPid() << endl;
    if (PIN_GetPid() != parent_pid)
    {
    	cerr << "PIN_GetPid() fails in parent process" << endl;
        exit(-1);
    }
}

VOID AfterForkInChild(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    PIN_GetLock(&lock, threadid+1);
    //cerr << "TOOL: After fork in child." << endl;
    PIN_ReleaseLock(&lock);
    
    int child_pid = PIN_GetPid();
    
    //cout << "Child PID: " << PIN_GetPid() << endl;
    get_command_line_args(child_pid);
    
    if ((PIN_GetPid() == parent_pid) || (getppid() != parent_pid))
    {
        cerr << "PIN_GetPid() fails in child process" << endl;
        exit(-1);
    }
}

int main(INT32 argc, CHAR **argv)
{
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Initialize the pin lock
    PIN_InitLock(&lock);
    
    // Register a notification handler that is called when the application
    // forks a new process.
    PIN_AddForkFunction(FPOINT_BEFORE, BeforeFork, 0);	
    //PIN_AddForkFunction(FPOINT_AFTER_IN_PARENT, AfterForkInParent, 0);
    PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, AfterForkInChild, 0);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
