// https://github.com/ReneNyffenegger/cpp-read-configuration-files

#include "ConfigFile.h"
#include "Chameleon.h"
#include <cstddef>
#include <exception>
#include <file_utilities.h>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

class ConfigSectionException : public std::exception {
  const char* what() const noexcept override { return "section does not exist"; }
};

class ConfigEntryException : public std::exception {
  const char* what() const noexcept override { return "entry does not exist"; }
};
std::string trim(std::string const& source, char const* delims = " \t\r\n") {
  std::string            result(source);
  std::string::size_type index = result.find_last_not_of(delims);
  if (index != std::string::npos) {
    result.erase(++index);
  }

  index = result.find_first_not_of(delims);
  if (index != std::string::npos) {
    result.erase(0, index);
  } else {
    result.erase();
  }
  return result;
}

ConfigFile::ConfigFile(std::string configFile) : configFile_(std::move(configFile)) {
  std::ifstream file(configFile_.c_str());
  if (file.is_open()) {
    std::string line;
    std::string name;
    std::string value;
    std::string inSection;
    size_t      posEqual;
    while (std::getline(file, line)) {

      if (line.length() == 0) {
        continue;
      }

      if (line[0] == '#') {
        continue;
      }
      if (line[0] == ';') {
        continue;
      }

      if (line[0] == '[') {
        inSection = trim(line.substr(1, line.find(']') - 1));
        continue;
      }

      posEqual = line.find('=');
      name     = trim(line.substr(0, posEqual));
      value    = trim(line.substr(posEqual + 1));
      sections_[inSection].insert(std::make_pair(name, Chameleon(value)));
      // content_[inSection + '/' + name] = Chameleon(value);
    }
  } else {
    need_to_save_++;
  }
}

ConfigFile::~ConfigFile() {
  if (need_to_save_ > 0) {
    std::cout << "Saving configuration file to " << configFile_.c_str() << std::endl;
    auto          x = vtpl::utilities::create_directories_from_file_path(configFile_);
    std::ofstream file(configFile_.c_str());
    if (file.is_open()) {
      std::string name;
      std::string value;
      std::string inSection;
      for (auto& section : sections_) {
        inSection = section.first;
        file << "[" << inSection << "]" << std::endl;
        file << std::endl;
        for (auto& it1 : section.second) {
          file << it1.first << " = " << it1.second << std::endl;
        }
        file << std::endl;
      }
    } else {
      std::cout << "!!! Could not save [check the directory seperator] configuration file to " << configFile_.c_str()
                << std::endl;
    }
  }
}

Chameleon const& ConfigFile::Value(std::string const& section, std::string const& entry) const {
  auto ci = sections_.find(section);
  if (ci == sections_.end()) {
    throw ConfigSectionException();
  }
  auto ci1 = ci->second.find(entry);
  if (ci1 == ci->second.end()) {
    throw ConfigEntryException();
  }
  return ci1->second;
}

Chameleon const& ConfigFile::Value(std::string const& section, std::string const& entry, double value) {
  try {
    return Value(section, entry);
  } catch (ConfigSectionException&) {
    sections_[section].insert(std::make_pair(entry, Chameleon(value)));
    need_to_save_++;
    return Value(section, entry);
  } catch (ConfigEntryException&) { // section found but entry not found
    sections_[section].insert(std::make_pair(entry, Chameleon(value)));
    need_to_save_++;
    return Value(section, entry);
  }
}

Chameleon const& ConfigFile::Value(std::string const& section, std::string const& entry, std::string const& value) {
  try {
    return Value(section, entry);
  } catch (ConfigSectionException&) {
    sections_[section].insert(std::make_pair(entry, Chameleon(value)));
    need_to_save_++;
    return Value(section, entry);
  } catch (ConfigEntryException&) { // section found but entry not found
    sections_[section].insert(std::make_pair(entry, Chameleon(value)));
    need_to_save_++;
    return Value(section, entry);
  }
}