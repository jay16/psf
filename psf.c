/*

Tested on: FreeBSD 4.3, Linux 2.4.x, NetBSD 1.5, Solaris 2.7

*/

#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define VERSION		"0.01a"
#define MAXPATH		256

char *fullpath(char *cmd)
{
	char *p, *q, *filename;
	struct stat st;

	if (*cmd == '/')
		return cmd;

	filename = (char *) malloc(MAXPATH);
	if  (*cmd == '.')
		if (getcwd(filename, MAXPATH - 1) != NULL)
		{
			strncat(filename, "/");
			strncat(filename, cmd);
			return filename;
		}
		else
			return NULL;

	for (p = q = (char *) getenv("PATH"); q != NULL; p = ++q)
	{
		if (q = (char *) strchr(q, ':'))
			*q = (char) '\0';

		snprintf(filename, MAXPATH, "%s/%s", p, cmd);

		if (stat(filename, &st) != -1
		    && S_ISREG(st.st_mode)
		    && (st.st_mode&S_IXUSR || st.st_mode&S_IXGRP || st.st_mode&S_IXOTH))
			return filename;

		if (q == NULL)
			break;
	}

	free(filename);
	return NULL;
}

char *mytmp(void)
{
	char *tmp, *me = "/.psf-", *ret;
	struct passwd *p;
	
	if ((tmp = (char *) getenv("TMPDIR")) == NULL)
		tmp = P_tmpdir;

	if ((p = getpwuid(getuid())) == NULL)
	{
		perror("[err] username reteival");
		exit(-1);
	}
	
	ret = (char *) malloc (strlen(tmp) + sizeof(me) + strlen(p->pw_name) + 1);
	*ret = '\0';

	strcat(ret, tmp);
	strcat(ret, me);
	strcat(ret, p->pw_name);
	
	return ret;
}

void usage(char *prog)
{
	printf("Process Stack Faker (a.k.a. Fucker) v" VERSION "\n");
	printf("Coded by Stas; (C)opyLeft by SysD Destructive Labs, 1997-2001\n\n");

	printf("Usage: %s [options] command arg1 arg2 ...\n", prog);
	printf("Where options can be:\n");
	printf("-s string\t"	"fake process name\n");
	printf("-p filename\t"	"file to write PID of spawned process - optional\n");
	printf("-d\t\t"		"try to start as daemon (in background, no tty) - optional\n");
	printf("-l\t\t"		"DO NOT exec through link (detectable by 'top'!!!) - optional\n");
}

int main(int argc, char *argv[])
{
	char buf[MAXPATH], fake[MAXPATH], fakecomm[MAXPATH];
	char *tmp, *fp, *p, *myexec, o;
	char *spoof = NULL, *pidfile = NULL;
	char **newargv;
	int i, j, n, daemon = 0, link = 1;
	FILE *f;
	int null;

	// Check for our parameters
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			switch (o = argv[i][1])
			{
				case 's': 
					spoof = argv[++i]; break;
				case 'p':
					pidfile = argv[++i]; break;
				case 'd':
					daemon = 1; break;
				case 'l':
					link = 0; break;

				default:
					usage(argv[0]);
					fprintf(stderr, "\n * Don't understood option -%c!\n", o);
					return -1;
			}
		else
			break;

	}

	// Is all OK?
	if (!(n = argc - i) || spoof == NULL)
	{
		usage(argv[0]);
		fprintf(stderr, "\n * Wrong usage!\n");
		return -1;
	}

	// Check for other's parameters
	newargv = (char **) malloc(n * sizeof(char **) + 1);
	for (j = 0; j < n; i++,j++)
		newargv[j] = argv[i];
	newargv[j] = NULL;

	if ((fp = fullpath(newargv[0])) == NULL)
	{
		perror("[err] full path seek");
		return -1;
	}
	
	// Now we'll make top happy linking wierd things...
	if (link)
	{
		tmp = mytmp();

		strncpy(buf, basename(spoof), strlen(spoof) + 1);
		for (p = buf; *p != '\0' && !isspace(*p); p++);
		*p = '\0';
		snprintf(fake, sizeof(fake), "%s/%s", tmp, buf);

		mkdir(tmp, 0700);	// try to create (see later if really created)
		remove(fake);		// reset any existing link
		if (symlink(fp, fake) == -1)
		{
			perror("[err] link creation");
			return -1;
		}
		
		myexec = fake;
	}
	else
		myexec = fp;

	if (n > 1)
	{
		// Build space-padded fake command
		memset(fakecomm, ' ', sizeof(fakecomm) - 1);
		fakecomm[sizeof(fakecomm) - 1] = '\0';
		strncpy(fakecomm, spoof, strlen(spoof));
		newargv[0] = fakecomm;
	}
	else
		newargv[0] = spoof;

	if (daemon)
	{
		if ((null = open("/dev/null", O_RDWR)) == -1)
		{
			perror("[err] /dev/null");
			return -1;
		}

		switch (fork())
		{
			case -1:
				perror("[err] fork1");
				return -1;
			case  0:
				setsid();
				switch (fork())
				{
					case -1:
						perror("[err] fork2");
						return -1;
					case  0:
						// chdir("/");
						umask(0);

						// close standart IO
						close(0);
						close(1);
						close(2);

						// open'em as /dev/null
						dup2(null, 0);
						dup2(null, 1);
						dup2(null, 2);

						break;
					default:
						return 0;
				}
				break;
			default:
				return 0;
		}
	}

	// Save PID if user asked...
	if (pidfile != NULL && (f = fopen(pidfile, "wt")) != NULL)
	{
		fprintf(f, "%d\n", getpid());
		fclose(f);
	}

/****** Some code from UPX 1.20 ;) ******/
	// Fork off a subprocess to clean up.
	// We have to do this double-fork trick to keep a zombie from
	// hanging around if the spawned original program doesn't check for
	// subprocesses (as well as to prevent the real program from getting
	// confused about this subprocess it shouldn't have).
	// Thanks to Adam Ierymenko <api@one.net> for this solution.

	if (link && !fork())
	{
		if (fork() == 0)
		{
			// Sleep 3 seconds, then remove the temp file.
			static const struct timespec ts = { 3, 0 };
			nanosleep(&ts, 0);

			if (unlink(fake) == -1)
				perror("[warn] can't erase symlink");
			if (rmdir(tmp) == -1)
				perror("[warn] can't remove tmpdir");
		}
		exit(0);
	}

	// Wait for the first fork()'d process to die.
	waitpid(-1, (int *)0, 0);
/****** Some code from UPX 1.20 ;) ******/
	
	// And now, execute it!
	execv(myexec, newargv);

	perror("[err] couldn't execute");
	return -1;
}
