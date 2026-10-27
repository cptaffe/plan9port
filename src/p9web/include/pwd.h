#ifndef _P9WEB_PWD_H
#define _P9WEB_PWD_H
#include <sys/types.h>
struct passwd { char *pw_name; char *pw_dir; uid_t pw_uid; gid_t pw_gid; char *pw_shell; };
struct passwd *getpwuid(uid_t);
struct passwd *getpwnam(const char*);
#endif
