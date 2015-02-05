/// This file is a part of Capn.
///
/// Capn is a useful and multipurpose hooking library for the Windows platform.
///
/// Copyright 2015 Aaron Bolyard.
///
/// For licensing information, review the LICENSE file located at the root
/// directory of the source package.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>

// Command line argument type.
enum ARGUMENT_TYPE
{
	ARGUMENT_TYPE_HELP = 0,
	ARGUMENT_TYPE_PID,
	ARGUMENT_TYPE_EXECUTABLE,
	ARGUMENT_TYPE_WORKING_DIRECTORY,
	ARGUMENT_TYPE_COMMAND_LINE_ARGS,
	ARGUMENT_TYPE_HOOK,
	ARGUMENT_TYPE_INVALID
};

struct ArgumentInfo
{
	const char* argument;
	const char* help;
	ARGUMENT_TYPE type;
};

// Each option is prefixed by a forward slash.
// If an argument requires an option, the value will directly follow
// the argument, seperated by a colon.
const ArgumentInfo Arguments[] =
{
	{ "?", NULL, ARGUMENT_TYPE_HELP },
	{ "pid", "Injects a DLL into an already running process", ARGUMENT_TYPE_PID },
	{ "exe", "Runs and then injects a DLL into an executable", ARGUMENT_TYPE_EXECUTABLE },
	{ "cwd", "Requires /exe, changes the current working directory of the executable", ARGUMENT_TYPE_WORKING_DIRECTORY },
	{ "args", "Requires /exe, supplies a list of arguments to the executable", ARGUMENT_TYPE_COMMAND_LINE_ARGS },
	{ "hook", "Provides the hook DLL.", ARGUMENT_TYPE_HOOK },
	{ NULL, NULL, ARGUMENT_TYPE_INVALID } // End of list.
};

// Checks an argument and sets the option.
//
// The parameter, `option', will be set to NULL (or an empty string) if there is no option.
// Otherwise, it will contain the option to the argument.
//
// Returns: The argument type, or ARGUMENT_TYPE_INVALID in the case of malformed input.
ARGUMENT_TYPE CheckArgument(const char* a, const char*& option)
{
	// Set option to NULL to indicate that no option was provided (general case).
	option = NULL;

	// Sanity check, in case an argument provided is empty or immediately invalid.
	// Keep in mind, since an argument begins with the forward slash character,
	// there must be at least two characters in a valid argument.
	if (strlen(a) < 2 || a[0] != '/')
	{
		return ARGUMENT_TYPE_INVALID;
	}

	for (const ArgumentInfo* arg = Arguments; arg->argument != NULL; ++arg)
	{
		int argLength = strlen(arg->argument);

		// Check the option, only checking the minimum amount possible.
		// As well, skip the first character of the provided string, `a'.
		if (std::strncmp(arg->argument, a + 1, argLength) == 0)
		{
			int l = std::strlen(a + 1);

			// Check for an optional option (woo) to the argument.
			// This also serves as a means to check that the provided argument
			// is not actually equal for `argLength' bytes.
			// Otherwise, /A would be considered the same as /ABC.
			if (l >= argLength)
			{
				if (l > argLength && a[argLength + 1] == ':')
					option = &a[argLength + 2]; // Skip the option seperator.

				return arg->type;
			}
		}
	}

	return ARGUMENT_TYPE_INVALID;
}

// The type of process to hook.
enum APPLICATION_HOOK_TYPE
{
	APPLICATION_HOOK_TYPE_PID,
	APPLICATION_HOOK_TYPE_EXECUTABLE,
	APPLICATION_HOOK_TYPE_INVALID
};

