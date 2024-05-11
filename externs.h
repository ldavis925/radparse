#ifndef	EXTERNS_H
#define EXTERNS_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {S_STOP, S_START, S_BLAH} status_t;
typedef	enum {	T_USER, T_ACST, T_AINB, T_AOUB,
		T_AINP, T_AOUP, T_TIME, T_BLAH} t_type_t;

struct stats {
	u_long		st_bin;		/* bytes-in */
	u_long		st_bout;	/* bytes-out */
	u_long		st_pin;		/* packets-in */
	u_long		st_pout;	/* packets-out */
	u_long		st_time;	/* session time */
};

struct accums {
	char		a_date[8];	/* e.g., "May1999" */
	u_long		a_totbin;	/* (same as stats) */
	u_long		a_totbout;	/*       :         */
	u_long		a_totpin;
	u_long		a_totpout;
	u_long		a_tottime;	/* Tot. seconds */
	u_long		a_totconn;	/* Tot. #-connects */
	struct accums	*a_next;
};

#define	UR_UF_SUBPROC	0x0001

typedef	struct userrec {
#define	DU_LEN		16
	char		u_user[DU_LEN+1];
	struct accums	*u_accums;
	u_int		u_flags;
	struct userrec	*u_left;
	struct userrec	*u_right;
} USER_REC;

typedef struct rec_ent {
	char		r_user[DU_LEN+1];
	status_t	r_status;
	struct stats	r_stats;
	u_short		r_month;
	u_short		r_year;
} REC_ENT;

typedef			void (*DFTYPE) (int *, USER_REC *);

#define	FREE_VAR(p)	{if(p){free((void*)p);p=NULL;}}
#define	NEW_ENTRY(e)	(isalpha((int)(e)[0]) && isspace((int)(e)[3]))
#define	SEC_2_HOUR(s)	((s) / 3600)
#define	SEC_2_MIN(s)	((s) / 60)
#define D_LIMIT		24
#define	MAX_LINES	50
#define	M_IX		0
#define	Y_IX		1
#define	MINUTE_SECS	60
#define	HOUR_SECS	3600
#define	DAY_SECS	86400
#define	WEEK_SECS	604800

void		detail_gulp	__P((const char *)),
		detail_gulp_2	__P((const char *, FILE *)),
		print_fullpath	__P((FILE *, const char *));
int		user_process	__P((REC_ENT *)),
		user_list	__P((void)),
		month_of	__P((char *)),
		month_name	__P((const char *)),
		my_itoa		__P((int, char *, int));

u_short		year_of		__P((char *));
char		*year_to_str	__P((u_short)),
		*timespec	__P((u_long)),
		*date_to_str	__P((u_short, u_short));

extern	char	*mkcatstr	__P((const char *, ...));

extern 	char 	*timestamp_to_tstring __P((long));

extern	int	debug, progress, importf;
extern	char	*month_names[], *detnam;
extern	u_short	date_list[][2], date_index;
extern	char	*userid;

#ifdef __cplusplus
{
#endif

#endif /* EXTERNS_H */
