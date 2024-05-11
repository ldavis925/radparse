
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "stdc.h"
#include "externs.h"

/* Prototypes */
static	int 		detail_tag_type 	__P((const char *)),
			readline 		__P((FILE *, char **, size_t *));

static	char		*detail_extract_data 	__P((char *)),
			*extract_username_2 	__P((const char *)),
			*line_data_locator_2	__P((const char *)),
			*time_passed_spec 	__P((long secs));

inline	static	void	show_progress 		__P((int, struct stat *));
static 	void		update_date_index 	__P((void)),
			user_print_2 		__P((FILE *)),
			free_lines_2 		__P((void)),
			print_headers_2		__P((FILE *));

/* Info tags */
static	struct {
	const char	*t_name;
	t_type_t	t_type;
} tags[] = {
#define	USER_NAME_INDEX		0
	{"User-Name",			T_USER},
	{"Acct-Status-Type",		T_ACST},
	{"Acct-Input-Octets",		T_AINB},
	{"Acct-Output-Octets",		T_AOUB},
	{"Acct-Input-Packets",		T_AINP},
	{"Acct-Output-Packets",		T_AOUP},
	{"Acct-Session-Time",		T_TIME},
	{NULL,				T_BLAH}
};

/* detail_gulp() stuff */
static	int		currmon;
static	int		curryear;
static	char		*tag_data;
static	u_long		intermed;

/* detail_gulp_2() stuff */
static	char		*lines[MAX_LINES] = {NULL};
static	size_t		line_index = 0;
static	size_t		user_index = 0;
static	int		lineno = 0;
static 	int		print_headers = 0;
static 	int		lines_printed = 0;
#define	MAX_REPORT_LINES	56

void
detail_gulp_2 (const char *f, FILE *repf)
{
	FILE		*fp = fopen (f, "r");
	char		*line,
			*curuser;
	size_t		len;
	static int	ft = 1;
	static long	total_count = 0L;
	int		proc_ent = 0,
			rv;

	if (!fp) {
		perror (f);
		exit (-1);
	}

	/* initializations */
	line_index = user_index = 0;
	lineno = 0;
	ft = 1;

	if (!print_headers)
		print_headers_2 (repf);

	while (!((rv = readline (fp, &line, &len)) < 0 && len == 0)) {
		++lineno;
		if (ft) {
			if (!NEW_ENTRY(line))
				continue;
			ft = 0;
		}
		if (NEW_ENTRY(line)) {
			/*
			** It's not the first pass (a user is being processed) */
			if (proc_ent) {
				if (user_index < 0) {
					free_lines_2 ();
					proc_ent = 0;
					continue;
				}

				if (!(++total_count % 1000L)) {
					putchar ('.');
					fflush (stdout);
				}

				/* See if this is the user we're interested in */
				curuser = extract_username_2 (lines[user_index]);
				if (!strcmp (curuser, userid)) {
					user_print_2 (repf);
				}
				FREE_VAR(curuser);
				free_lines_2 ();
				proc_ent = 0;
				user_index = -1;
			}
		} else {
			/* Ok, now a user record is being processed */
			proc_ent = 1;

			if (line_index >= MAX_LINES) {
				fprintf (stderr,
				"Warning: MAX_LINES of %d is insufficient (readline=%d)!\n",
					MAX_LINES, lineno);
				fflush (stderr);
				fclose (repf);
				fclose (fp);
				exit (-1);
			}

			/* skip spaces */
			while (*line != '\0' && isspace((int)*line))
				++line;

			lines[line_index] = strdup(line);

			/* Keep a pointer to the user's name */
			if (strstr (lines[line_index], tags[USER_NAME_INDEX].t_name))
				user_index = line_index;

			/* increment for the next entry */
			++line_index;
		}
	}

	if (proc_ent) {
		if (user_index >= 0) {
			curuser = extract_username_2 (lines[user_index]);
			if (!strcmp (curuser, userid)) {
				user_print_2 (repf);
			}
			FREE_VAR(curuser);
		}
		free_lines_2 ();
	}

	putchar ('!');
	putchar ('\n');
	fflush (stdout);

	fclose (fp);
	return;
}

