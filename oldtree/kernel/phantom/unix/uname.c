#include <sys/utsname.h>
#include <kernel/boot.h>
#include "svn_version.h"


struct utsname phantom_uname = 
{
	"Phantom OS",
	"?",
	"?",
	PHANTOM_VERSION_STR,
	"x86",
	"--"
};
