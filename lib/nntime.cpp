/*
 * nntime.cpp - time source + UTC calendar for the nx curl port.
 */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

namespace nn { namespace os {

struct Tick { int64_t value; };

Tick    GetSystemTick() noexcept;
int64_t ConvertToTimeSpan(Tick tick) noexcept;

}} // namespace nn::os

#define NNTIME_NS_PER_SEC 1000000000LL

static int64_t nntime_now_ns(void)
{
    return nn::os::ConvertToTimeSpan(nn::os::GetSystemTick());
}

extern "C" int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if(tv) {
        int64_t ns = nntime_now_ns();
        tv->tv_sec  = (long)(ns / NNTIME_NS_PER_SEC);
        tv->tv_usec = (long)((ns % NNTIME_NS_PER_SEC) / 1000);
    }
    return 0;
}

/* newlib routes its gettimeofday wrapper through this stub name. */
extern "C" int _gettimeofday(struct timeval *tv, void *tz)
{
    return gettimeofday(tv, tz);
}

extern "C" time_t time(time_t *t)
{
    time_t s = (time_t)(nntime_now_ns() / NNTIME_NS_PER_SEC);
    if(t)
        *t = s;
    return s;
}

/*
 * UTC seconds -> struct tm, matching nnSdk.
 */
static int nntime_secs_to_tm(int64_t t, struct tm *tm)
{
    /* 2000-03-01 00:00:00 UTC, in Unix seconds (== 0x38BC5D80). */
    static const int64_t LEAPOCH       = 946684800LL + 86400LL * (31 + 29);
    static const int64_t DAYS_PER_400Y = 365LL * 400 + 97;   /* 146097 */
    static const int64_t DAYS_PER_100Y = 365LL * 100 + 24;   /* 36524  */
    static const int64_t DAYS_PER_4Y   = 365LL * 4   + 1;    /* 1461   */
    /* month lengths starting from March; last entry is February. */
    static const int days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

    int64_t days, secs, years;
    int remdays, remsecs, remyears;
    int qc_cycles, c_cycles, q_cycles;
    int months, wday, yday, leap;

    /* reject values whose tm_year would overflow an int */
    if(t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
        return -1;

    secs = t - LEAPOCH;
    days = secs / 86400;
    remsecs = (int)(secs % 86400);
    if(remsecs < 0) {
        remsecs += 86400;
        days--;
    }

    wday = (int)((3 + days) % 7);
    if(wday < 0)
        wday += 7;

    qc_cycles = (int)(days / DAYS_PER_400Y);
    remdays = (int)(days % DAYS_PER_400Y);
    if(remdays < 0) {
        remdays += (int)DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / (int)DAYS_PER_100Y;
    if(c_cycles == 4)
        c_cycles--;
    remdays -= c_cycles * (int)DAYS_PER_100Y;

    q_cycles = remdays / (int)DAYS_PER_4Y;
    if(q_cycles == 25)
        q_cycles--;
    remdays -= q_cycles * (int)DAYS_PER_4Y;

    remyears = remdays / 365;
    if(remyears == 4)
        remyears--;
    remdays -= remyears * 365;

    leap = !remyears && (q_cycles || !c_cycles);
    yday = remdays + 31 + 28 + leap;
    if(yday >= 365 + leap)
        yday -= 365 + leap;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400LL * qc_cycles;

    for(months = 0; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    if(months >= 10) {
        months -= 12;
        years++;
    }

    if(years + 100 > INT_MAX || years + 100 < INT_MIN)
        return -1;

    tm->tm_year = (int)(years + 100);
    tm->tm_mon  = months + 2;
    tm->tm_mday = remdays + 1;
    tm->tm_wday = wday;
    tm->tm_yday = yday;
    tm->tm_hour = remsecs / 3600;
    tm->tm_min  = remsecs / 60 % 60;
    tm->tm_sec  = remsecs % 60;
    return 0;
}

extern "C" struct tm *gmtime_r(const time_t *timer, struct tm *result)
{
    if(!timer || !result)
        return NULL;
    if(nntime_secs_to_tm((int64_t)*timer, result) != 0) {
        errno = EOVERFLOW;
        return NULL;
    }
    result->tm_isdst = 0;
#ifdef __TM_GMTOFF
    result->__TM_GMTOFF = 0;
#endif
#ifdef __TM_ZONE
    result->__TM_ZONE = "UTC";
#endif
    return result;
}

extern "C" struct tm *gmtime(const time_t *timer)
{
    static struct tm g_tm;
    return gmtime_r(timer, &g_tm);
}
