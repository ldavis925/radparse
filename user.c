
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "stdc.h"
#include "externs.h"

/* This is base tree where everything goes... */
static	USER_REC	*u_tree = (USER_REC *)NULL;

static	USER_REC	*user_find	__P((USER_REC *,const char *)),
			*user_mod	__P((USER_REC *,REC_ENT *)),
			*user_new	__P((REC_ENT *));

static	void		user_next_node	__P((USER_REC *,DFTYPE, int *)),
			user_clear_data	__P((int *, USER_REC *)),
			user_print_node	__P((USER_REC *, const char *, int *)),
			user_imp_print	__P((USER_REC *, const char *, int *)),
			user_rpt_print	__P((USER_REC *,const char *, int *)),
			user_insert	__P((USER_REC *));

static	int		adds = 0, updates = 0;
FILE			*fspec;

int
user_list (void)
{
	char	d[8], df[18];
	int	status;
	int	n;

	if (!adds)
		return;

	for (n = 0; n < date_index; n++) {
		status = 1;
		memcpy (d, date_to_str(date_list[n][M_IX], date_list[n][Y_IX]), 8);
		strncpy (df, d, 8);
		df[7] = '\0';
		strcat (df, importf ? "-imp.txt" : "-rpt.txt");
		fspec = fopen (df, "w+");
		if (!fspec) {
			perror(df);
			exit (-1);
		}

		user_print_node (u_tree, d, &status);
		fclose (fspec);
	}

	return 0;
}

int
user_clear (void)
{
	int	status;

	user_next_node (u_tree, user_clear_data, &status);
	return 0;
}

int
user_process (REC_ENT *rp)
{
	USER_REC	*u_rec;

	/* Only process Acct-Status-Type of Stop */
	if (rp->r_status != S_STOP || !strlen(rp->r_user))
		return ;

	if ((u_rec = user_find (u_tree, rp->r_user))) {
		++updates;
		(void) user_mod (u_rec, rp);
	} else {
		++adds;
		(void) user_new (rp);
	}

	return 0;
}

static USER_REC *
user_new (REC_ENT *r_buf)
{
	USER_REC	*u_rec;
	struct accums	*a_rec;
	char		*dstr;

	/* sanity check */
	if ((user_find (u_tree, r_buf->r_user)))
		return (USER_REC *) NULL;

	u_rec = (USER_REC *) calloc (1, sizeof(USER_REC));
	if (!u_rec) {
		perror ("user_new: calloc");
		exit (-1);
	}

	a_rec = (struct accums *) calloc (1, sizeof(struct accums));
	if (!a_rec) {
		perror ("user_new: calloc:");
		exit (-1);
	}
	
	dstr = date_to_str (r_buf->r_month, r_buf->r_year);
	memcpy (a_rec->a_date, dstr, sizeof (a_rec->a_date));
	memcpy (u_rec->u_user, r_buf->r_user, sizeof (u_rec->u_user));
	u_rec->u_accums = a_rec;

	u_rec->u_accums->a_totbin = r_buf->r_stats.st_bin;
	u_rec->u_accums->a_totbout = r_buf->r_stats.st_bout;
	u_rec->u_accums->a_totpin = r_buf->r_stats.st_pin;
	u_rec->u_accums->a_totpout = r_buf->r_stats.st_pout;
	u_rec->u_accums->a_tottime = r_buf->r_stats.st_time;
	u_rec->u_accums->a_totconn = (u_long) 1;

	if (u_tree)
		user_insert (u_rec);
	else
		u_tree = u_rec;

	if (debug > 1) {
		(void) fprintf (stderr, "Debug> user_new (%s)\n", r_buf->r_user);
	}
	return u_rec;
}

static void
user_insert (USER_REC *u_rec)
{
	register USER_REC	*rp, *lrp;
	int			left;

	if (!u_tree) return;

	for (rp = u_tree; rp; ) {
		lrp = rp;
		left = (strcmp (u_rec->u_user, rp->u_user) < 0);
		rp = (left) ? rp->u_left : rp->u_right;
	}

	if (left)
		lrp->u_left = u_rec;
	else
		lrp->u_right = u_rec;

	return;
}

