#include "logger.h"

namespace logger {

const char LEVEL_CHAR[] = "IEWF-";
const char LEVEL_CHAR_SEP = LEVEL_CHAR[4];
const char EXAMPLE[] = "Mon Apr 23 17:48:14 2012";
const int EXAMPLE_LEN = sizeof(EXAMPLE);
const char TIME_FORMAT[] = "%a %b %d %H:%M:%S %Y";

Logger::Logger(const Level level, std::ostream *os)
    : level_(level), level_char_(LEVEL_CHAR[level]), os_(os) {}

const Logger &Logger::printprefix() const {
  const time_t rawtime = time(nullptr);
  const struct tm *timeinfo = localtime(&rawtime);

  static char buffer_[EXAMPLE_LEN];
  strftime(buffer_, 80, TIME_FORMAT, timeinfo);
  (*stream()) << level_char_ << LEVEL_CHAR_SEP << buffer_;
  return *this;
}

} // namespace logger