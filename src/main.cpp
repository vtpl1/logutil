#include "logging.h"
#include "sinks/rotating_sqllite_sink.h"
#include <spdlog/sinks/rotating_file_sink.h>

int main(/*int argc, char const* argv[]*/)
{
  // Create a file rotating logger with 5 MB size max and 3 rotated files
  auto max_size = 100 * 1024;
  auto max_files = 3;
  // auto logger = spdlog::rotating_logger_mt("some_logger_name", "logs/rotating.txt", max_size, max_files);
  auto logger = vtpl::rotating_sqllite_logger_mt("test_logger", "logs/test.db", max_size, max_files);
  int counter = 1024 * 1024;
  while (--counter > 0) {
    logger->info("Create a file rotating logger with 5 MB size max and 3 rotated files");
    logger->info("1234,1,0,28.5,0.0,0.0,");
    break;
  }

  return 0;
}
