#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "stdc.h"
#include "externs.h"
#include "readline.h"

/* Info tags */
static	struct {
	const char	*t_name;
	t_type_t	t_type;
} tags[] = {
	{"User-Name",			T_USER},
	{"Acct-Status-Type",		T_ACST},
	{"Acct-Input-Octets",		T_AINB},
	{"Acct-Output-Octets",		T_AOUB},
	{"Acct-Input-Packets",		T_AINP},
	{"Acct-Output-Packets",		T_AOUP},
	{"Acct-Session-Time",		T_TIME},
	{NULL,				T_BLAH}
};

static	int		currmon;
static	int		curryear;

static	int 		detail_tag_type __P((const char *));
static	char		*detail_extract_data __P((char *));
static	void		show_progress __P((int, struct stat *)),
			update_date_index __P((void));
static	char		*tag_data;
static	u_long		intermed;

void
detail_gulp (const char *f)
{
	int		fd = open (f, O_RDONLY, 0),
			proc_ent = 0,
			curtag,
			rv;
	struct stat	fsb;
	static int	ft;
	char		*line;
	size_t		len;
	REC_ENT		rec;

	if (fd < 0) {
		perror("open");
		exit (-1);
	}

	if (progress) {
		intermed = 0UL;
		(void) fstat (fd, &fsb);
		(void) fprintf (stderr, "File size: %ld byte(s)\n",
			fsb.st_size);
	}

	/*
	** ft is the "first time" flag for (per detail file)
	*/
	ft = 1;

	while (!((rv = readline (fd, &line, &len)) < 0 && len == 0)) {
		show_progress (fd, &fsb);

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

	close (fd);
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

static void
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

