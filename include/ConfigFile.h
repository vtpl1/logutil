#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#include <core_export.h>
#include <map>
#include <string>
#include <version.h>

#include "Chameleon.h"

class CORE_EXPORT ConfigFile {
  // std::map<std::string, Chameleon> content_;
  std::map<std::string, std::map<std::string, Chameleon>> sections_;
  std::string                                             configFile_;
  int                                                     need_to_save_{0};

public:
  ConfigFile(std::string configFile);
  ~ConfigFile();

  Chameleon const& Value(std::string const& section, std::string const& entry) const;

  Chameleon const& Value(std::string const& section, std::string const& entry, double value);
  Chameleon const& Value(std::string const& section, std::string const& entry, std::string const& value);
};

#endif