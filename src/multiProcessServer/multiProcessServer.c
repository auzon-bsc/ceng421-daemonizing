/* 
non_preforking multi process server

gcc multiProcessServer.c -o MultiProcessServer
./MultiProcessServer
*/
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
// additional includes
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>

void sigchld_handler(int signo)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int daemonize()
{
	pid_t pid;
	long n_desc;
	int i;
	
	if ((pid = fork()) != 0) 
	{
		exit(0);
	}
	
	setsid();
	
	if ((pid = fork()) != 0) 
	{
		exit(0);
	}
	
	chdir("/");
	umask(0);
	
	n_desc = sysconf(_SC_OPEN_MAX);
	
	for (i = 0; i < n_desc; i++) 
	{
		close(i);
	}
		return 1;
}

int main(int argc, char *argv[])
{
	struct passwd *pws;
	/*create a nonprivileged user for this
	sudo useradd nopriv 
	*/
	const char *user = "nopriv";
	
	pws = getpwnam(user);
	
	if (pws == NULL) 
	{
		printf ("Unknown user: %s\n", user);
		return 0;
	}

	daemonize();

	chroot("/home/multiProcessServer");	// change root to home dir
	chdir("/");

	setuid(pws->pw_uid);
	
	struct sockaddr_in sAddr;
	int listensock;
	int newsock;
	char buffer[25];
	int result;
	int nread;
	int pid;
	int val;

	listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	val = 1;
	result = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (result < 0) 
	{
		perror("server2");
		return 0;
	}

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(1972);
	sAddr.sin_addr.s_addr = INADDR_ANY;
	result = bind(listensock, (struct sockaddr *) &sAddr, sizeof(sAddr));
	
	if (result < 0) 
	{
		perror("server2");
		return 0;
	}

	result = listen(listensock, 5);
	
	if (result < 0) 
	{
		perror("server2");
		return 0;
	}

	signal(SIGCHLD, sigchld_handler);

	while (1) 
	{
		newsock = accept(listensock, NULL, NULL);
		if ((pid = fork()) == 0) 
		{
			openlog("multiProcessServer", LOG_PID, LOG_USER);
			
			/*instead of printing directly, send to syslog
			printf("child process %i created.\n", getpid());*/
			syslog(LOG_INFO, "child process %i created.\n", getpid());

			close(listensock);

			nread = recv(newsock, buffer, 25, 0);
			buffer[nread] = '\0';
			printf("%s\n", buffer);
			send(newsock, buffer, nread, 0);
			close(newsock);
			
			/*instead of printing directly, send to syslog
			printf("child process %i finished.\n", getpid()); */
			syslog(LOG_INFO, "child process %i finished.\n", getpid());

			closelog();

			exit(0);
		}

		close(newsock);
	}
}
