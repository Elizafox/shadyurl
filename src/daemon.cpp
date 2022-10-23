// Taken from https://lloydrochester.com/post/c/unix-daemon-example/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fstream>
#include <string_view>

#include "daemon.hpp"

namespace daemonise
{

std::string_view pid_file_path{"/var/run/shadyurl.pid"};

// returns true on success, false on error
bool
daemonise(int flags)
{
	int maxfd, fd;

	/* The first fork will change our pid
	 * but the sid and pgid will be the
	 * calling process.
	 */
	switch(fork())				// become background process
	{
		case -1: return false;
		case 0: break;			// child falls through
		default: _exit(EXIT_SUCCESS);	// parent terminates
	}

	/*
	 * Run the process in a new session without a controlling
	 * terminal. The process group ID will be the process ID
	 * and thus, the process will be the process group leader.
	 * After this call the process will be in a new session,
	 * and it will be the progress group leader in a new
	 * process group.
	 */
	if(setsid() == -1)			// become leader of new session
		return false;

	/*
	 * We will fork again, also known as a
	 * double fork. This second fork will orphan
	 * our process because the parent will exit.
	 * When the parent process exits the child
	 * process will be adopted by the init process
	 * with process ID 1.
	 * The result of this second fork is a process
	 * with the parent as the init process with an ID
	 * of 1. The process will be in it's own session
	 * and process group and will have no controlling
	 * terminal. Furthermore, the process will not
	 * be the process group leader and thus, cannot
	 * have the controlling terminal if there was one.
	 */
	switch(fork())
	{
		case -1: return false;
		case 0: break;			// child breaks out of case
		default: _exit(EXIT_SUCCESS);	// parent process will exit
	}

	if(!(flags & D_NO_UMASK0))
		umask(0);			// clear file creation mode mask

	if(!(flags & D_NO_CHDIR))
		chdir("/");			// change to root directory

	if(!(flags & D_NO_CLOSE_FILES))		// close all open files
	{
		maxfd = sysconf(_SC_OPEN_MAX);
		if(maxfd == -1)
			maxfd = D_MAX_CLOSE;	// if we don't know then guess
		for(fd = 0; fd < maxfd; fd++)
			close(fd);
	}

	if(!(flags & D_NO_REOPEN_STD_FDS))
	{
		/* now time to go "dark"!
		 * we'll close stdin
		 * then we'll point stdout and stderr
		 * to /dev/null
		 */
		close(STDIN_FILENO);

		fd = open("/dev/null", O_RDWR);
		if(fd != STDIN_FILENO)
			return false;
		if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
			return false;
		if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
			return false;
	}

	return true;
}

// Returns true if process doesn't exist, false otherwise
bool
check_pid()
{
	std::ifstream pid_file{pid_file_path};
	if(!pid_file)
		return true;

	int pid;
	pid_file >> pid;
	pid_file.close();

	if(kill(static_cast<pid_t>(pid), 0))
		// Error, we assume all good.
		return true;
	else
		return false;
}

// Returns true if PID written, false otherwise
bool
write_pid()
{
	pid_t pid = getpid();

	std::ofstream pid_file{pid_file_path, std::ofstream::out | std::ofstream::trunc};
	if(!pid_file)
		return false;

	pid_file << static_cast<int>(pid);
	pid_file.close();

	return true;
}

void
remove_pid()
{
	(void)unlink(pid_file_path.data());
}

} // namespace daemon
