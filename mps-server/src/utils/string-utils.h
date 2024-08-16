#ifndef _MPS_UTILS_STRING_UTILS_H_
#define _MPS_UTILS_STRING_UTILS_H_
#include <string>
#include <string_view>
#include <vector>

namespace utils
{

std::vector<std::string>
SplitString(const std::string_view string, const std::string_view& delimiters);

std::vector<std::string> SplitString(const std::string_view string, const char delimiter);

} // namespace utils

#endif // _MPS_UTILS_STRING_UTILS_H_