void
detail_gulp (const char *f)
{
	FILE		*fp = fopen (f, "r");
	int		proc_ent = 0,
			curtag,
			rv;
	struct stat	fsb;
	static int	ft = 1;
	char		*line;
	size_t		len;
	REC_ENT		rec;

	if (!fp) {
		perror(f);
		exit (-1);
	}

	if (progress) {
		intermed = 0UL;
		(void) fstat (fileno(fp), &fsb);
		(void) fprintf (stderr, "File size: %ld byte(s)\n",
			fsb.st_size);
	}

	/*
	** ft is the "first time" flag for (per detail file)
	*/
	ft = 1;

	while (!((rv = readline (fp, &line, &len)) < 0 && len == 0)) {
		show_progress (fileno(fp), &fsb);

		/*
		** This tidbit just guarantees that we start at a new entry
		*/
		if (ft) {
			if (!NEW_ENTRY(line))
				continue;

			currmon  = month_of (line);
			curryear = year_of (line);
			update_date_index ();
			ft = 0;
		}
		if (NEW_ENTRY(line)) {
			int		n_mo;

			/*
			** It's not the first pass (a user is being processed)
			*/
			if (proc_ent) {
				rec.r_month = (u_short)currmon;
				rec.r_year = (u_short)curryear;
				user_process (&rec);
				proc_ent = 0;
			}

			/*
			** Month changed in last line, sub process...
			*/
			if ((n_mo = month_of (line)) != currmon) {
				currmon = n_mo;
				curryear = year_of (line);
				update_date_index ();
			}

			memset (&rec, 0, sizeof rec);
			rec.r_status = S_BLAH;

		} else {
			/* Ok, now a user record is being processed */
			proc_ent = 1;

			if ((curtag = detail_tag_type (line)) >= 0) {
				tag_data = detail_extract_data (line);
				switch (tags[curtag].t_type) {
				case 	T_ACST:
					if (strcasecmp(tag_data,"Start")==0)
						rec.r_status = S_START;
					else if (strcasecmp(tag_data,"Stop")==0)
						rec.r_status = S_STOP;
					else rec.r_status = S_BLAH;
					break;
				case	T_USER:
					strncpy (rec.r_user, tag_data, DU_LEN);
					rec.r_user[DU_LEN] = '\0';
					break;
				case	T_AINB:
					rec.r_stats.st_bin = atol (tag_data);
					break;
				case	T_AOUB:
					rec.r_stats.st_bout = atol (tag_data);
					break;
				case	T_AINP:
					rec.r_stats.st_pin = atol (tag_data);
					break;
				case	T_AOUP:
					rec.r_stats.st_pout = atol (tag_data);
					break;
				case	T_TIME:
					rec.r_stats.st_time = 
						(u_long) strtol(tag_data, NULL, 10);
					break;
				default :
					break;
				}
			}
		}
	}

	/* Make the display look nice :) */
	if (progress) { fputc ('\n', stderr); fflush (stderr); }

	if (proc_ent) {
		rec.r_month = (u_short) currmon;
		rec.r_year = (u_short) curryear;
		user_process (&rec);
	}

	fclose (fp);
	return;
}

static int
detail_tag_type (const char *line)
{
	register int 	i;
	const char	*l = line;

	if (!(l && *l)) return -1;

	while (*l && isspace((int)*l))
		l++;

	if (!*l) return -1;

	for (i = 0; tags[i].t_name; i++)
		if (!strncmp (tags[i].t_name, l, strlen(tags[i].t_name)))
			return i;

	return -1;
}

static char *
detail_extract_data (char *d)
{
	size_t	dl = strlen(d);
	char	*t;

	t = d + dl;
	while (dl > 0) {
		if (isspace((int)d[dl-1]) || d[dl-1] == '\"') {
			d[dl-1] = '\0';
			dl--;
			t--;
		} else break;
	}

	while (dl > 0) {
		if (!(isspace((int)d[dl-1]) || d[dl-1] == '\"')) {
			dl--;
			t--;
		} else break;
	}
	return (strlen(t)) ? t : "";
}

