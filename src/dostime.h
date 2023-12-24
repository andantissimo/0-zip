#pragma once

#include <chrono>
#include <tuple>

std::tuple<uint16_t, uint16_t> to_dos_date_time(const std::chrono::time_point<std::chrono::file_clock> &);
