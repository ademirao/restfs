#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <iostream>

enum Level { INFO, ERROR, WARNING, FATAL };

namespace logger {

class Logger final {
public:
  Logger(const Logger &) = delete;
  Logger &operator=(Logger const &) = delete;
  Logger(const Level level, std::ostream *os);
  ~Logger() {
    *os_ << std::endl;
    if (level_ == FATAL) {
      std::exit(EXIT_FAILURE);
    }
  }
  const Logger &printprefix() const;
  std::ostream *stream() const { return os_; }

private:
  const Level level_;
  const char &level_char_;
  std::ostream *const os_;
};


} // namespace logger

#define LOG(LEVEL)                                                             \
  (*(::logger::Logger(LEVEL, &std::cerr).printprefix().stream())               \
   << " [" << __FILE__ << ":" << __LINE__ << "] ")
#define CHECK(EXP)                                                             \
  if (!(EXP)) {                                                                \
    LOG(FATAL) << #EXP << " evaluate to false.";                               \
  }
#endif