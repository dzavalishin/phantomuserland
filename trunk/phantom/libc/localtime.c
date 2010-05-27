#include <phantom_time.h>


static char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char* monthNames[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
static char days[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char* dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


static char* numbers[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
//static int monthDay[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

#if 0
static int _getMonthFromYDay(int yDay, int isLeapYear)
{
	int i = 12;
	int month_first;
	do
	{
		i--;
		month_first = monthDay[i] + ((i < 2) ? 0 : isLeapYear);
	}
	while(yDay < month_first);
	return i;
}
//#endif

static long _getLastDayOfYear(long year)
{
	return 365L * year
	+ ((long)year)/4L
	- ((long)year)/400L;
}

//#if 0
static long _getYear(long day)
{
	long y1 = day / 366;
	
	for(; _getLastDayOfYear(y1) <= day; y1++){}
	
	return y1;	
}
#endif

#if 0
static int _isLeapYear(int year)
{
	if(year % 4 == 0 && year % 400 != 0)
	{
		return 1;
	}
	return 0;
}

static int _getDayOfYear(int year, int month, int dayOfMonth)
{
	return dayOfMonth
		+ monthDay[month] 
		+ ((month > 1) ? _isLeapYear(year): 0)
		- 1;
}

static long _getDay(int year, int month, int dayOfMonth)
{
	return _getDayOfYear(year, month, dayOfMonth)
		+ _getLastDayOfYear(year - 1);
}
#endif

/*
struct tm *localtime_rb(bigtime_t timer, struct tm *tmb)
{
	long long esec = timer/1000000L;
	long eday = esec / 86400L;
	long daySec = esec % 86400L;
	long dayMin = daySec / 60;
	
	tmb->tm_year = _getYear(eday);
	tmb->tm_yday = eday - _getLastDayOfYear(tmb->tm_year - 1);
	tmb->tm_wday = (eday + 3) % 7;
	tmb->tm_mon  = _getMonthFromYDay(tmb->tm_yday, _isLeapYear(tmb->tm_year));
	tmb->tm_mday = tmb->tm_yday - monthDay[tmb->tm_mon] + 1;
	tmb->tm_hour = dayMin / 60;
	tmb->tm_min = dayMin % 60;
	tmb->tm_sec = daySec % 60;
	
	return tmb;
}
*/
static void _write(char* output, size_t* outdex, char* val, size_t max)
{
	int index = 0;
	/*printf("outdex: %ld max: %ld val[index]: %c\n", *outdex, max, val[index]);*/
	while(*outdex < (max - 1) && val[index] != '\0')
	{
		output[(*outdex)++] = val[index++];
		/*printf("outdex: %ld max: %ld val[index]: %c\n", *outdex, max, val[index]);*/
	}
}



size_t strftime(char *str, size_t maxsize, const char *format, const struct tm *timeptr)
{
	size_t outdex = 0;
	if(maxsize == 0)
	{
		return 0;
	}

	while(*format != '\0' && outdex < (maxsize - 1))
	{
		char c = *format++;
		if(c != '%')
		{
			/*printf("copy: %c\n", c);*/
			str[outdex++] = c;
		}
		else
		{
			c = *format++;
			switch(c)
			{
				case '\0':
					str[outdex++] = '\0';
					return outdex;
				case '%':
					_write( str, &outdex, "%", maxsize);
				break;
				case 'a':
					_write( str, &outdex, days[timeptr->tm_wday], maxsize);
				break;
				case 'A':
					_write( str, &outdex, dayNames[timeptr->tm_wday], maxsize);
				break;
				case 'b':
					_write( str, &outdex, months[timeptr->tm_mon], maxsize);
				break;
				case 'B':
					_write( str, &outdex, monthNames[timeptr->tm_mon], maxsize);
				break;
				case 'c':
				break;
				case 'd':
					_write( str, &outdex, numbers[timeptr->tm_mday / 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_mday % 10], maxsize);
				break;
				case 'H':
					_write( str, &outdex, numbers[timeptr->tm_hour / 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_hour % 10], maxsize);
				break;
				case 'I':
				{
					int hour = (timeptr->tm_hour > 12) 
						? timeptr->tm_hour-12 
						: timeptr->tm_hour;
					_write( str, &outdex, numbers[hour / 10], maxsize);
					_write( str, &outdex, numbers[hour % 10], maxsize);
				}
				break;
				case 'j':
					_write( str, &outdex, numbers[timeptr->tm_yday / 100], maxsize);
					_write( str, &outdex, numbers[(timeptr->tm_yday / 10) % 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_yday % 10], maxsize);
				break;
				case 'm':
					_write( str, &outdex, numbers[timeptr->tm_mon / 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_mon % 10], maxsize);
				break;
				case 'M':
					_write( str, &outdex, numbers[timeptr->tm_min / 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_min % 10], maxsize);
				break;
				case 'p':
					_write( str, &outdex, timeptr->tm_hour > 11 ? "pm": "am", maxsize);
				break;
				case 'S':
					_write( str, &outdex, numbers[timeptr->tm_sec / 10], maxsize);
					_write( str, &outdex, numbers[timeptr->tm_sec % 10], maxsize);
				break;
				case 'U':
				{
					int week = (timeptr->tm_yday >= timeptr->tm_wday) 
						? (timeptr->tm_yday - timeptr->tm_wday) / 7 + 1
						: 0;
					_write( str, &outdex, numbers[week / 10], maxsize);
					_write( str, &outdex, numbers[week % 10], maxsize);
				}
				break;
				case 'w':
					_write( str, &outdex, numbers[timeptr->tm_wday], maxsize);
				break;
				case 'W':
				{
					int week = (timeptr->tm_yday >= (timeptr->tm_wday + 1)) 
						? (timeptr->tm_yday - (timeptr->tm_wday + 1)) / 7 + 1
						: 0;
					_write( str, &outdex, numbers[week / 10], maxsize);
					_write( str, &outdex, numbers[week % 10], maxsize);
				}
				break;
				case 'x':
				break;
				case 'X':
				break;
				case 'y':
				{
					int cYear = timeptr->tm_year % 100;
					_write( str, &outdex, numbers[cYear / 10], maxsize);
					_write( str, &outdex, numbers[cYear % 10], maxsize);
				}
				break;
				case 'Y':
				{
					int year = timeptr->tm_year % 10000;
					_write( str, &outdex, numbers[year / 1000], maxsize);
					_write( str, &outdex, numbers[(year / 100) % 10], maxsize);
					_write( str, &outdex, numbers[(year / 10) % 10], maxsize);
					_write( str, &outdex, numbers[year % 10], maxsize);
				}
				case 'Z':
				break;
/*
%c	date and time representation
%x	date representation
%X	time representation
%Z	time zone (blank, if time zone not available)
%%	%
*/
			}
		}
	}
	str[outdex++] = '\0';
	return outdex;
}


/*
time_t mktime(struct tm* timeptr)
{
	long day = _getDay(timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday);
	long long secs =  60 * (60 * timeptr->tm_hour + timeptr->tm_min) 
					  + timeptr->tm_sec 
					  + 86400L * day;

	return (time_t) secs;
}
  */
