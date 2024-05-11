
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/param.h>
#include <ctype.h>
#include <time.h>

#include "stdc.h"
#include "externs.h"

char	*month_names [12] = {
	"Jan","Feb","Mar","Apr",
	"May","Jun","Jul","Aug",
	"Sep","Oct","Nov","Dec"
};

void
print_fullpath (FILE *stream, const char *s)
{
	char 	*t;

	(void) fprintf (stream, (t = getcwd (NULL, MAXPATHLEN)));
	free (t);
	(void) fprintf (stream, "/%s", s);
	return;
}

char *
timespec (u_long secs)
{
	static char 	tspec[10];
	int		hr, mn, sc;
	char		*tp;

	tp = tspec;
	hr = (int) SEC_2_HOUR(secs);
	if (hr > 99) {
		(void) my_itoa (hr, tp, 3);
		tp += 3;
	} else {
		(void) my_itoa (hr, tp, 2);
		tp += 2;
	}
	*tp++ = ':';

	secs -= (int)(hr * 3600);
	mn = (int) SEC_2_MIN(secs);
	(void) my_itoa (mn, tp, 2);
	tp += 2;
	*tp++ = ':';

	secs -= (int)(mn * 60);
	(void) my_itoa (secs, tp, 2);
	*(tp += 2) = '\0';

	return tspec;
}

char *
year_to_str (u_short y)
{
	static char	yts[5];

	(void) my_itoa (y, yts, 4);
	return yts;
}

char *
date_to_str (u_short m, u_short y)
{
	static char	dts[8];

	strncpy (dts, month_names[m %= 12], 3);
	strncpy (dts + 3, year_to_str (y), 4);
	dts[7] = '\0';
	return dts;
}

u_short
year_of (char *ctime_str)
{
	register char	*cp;

	if (!(cp = strrchr (ctime_str, ' ')))
		return (u_short) 0;

	return (u_short) atoi (++cp);
}

int
month_of (char *ctime_str)
{
	int	m_ndx;
	char	tmp;

	tmp = ctime_str[7];
	ctime_str[7] = '\0';
	m_ndx = month_name (ctime_str + 4);
	ctime_str[7] = tmp;
	return m_ndx;
}

int
month_name (const char *s)
{
	register int i;
	for (i=0; i < 12; ++i)
		if (strncasecmp(month_names [i], s, 3) == 0)
			return i;

	return 0;
}

/*
** Homebrew itoa(), --lane
*/
int
my_itoa (int i, char *n, int digits)
{
	register int	v, rep, base = 1;
	register char	ch;

	for (rep=0; rep < digits-1; rep++) base *= 10;
	for (rep = 0; base > 0 && rep < digits; rep++,base /= 10) {
		if (isdigit((int)(ch = (char)((v = i / base) + '0')))) *n++ = ch;
		else break;
		i -= (v * base);
	}
	*n = '\0';
	return rep;
}

extern char *
mkcatstr (const char *str, ...)
{
	va_list		ap;
	FILE		*devnull;
	char		*buff;
	int		nprint;

	if (!str) 
		return NULL;

	if ((devnull = fopen ("/dev/null", "r+")) == NULL) {
		return NULL;
	}

	va_start (ap, str);
	/*
	** Off into the pit of darkness it goes, but we find out how much
	** is written :-)
	*/
	nprint = vfprintf (devnull, str, ap);
	va_end (ap);

	fclose (devnull);

	/* User is responsible for free()'ing, include 1 for NULL */
	buff = (char *) calloc (1, ++nprint);

	va_start (ap, str);
	vsnprintf (buff, nprint, str, ap);
	va_end (ap);

	return buff;
}

extern char *
timestamp_to_tstring (long tstamp)
{
	/* returns short version of ctime: e.g.: *\
	\* Dec 15 2001 15:15:15                  */
	time_t				ts;
	struct tm			*tms;
	static char			rettime[21];

	if (!tstamp)
		return;

	ts = (time_t) tstamp;
	tms = localtime (&ts);
	snprintf (rettime, sizeof (rettime), "%3.3s %02d %04d %02d:%02d:%02d",
		month_names[tms->tm_mon],
		tms->tm_mday,
		tms->tm_year + 1900,
		tms->tm_hour,
		tms->tm_min,
		tms->tm_sec
	);

	return rettime;	
}
