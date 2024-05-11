
#define	MINUTE_SECS		60
#define	HOUR_SECS		3600
#define	DAY_SECS		86400
#define	WEEK_SECS		604800

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