int main(int argc, const char* argv[])
{
	// Start with a sane default value. If the value remains unchanged
	// after checking the command-line arguments, an application type
	// was never provided. In this case, bail out and inform the user.
	APPLICATION_HOOK_TYPE hookType = APPLICATION_HOOK_TYPE_INVALID;

	// Required. This is either a PID or an executable name, as determined by `hookType'.
	const char* application = NULL;

	// Required. The hook DLL.
	const char* hook = NULL;

	// Optional working directory and command line options.
	// These are only used, and silently ignored otherwise, if the application hook type
	// is an executable.
	const char* workingDirectory = NULL;
	const char* commandLineArguments = "";

	// Optional value indicating the program should display help.
	bool showHelp = false;

	// Early sanity check. Show a helpful line if no arguments are provided.
	if (argc == 1)
	{
		std::printf("No arguments provided.\n");
		std::printf("Run with /? for  help.");

		return 1;
	}

	// Process the arguments, one at a type. The last type of each argument
	// is the only that matters (to allow overriding previous options).
	// (Obviously skip the first one since it's the executable name.)
	// (Also, if showHelp is set, abort early.)
	for (int i = 1; i < argc && !showHelp; ++i)
	{
		const char* option = NULL;

		switch(CheckArgument(argv[i], option))
		{
			case ARGUMENT_TYPE_HELP:
				showHelp = true;
				break;

			case ARGUMENT_TYPE_PID:
				hookType = APPLICATION_HOOK_TYPE_PID;
				application = option;
				break;
			
			case ARGUMENT_TYPE_EXECUTABLE:
				hookType = APPLICATION_HOOK_TYPE_EXECUTABLE;
				application = option;
				break;

			case ARGUMENT_TYPE_WORKING_DIRECTORY:
				workingDirectory = option;
				break;

			case ARGUMENT_TYPE_COMMAND_LINE_ARGS:
				commandLineArguments = option;
				break;

			case ARGUMENT_TYPE_HOOK:
				hook = option;
				break;

			default:
				// Silently ignore invalid input.
				break;
		}
	}

	// Show help and exit.
	if (showHelp)
	{
		for (const ArgumentInfo* arg = Arguments; arg->argument != NULL; ++arg)
		{
			if (arg->help)
				std::printf("%4s: %s\n", arg->argument, arg->help);
		}

		return 0;
	}

	// If no hook type is specified, tell the user and exit.
	if (hookType == APPLICATION_HOOK_TYPE_INVALID)
	{
		std::printf("No hook type specified.\n");
		std::printf("Run with /? for  help.");

		return 1;
	}

	// Same for no hook specified.
	if (!hook)
	{
		std::printf("No hook DLL specified.\n");
		std::printf("Run with /? for  help.");

		return 1;
	}

	HANDLE process = INVALID_HANDLE_VALUE;
	HANDLE processThread = INVALID_HANDLE_VALUE;
	int pid = 0;

	// Try and execute the program and get the PID.
	if (hookType == APPLICATION_HOOK_TYPE_EXECUTABLE)
	{
		// Build command line arguments (simply '<application> <commandLineArguments>').
		// Keep in mind there is a space and the terminating NUL character.
		// This has to be done in a non-const buffer since CreateProcess can modify the buffer...
		int commandLineLength = std::strlen(application) + std::strlen(commandLineArguments) + 2;
		char* commandLine = (char*)std::malloc(commandLineLength);
		
		// Add the application name.
		std::memcpy(commandLine, application, strlen(application));

		// And the space.
		commandLine[strlen(application)] = ' ';

		// And the command line arguments.
		std::memcpy(commandLine + strlen(application) + 1, commandLineArguments, std::strlen(commandLineArguments));

		// Lastly, the NUL terminator.
		commandLine[commandLineLength - 1] = '\0';

		// Run the process.
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION processInformation;

		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		BOOL result = CreateProcess(NULL, commandLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, workingDirectory, &startupInfo, &processInformation);
		free(commandLine);

		if (result)
		{
			processThread = processInformation.hThread;
			CloseHandle(processInformation.hProcess);

			pid = processInformation.dwProcessId;
		}
		else
		{
			std::fprintf(stderr, "Could not open process %s (%lu)!\n", application, GetLastError());

			return 1;
		}
	}
	// Else, get the PID from the argument.
	else
	{
		pid = std::strtol(application, NULL, 0);
	}

	// Open the process.
	process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);

	if (process == INVALID_HANDLE_VALUE)
	{
		std::fprintf(stderr, "Could not open process from PID %d", pid);

		return 1;
	}

	// Allocate memory in the process.
	int memorySize = std::strlen(hook) + 1;
	void* memory = VirtualAllocEx(process, NULL, memorySize, MEM_COMMIT, PAGE_READWRITE);

	if (!memory)
	{
		std::fprintf(stderr, "Could not allocate memory in process!");

		CloseHandle(process);

		return 1;
	}

	// Write memory.
	if (WriteProcessMemory(process, memory, hook, memorySize, NULL) == 0)
	{
		std::fprintf(stderr, "Could not write to process memory!");

		// This shouldn't fail if the program has gotten this far.
		if (!VirtualFreeEx(process, memory, memorySize, MEM_RELEASE))
			std::fprintf(stderr, "Also failed to free memory in process...");

		CloseHandle(process);

		return 1;
	}

	// Create a thread, using the `LoadLibraryA' method as the thread function.
	// This is because the remote function would have to exist in the process,
	// and in doing so, would have to be copied over. This is the easier route,
	// since LoadLibrary is shared amongst all processes.
	LPTHREAD_START_ROUTINE routine = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	HANDLE thread = CreateRemoteThread(process, NULL, 0, routine, memory, 0, NULL);

	if (thread == INVALID_HANDLE_VALUE)
	{
		std::fprintf(stderr, "Could not create thread!");
		
		// Deja vu.
		if (!VirtualFreeEx(process, memory, memorySize, MEM_RELEASE))
			std::fprintf(stderr, "Also failed to free memory in process...");

		CloseHandle(process);

		return 1;
	}

	// Check the result of the method.
	WaitForSingleObject(thread, INFINITE);

	DWORD exitCode = 0;
	BOOL result = GetExitCodeThread(thread, &exitCode);

	// Free memory, etc.
	VirtualFreeEx(process, memory, memorySize, MEM_RELEASE);
	CloseHandle(thread);

	// Run process.
	if (hookType == APPLICATION_HOOK_TYPE_EXECUTABLE)
	{
		ResumeThread(processThread);
		CloseHandle(processThread);
	}

	CloseHandle(process);

	if (!result || !exitCode)
	{
		std::fprintf(stderr, "Could not inject DLL!");

		return 1;
	}

	// Success!
	std::printf("Injected DLL.");

	return 0;
}