inline static void
show_progress (int fd, struct stat *fsbp)
{
	if (progress && (++intermed % 500UL) == 0) {
		fprintf (stderr, "\r%3ld%% done. ",
			((u_longlong_t) tell(fd) * 100) / fsbp->st_size);
	}
	return;
}

static void
update_date_index (void)
{
	register int	i, f;

	if (debug > 0) {
		(void) fprintf (stderr, "Debug> date_index=(%d); mon=(%d); year = (%d)\n",
			date_index, currmon, curryear);
		fflush (stdout);
	}

	if (date_index >= D_LIMIT) {
		(void) fprintf (stderr, "Date index limit reached (%d) entries\n",
			D_LIMIT);
		exit (-1);
	}

	for (i = f = 0; i < date_index; i++) {
		if (date_list[i][M_IX] == currmon && date_list[i][Y_IX] == curryear) {
			++f;
			break;
		}
	}

	if (!f) {
		date_list[date_index][M_IX] = currmon;
		date_list[date_index++][Y_IX] = curryear;
	}
	return;
}

static int
readline (FILE *fp, char **buffer, size_t *len)
{
	register char	*cp;
	static char	_intbuf[1024];

	_intbuf[0] = '\0';
	*len = 0;
	*buffer = &_intbuf[0];

	if (!(fgets (_intbuf, sizeof _intbuf, fp)))
		return -1;

	if ((cp = strpbrk (_intbuf, "\r\n")))
		*cp = '\0';

	*len = strlen (_intbuf);
	return 0;
}

static char *
extract_username_2 (const char *radline)
{
	char		*tmplin, *uname;

	tmplin = strdup (radline);
	uname = strdup (detail_extract_data (tmplin));
	FREE_VAR(tmplin);
	return uname;
}

static void
free_lines_2 (void)
{
	register int	i;

	for (i = 0; i < line_index; i++) {
		FREE_VAR(lines[i]);
	}
	line_index = 0;

	return;
}

static void
user_print_2 (FILE *repf)
{
	status_t	stat = S_BLAH;
	register int	i, j;
	char 		fmt1[] = 
	{"%-10.10s %-20.20s %-15.15s %c%c%c-%c%c%c-%c%c%c%c %c%c%c-%c%c%c-%c%c%c%c\n%s"};
 	char		fmt2[] = 
	{"%-9d  %-9d            %-s\n\n"};
	char		*f1, *f2, *f3, *f4, *f5, *tstr;

	/* We only want Start and Stop records */
	f1 = line_data_locator_2 ("Acct-Status-Type");
	if (!f1 || toupper ((int)*f1) != 'S')
		return;

	/* print out a nice thingy */
	putchar ('o');
	fflush (stdout);

	if (!strcasecmp (f1, "start"))
		stat = S_START;
	else if (!strcasecmp (f1, "stop"))
		stat = S_STOP;

	/*
	** We wait until we know whether we have a start or stop record before
	** we break for headers (because there are only 2 lines printed on start) */
	if (lines_printed >= MAX_REPORT_LINES ||
	lines_printed + ((stat == S_START) ? 2 : 3) > MAX_REPORT_LINES)
		print_headers_2 (repf);

	f2 = line_data_locator_2 ("Timestamp");
	if (f2)
		tstr = timestamp_to_tstring (atol (f2));
	else
		tstr = "Unavailable";

	f3 = line_data_locator_2 ("Framed-IP-Address");
	if (!f3)
		f3 = "0.0.0.0";

	f4 = line_data_locator_2 ("Calling-Station-Id");
	if (!f4)
		f4 = "0000000000";

	f5 = line_data_locator_2 ("Called-Station-Id");
	if (!f5)
		f5 = "0000000000";

	fprintf (repf, fmt1,
		( stat == S_START ) ? "Connect" : 
		( stat == S_STOP  ) ? "Disconnect" : "Unknown",
		tstr,
		f3,
		f4[0], f4[1], f4[2], f4[3], f4[4],
		f4[5], f4[6], f4[7], f4[8], f4[9],
		f5[0], f5[1], f5[2], f5[3], f5[4],
		f5[5], f5[6], f5[7], f5[8], f5[9],
		((stat == S_START) ? "\n" : "")
	);

	++lines_printed;
	if (stat == S_START) {
		++lines_printed;
		return;
	}

	f1 = line_data_locator_2 ("Acct-Input-Octets");
	f2 = line_data_locator_2 ("Acct-Output-Octets");
	f3 = line_data_locator_2 ("Acct-Session-Time");

	fprintf (repf, fmt2,
		(f1) ? atol(f1) / 1024L : 0,
		(f2) ? atol(f2) / 1024L : 0,
		time_passed_spec ((f3) ? atol(f3) : 0)
	);

	lines_printed += 2;

	return;
}

