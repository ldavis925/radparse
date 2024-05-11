/*
** Radius log processor
** Lane Davis, FastQ Communications
** <ldavis@fastq.com>
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#include "stdc.h"
#include "radp.h"
#include "externs.h"

#if !defined(lint)
u_char *rcs_id = "$Id$";
#endif /*lint*/

extern	int		optind, opterr;
extern	char		*optarg;

static	char 		*basename = NULL;
static	dev_t		this_device;

static	void		usage 			__P((void));
static	int		process_detail_dirs 	__P((const char *));
static	char		*userfile 		__P((const char *));

int			importf = 0;
int			debug = 0;
int			progress = 0;
char			*detnam = "detail";
char			*userid = NULL;

u_short			date_list[D_LIMIT][2], date_index;

static			FILE	*repf;
static			char	*repnam = NULL;

int
main (ac, av)
	int 	ac;
	char 	**av;
{
	int	opt;
	char	cwd[PATH_MAX+1];

	date_index = 0;

	basename = strrchr (av[0], '/');
	if (!basename) basename = av[0];
	else ++basename;

	opterr = 1;
	while ((opt = getopt (ac, av, "ih?pn:d:u:")) != EOF) {
		switch (opt) {
		/* Progress and debug are exclusive */
		case 'p':
			progress++;
			/*debug = 0; */
			break;
		case 'u':
			userid = strdup (optarg);
			repnam = userfile (userid);
			if (!(repf = fopen (repnam, "wt"))) {
				perror (repnam);
				exit (-1);
			}
			break;
		case 'd':
			debug = atoi (optarg);
			/* if (debug) progress = 0; */
			break;
		case 'n':
			detnam = strdup (optarg);
			break;
		case 'i':
			importf++;
			break;
		case 'h':
		case '?':
		default	:
			usage ();
			break;
		}
	}

	ac -= optind;
	av += optind;

	if (!getwd (cwd)) {
		fprintf (stderr, "getwd: current working directory too long\n");
		exit (-1);
	}

	process_detail_dirs (ac > 0 ? av[0] : ".");

	(void) chdir (cwd);
	user_list ();

	exit (0);
}

static void
usage (void)
{
	(void) fprintf (stderr, "usage: %s [-hpi] [-u user] [-d level] [-n name] [radius_dir]\n",
		basename);
	exit (-1);
}

static int
process_detail_dirs (const char *dir)
{
	static	int	ft = 1;
	DIR		*Dp;
	struct dirent	*De;
	struct stat	sb;
	int		rv;
	int		nproc = 0;

	if (DOTDOT(dir))
		return -1;

	if (chdir (dir) < 0) {
		perror ("chdir");
		return -1;
	}

	rv = 0;

	if (!(Dp = opendir ("."))) {
		perror (basename);
		rv = -1;
		goto process_fini;
	}

	while ((De = readdir (Dp))) {
		if (DOTFILE(De->d_name))
			continue;

		if (lstat (De->d_name, &sb) >= 0) {

			/*
			** Save information about the current device
			*/
			if (ft) {
				this_device = sb.st_dev;
				ft = 0;
			}

			if (sb.st_dev != this_device)
				continue;

			if (debug > 0) {
				(void) fprintf (stderr, "Debug> ");
				print_fullpath (stderr, De->d_name);
				fputc ('\n', stderr);
				fflush (stderr);
			}

			/*
			** To save on filehandles and memory, temporary close
			** the current directory, process the new one, then resume
			** processing where we left off.
			*/
			if (S_ISDIR(sb.st_mode)) {
				const char	*current;
				int		gotlost;

				current = strdup (De->d_name);
				gotlost = 1;
				closedir (Dp);

				if (process_detail_dirs (current) < 0)
					return -1;
	
				if (!(Dp = opendir("."))) {
					perror ("opendir");
					exit (-1);
				}
				while ((De = readdir (Dp))) {
					if (!strcmp (De->d_name, current)) {
						gotlost = 0;
						break;
					}
				}
				if (gotlost) {
					(void) fprintf (stderr, "%s: got lost at about \"%s\"\n",
						basename, current);
					exit (-1);
				}
				if (current) {
					free (current);
					current = NULL;
				}
			} else {
				if (S_ISREG(sb.st_mode) && !strcmp (De->d_name, detnam)) {
					if (userid)
						detail_gulp_2 (De->d_name, repf);
					else
						detail_gulp (De->d_name);
					nproc++;
				}
			}
		}
	}

	process_fini:

	if (Dp) closedir (Dp);
	if (!DOTFILE(dir)) {
		chdir ("..");
	}

	return rv;
}

static char *
userfile (const char *user)
{
	char		*name = NULL;
	int		number = 0;

	do {
		FREE_VAR(name);

		name = mkcatstr ("%s-report%d.txt", 
			user, ++number);

		if (number > 8192) {
			FREE_VAR(name);
			name = mkcatstr ("%s-report%d.txt", user, 1);
			return name;
		}

	} while (!(access (name, F_OK)));

	return name;
}

