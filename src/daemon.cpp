#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <fstream>
#include <string_view>

#include "daemon.hpp"

namespace daemonise
{

static std::string_view pid_file_path{"/var/run/shadyurl.pid"};

// Taken from https://lloydrochester.com/post/c/unix-daemon-example/
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

bool
drop_privs(std::string_view user, std::string_view group)
{
	gid_t gid = static_cast<gid_t>(-1);
	uid_t uid = static_cast<uid_t>(-1);

	if(!user.empty())
	{
		struct passwd* pwd;
		if((pwd = getpwnam(user.data())) == nullptr)
		{
			syslog(LOG_ALERT, "Could not find user: %s", user.data());
			return false;
		}

		uid = pwd->pw_uid;

		if(group.empty())
		{
			gid = pwd->pw_gid;
		}
	}

	if(!group.empty())
	{
		struct group* grp;

		if((grp = getgrnam(group.data())) == nullptr)
		{
			syslog(LOG_ALERT, "Could not find group %s: %s", group.data(), strerror(errno));
			return false;
		}

		gid = grp->gr_gid;
	}

	if(gid != static_cast<gid_t>(-1))
	{
		if(setgid(gid) == -1)
		{
			syslog(LOG_ALERT, "Could not setgid(%d): %s", gid, strerror(errno));
			return false;
		}

		// Pare down ancillary groups
		if(setgroups(0, NULL) == -1)
		{
			syslog(LOG_ALERT, "Could not setgroups(): %s", strerror(errno));
			return false;
		}

		if(!user.empty() && initgroups(user.data(), gid) == -1)
		{
			syslog(LOG_ALERT, "Could not initgroups(%s, %d): %s", user.data(), gid, strerror(errno));
			return false;
		}
	}

	if(uid != static_cast<uid_t>(-1))
	{
		if(setuid(uid) == -1)
		{
			syslog(LOG_ALERT, "Could not setuid(%d): %s", uid, strerror(errno));
			return false;
		}
	}

	return true;
}

} // namespace daemon
