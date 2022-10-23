#ifndef DAEMON_H
#define DAEMON_H

#include <string_view>

namespace daemonise
{

enum
{
	D_NO_CHDIR = 0x1,		// Don't chdir("/")
	D_NO_CLOSE_FILES = 0x2,	// Don't close all open files
	D_NO_REOPEN_STD_FDS = 0x4,	// Don't reopen stdin, stdout, and stderr to /dev/null
	D_NO_UMASK0 = 0x8,		// Don't do a umask(0)
	D_MAX_CLOSE = 8192,		// Max file descriptors to close if sysconf(_SC_OPEN_MAX) is indeterminate
};

// returns 0 on success -1 on error
bool daemonise(int flags);

bool check_pid();
bool write_pid();
void remove_pid();

bool drop_privs(std::string_view, std::string_view);
bool set_rlimit();

} // namespace daemon

#endif // DAEMON_H
