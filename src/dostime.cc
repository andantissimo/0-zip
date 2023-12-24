#include "dostime.h"

using namespace std;
using namespace std::chrono;

tuple<uint16_t, uint16_t> to_dos_date_time(const time_point<file_clock> &mtime)
{
    const auto time = file_clock::to_time_t(mtime);
    auto t = duration_cast<system_clock::duration>(mtime.time_since_epoch())
           + seconds(localtime(&time)->tm_gmtoff);
    const auto d = floor<days>(t);
    const auto ymd = year_month_day(sys_days(d));
    auto dos_date = static_cast<uint16_t>(
        (static_cast<int>(ymd.year()) - 1980) << 9 |
        static_cast<unsigned>(ymd.month()) << 5 |
        static_cast<unsigned>(ymd.day())
    );
    t -= d;
    const auto h = floor<hours>(t).count();
    t -= hours(h);
    const auto m = floor<minutes>(t).count();
    t -= minutes(m);
    const auto s = floor<seconds>(t).count();
    auto dos_time = static_cast<uint16_t>(h << 11 | m << 5 | s >> 1);
    return make_tuple(dos_date, dos_time);
}
