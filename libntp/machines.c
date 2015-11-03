/* machines.c - provide special support for peculiar architectures
 *
 * Real bummers unite !
 *
 */

#include <unistd.h>

#include "config.h"

#ifdef HAVE_SYS_TIMEX_H
# include <sys/time.h>	/* prerequisite on NetBSD */
# include <sys/timex.h>
#endif

#include "ntp.h"
#include "ntp_machine.h"
#include "ntp_syslog.h"
#include "ntp_stdlib.h"
#include "ntp_unixtime.h"
#include "lib_strbuf.h"
#include "ntp_debug.h"
#include "ntp_syscall.h"

#ifdef SYS_WINNT
int _getch(void);	/* Declare the one function rather than include conio.h */
#else

#if !defined(HAVE_NTP_GETTIME) && defined(HAVE_NTP_ADJTIME)
int ntp_gettime(struct ntptimeval *ntv)
{
	struct timex tntx;
	int result;

	ZERO(tntx);
	result = ntp_adjtime(&tntx);
	ntv->time = tntx.time;
	ntv->maxerror = tntx.maxerror;
	ntv->esterror = tntx.esterror;
#  ifdef NTP_API
#   if NTP_API > 3
	ntv->tai = tntx.tai;
#   endif
#  endif
	return result;
}
#endif	/* !HAVE_NTP_GETTIME */

#define SET_TOD_UNDETERMINED	0
#define SET_TOD_CLOCK_SETTIME	1
#define SET_TOD_SETTIMEOFDAY	2
#define SET_TOD_STIME		3

const char * const set_tod_used[] = {
	"undetermined",
	"clock_settime",
	"settimeofday",
	"stime"
};

pset_tod_using	set_tod_using = NULL;


int
ntp_set_tod(
	struct timespec *tvs,
	void *tzp
	)
{
	static int	tod;
	int		rc;
	int		saved_errno;

	TRACE(1, ("In ntp_set_tod\n"));
	rc = -1;
	saved_errno = 0;

#ifdef HAVE_CLOCK_SETTIME
	if (rc && (SET_TOD_CLOCK_SETTIME == tod || !tod)) {
		errno = 0;
		rc = clock_settime(CLOCK_REALTIME, tvs);
		saved_errno = errno;
		TRACE(1, ("ntp_set_tod: clock_settime: %d %m\n", rc));
		if (!tod && !rc)
			tod = SET_TOD_CLOCK_SETTIME;

	}
#endif /* HAVE_CLOCK_SETTIME */
#ifdef HAVE_SETTIMEOFDAY
	if (rc && (SET_TOD_SETTIMEOFDAY == tod || !tod)) {
		struct timeval adjtv;

		/*
		 * Some broken systems don't reset adjtime() when the
		 * clock is stepped.
		 */
		adjtv.tv_sec = adjtv.tv_usec = 0;
		adjtime(&adjtv, NULL);
		errno = 0;

		adjtv.tv_sec = tvs->tv_sec;
		adjtv.tv_usec = (tvs->tv_nsec + 500) / 1000;
		rc = settimeofday(&adjtv, tzp);
		saved_errno = errno;
		TRACE(1, ("ntp_set_tod: settimeofday: %d %m\n", rc));
		if (!tod && !rc)
			tod = SET_TOD_SETTIMEOFDAY;
	}
#endif /* HAVE_SETTIMEOFDAY */
	errno = saved_errno;	/* for %m below */
	TRACE(1, ("ntp_set_tod: Final result: %s: %d %m\n",
		  set_tod_used[tod], rc));
	/*
	 * Say how we're setting the time of day
	 */
	if (!rc && NULL != set_tod_using) {
		(*set_tod_using)(set_tod_used[tod]);
		set_tod_using = NULL;
	}

	if (rc)
		errno = saved_errno;

	return rc;
}

#endif /* not SYS_WINNT */

#if defined (SYS_WINNT)
/* getpass is used in ntpq.c */

char *
getpass(const char * prompt)
{
	int c, i;
	static char password[32];

	fprintf(stderr, "%s", prompt);
	fflush(stderr);

	for (i=0; i<sizeof(password)-1 && ((c=_getch())!='\n' && c!='\r'); i++) {
		password[i] = (char) c;
	}
	password[i] = '\0';

	fputc('\n', stderr);
	fflush(stderr);

	return password;
}
#endif /* SYS_WINNT */
