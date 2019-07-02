#pragma once

#include <istream>
#include <ostream>

#include "pkzip.h"

namespace zz::pkzip
{
    extern std::string charset;

    std::istream & operator >> (std::istream &, local_file_header &);
    std::ostream & operator << (std::ostream &, const local_file_header &);

    std::istream & operator >> (std::istream &, central_file_header &);
    std::ostream & operator << (std::ostream &, const central_file_header &);

    std::istream & operator >> (std::istream &, end_of_central_directory_record &);
    std::ostream & operator << (std::ostream &, const end_of_central_directory_record &);
}
