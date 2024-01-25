#include "dostime.h"

using namespace std;
using namespace std::chrono;

static inline auto to_dos_date(const year &y, const month &m, const day &d)
{
    return static_cast<uint16_t>(
        (static_cast<int>(y) - 1980) << 9 |
        static_cast<unsigned>(m) << 5 |
        static_cast<unsigned>(d)
    );
}

static inline auto to_dos_time(const hours &h, const minutes &m, const seconds &s)
{
    return static_cast<uint16_t>(h.count() << 11 | m.count() << 5 | s.count() >> 1);
}

tuple<uint16_t, uint16_t> to_dos_date_time(const time_point<file_clock> &mtime)
{
#if _MSC_VER
    const auto t = current_zone()->to_local(utc_clock::to_sys(file_clock::to_utc(mtime)));
    const auto d = floor<days>(t);
    const auto y = year_month_day(d);
    const auto h = floor<hours>(t - d);
    const auto m = floor<minutes>(t - d - h);
    const auto s = floor<seconds>(t - d - h - m);
#else
    const auto t = file_clock::to_time_t(mtime);
    const auto u = duration_cast<system_clock::duration>(mtime.time_since_epoch())
                 + seconds(localtime(&t)->tm_gmtoff);
    const auto d = floor<days>(u);
    const auto y = year_month_day(sys_days(d));
    const auto h = floor<hours>(u - d);
    const auto m = floor<minutes>(u - d - h);
    const auto s = floor<seconds>(u - d - h - m);
#endif
    return make_tuple(to_dos_date(y.year(), y.month(), y.day()), to_dos_time(h, m, s));
}
