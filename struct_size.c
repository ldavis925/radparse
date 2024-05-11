#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>

#include "stdc.h"
#include "externs.h"

int
main (int c, char **v)
{
	u_int		nds, nrs;
	u_long		totd, totr;

	if (c != 3) {
		(void) fprintf (stderr, "usage: %s dates entries\n", v[0]);
		exit (-1);
	}

	nds = atoi (v[1]);
	nrs = atoi (v[2]);

	(void) fprintf (stdout, "date_recsize(%d) * dates(%d) = %ld\n",
		sizeof (struct accums), nds,
		(totd = sizeof (struct accums) * nds));

	(void) fprintf (stdout, "user_recsize(%d) + date_recsize(%ld) = %ld\n",
		sizeof (struct userrec), totd,
		(totr = sizeof (struct userrec) + totd));

	(void) fprintf (stdout, "total_recsize(%ld) * entries(%ld) = %ld\n",
		totr, nrs, totr * nrs);

	fflush (stdout);
	exit (0);
}