static USER_REC *
user_mod (USER_REC *u_buf, REC_ENT *r_buf)
{
	char			*dstr;
	register struct accums	*a;

	if (!u_buf || !r_buf)
		return (USER_REC *) NULL;

	dstr = date_to_str (r_buf->r_month, r_buf->r_year);
	for (a = u_buf->u_accums; a; a = a->a_next)
		if (strncmp (a->a_date, dstr, 7) == 0)
			break;

	if (!a) {
		a = (struct accums *) calloc (1, sizeof (struct accums));
		if (!a) {
			perror ("user_mod: calloc");
			exit (-1);
		}
		a->a_next = u_buf->u_accums;
		u_buf->u_accums = a;
		memcpy (a->a_date, dstr, 8);
	}
	
	a->a_totbin  += r_buf->r_stats.st_bin;
	a->a_totbout += r_buf->r_stats.st_bout;
	a->a_totpin  += r_buf->r_stats.st_pin;
	a->a_totpout += r_buf->r_stats.st_pout;
	a->a_tottime += r_buf->r_stats.st_time;
	a->a_totconn++;

	return u_buf;
}

static USER_REC *
user_find (USER_REC *u_point, const char *name)
{
	register int		cmp;
	register USER_REC	*urp;

	if (!u_point)
		return (USER_REC *) NULL;

	if ((cmp = strcmp (name, u_point->u_user)) == 0)
		return u_point;

	if (cmp < 0)
		return user_find (u_point->u_left, name);

	return user_find (u_point->u_right, name);
}

static void
user_next_node (USER_REC *urp, DFTYPE ufunc, int *status)
{
	if (urp) {
		user_next_node (urp->u_left, ufunc, status);
		(*ufunc) (status, urp);
		user_next_node (urp->u_right, ufunc, status);
	}
	return;
}

static void
user_print_node (USER_REC *urp, const char *d, int *status)
{
	if (urp) {
		user_print_node (urp->u_left, d, status);

		if (importf) user_imp_print (urp, d, status);
		else user_rpt_print (urp, d, status);

		user_print_node (urp->u_right, d, status);
	}
	return;
}

static void
user_imp_print (USER_REC *u, const char *d, int *hdr)
{
	register struct accums	*a;

	if (hdr && *hdr) {
		/* (void) fprintf (stdout, "# %s\n", d); */
		*hdr = 0; /* No header for now */
	}

	for (a = u->u_accums; a; a = a->a_next) {
		if (strncmp (a->a_date, d, 7) == 0) {
			fprintf (fspec, "%-8.16s, %9.9s, %d\n",
				u->u_user, 
				timespec(a->a_tottime),
				a->a_totconn
			);
			break;
		}
	}
	return;	
}

static void
user_rpt_print (USER_REC *urp, const char *d, int *hdr)
{
	register struct accums	*a;

	if (hdr && *hdr) {
		fprintf (fspec, "\f\n\t\t\tRadius log report for %s\n\n", d);
		fprintf (fspec, "User                 Hours     Data In    Data Out");
		fprintf (fspec, "    PackIn   PackOut Connects\n");
		fprintf (fspec, "-----------------------------------------------");
		fprintf (fspec, "-----------------------------\n");
		*hdr = 0;
	}
	for (a = urp->u_accums; a; a = a->a_next) {
		if (strncmp (a->a_date, d, 7) == 0) {
			fprintf (fspec, "%-16.16s %9s %10ldK %10ldK %9ld %9ld %d\n",
				urp->u_user, timespec (a->a_tottime),
				a->a_totbin/1024UL,
				a->a_totbout/1024UL,
				a->a_totpin, a->a_totpout,
				a->a_totconn
			);
			break;
		}
	}
	return;
}

static void
user_clear_data (int *status, USER_REC *u)
{
	register struct accums	*a;

	for (a = u->u_accums; a; a = a->a_next) {
		a->a_totbin = a->a_totbout = a->a_totpin = 0UL;
		a->a_totpout = a->a_tottime = a->a_totconn = 0UL;
	}

	*status = 0; /* OK */
	return;
}