static void
print_headers_2 (FILE *repf)
{
	static int		ff = 1;

	fprintf (repf, "%s\n\t\t\tFastQ Internet User Activity Report\n",
		(ff) ? "" : "\f");
	fprintf (repf, "\t\t\tActivity For Username: %-s\n", userid);
	fprintf (repf, "\t\t\tDate: %-20.20s MST (-0700)\n\n",
		timestamp_to_tstring (time(0)));

	fprintf (repf, "Status/    Timestamp/           IP Address/     Calling      Called\n");
	fprintf (repf, "KBytes In  KBytes Out           Minutes Online  Station      Station\n");
	fprintf (repf, 
	"--------------------------------------------------------------------------\n");

	print_headers = 1;
	lines_printed = 8;
	ff = 0;

	return;
}

static char *
line_data_locator_2 (const char *tag)
{
	register int		i;
	size_t			l;
	char			*cp;

	for (i = 0; i < line_index; i++) {
		if (!strncasecmp (lines[i], tag, strlen(tag))) {
			l = strlen(lines[i]);
			if (!(cp = strrchr (lines[i], '=')))
				return NULL;
			cp += 2;
			if (*cp == '\"')
				++cp;

			if (lines[i][l-1] == '\"')
				lines[i][l-1] = '\0';

			return cp;
		}
	}

	return NULL;
}

static char *
time_passed_spec (long secs)
{
	int			weeks, days, hours, minutes, seconds;
	static char		tpspec[64];
	long			wsecs;
	char			*cp;

	
	wsecs = secs;
	memset (tpspec, 0, sizeof (tpspec));

	/* # of weeks, and so on */
	weeks = wsecs / WEEK_SECS;
	wsecs %= WEEK_SECS;

	days = wsecs / DAY_SECS;
	wsecs %= DAY_SECS;

	hours = wsecs / HOUR_SECS;
	wsecs %= HOUR_SECS;

	minutes = wsecs / MINUTE_SECS;
	wsecs %= MINUTE_SECS;

	seconds = (int) wsecs;

	if (weeks) {
		cp = mkcatstr ("%d week%s", weeks, (weeks == 1) ? "" : "s");
		snprintf (tpspec, sizeof tpspec, "%s", cp);
		FREE_VAR(cp);
	}

	if (days) {
		cp = mkcatstr ("%s%s%d day%s",
			tpspec,
			tpspec[0] ? ", " : "",
			days,
			(days == 1) ? "" : "s"
		);
		snprintf (tpspec, sizeof tpspec, "%s", cp);
		FREE_VAR(cp);
	}

	if (hours) {
		cp = mkcatstr ("%s%s%d hour%s",
			tpspec,
			tpspec[0] ? ", " : "",
			hours,
			(hours == 1) ? "" : "s"
		);
		snprintf (tpspec, sizeof tpspec, "%s", cp);
		FREE_VAR(cp);
	}

	if (minutes) {
		cp = mkcatstr ("%s%s%d minute%s",
			tpspec,
			tpspec[0] ? ", " : "",
			minutes,
			(minutes == 1) ? "" : "s"
		);
		snprintf (tpspec, sizeof tpspec, "%s", cp);
		FREE_VAR(cp);
	}

	if (seconds) {
		cp = mkcatstr ("%s%s%d second%s",
			tpspec,
			tpspec[0] ? ", " : "",
			seconds,
			(seconds == 1) ? "" : "s"
		);
		snprintf (tpspec, sizeof tpspec, "%s", cp);
		FREE_VAR(cp);
	}

	if (!tpspec[0]) {
		strcpy (tpspec, "0 seconds");
	}

	return tpspec;
}

