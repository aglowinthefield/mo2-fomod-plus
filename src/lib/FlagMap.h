#ifndef FLAGMAP_H
#define FLAGMAP_H
#include <unordered_map>
#include <string>

class FlagMap : public std::unordered_map<std::string, std::string> {
public:
  std::string getFlag(const std::string &flag);
  void setFlag(const std::string &flag, const std::string &value);
};



#endif //FLAGMAP_H
