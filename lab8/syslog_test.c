#include <syslog.h>
#include <unistd.h>
 

int main(int argc, char **argv){
	setlogmask (LOG_UPTO (LOG_NOTICE));
	
	openlog (argv[0], LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);
	
	syslog (LOG_NOTICE, "Program started by User %d", getuid ());
	syslog (LOG_NOTICE, "ERRNO = %m");
	syslog (LOG_INFO, "A tree falls in a forest");
	syslog (LOG_ERR, "A big tree falls in a forest");
	
	closelog ();
}
