#ifndef TZFILE_H
#define TZFILE_H

/*
** This file is in the public domain, so clarified as of
** June 5, 1996 by Arthur David Olson (arthur_david_olson@nih.gov).
*/

/*
** This header is for use ONLY with the time conversion code.
** There is no guarantee that it will remain unchanged,
** or that it will remain at all.
** Do NOT copy it to any system include directory.
** Thank you!
*/




#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#ifndef USG

/*
** Use of the underscored variants may cause problems if you move your code to
** certain System-V-based systems; for maximum portability, use the
** underscore-free variants.  The underscored variants are provided for
** backward compatibility only; they may disappear from future versions of
** this file.
*/

#define SECS_PER_MIN	SECSPERMIN
#define MINS_PER_HOUR	MINSPERHOUR
#define HOURS_PER_DAY	HOURSPERDAY
#define DAYS_PER_WEEK	DAYSPERWEEK
#define DAYS_PER_NYEAR	DAYSPERNYEAR
#define DAYS_PER_LYEAR	DAYSPERLYEAR
#define SECS_PER_HOUR	SECSPERHOUR
#define SECS_PER_DAY	SECSPERDAY
#define MONS_PER_YEAR	MONSPERYEAR

#endif /* !defined USG */

#endif /* !defined TZFILE_H */
