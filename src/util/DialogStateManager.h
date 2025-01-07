#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>
#include <unordered_map>

#include "xml/ModuleConfiguration.h"

// I have some sort of aversion to all the ::. Maybe I have that tiny hole phobia lol
using std::string;
using std::unordered_map;

class DialogStateManager {
public:
  // TODO: Maybe we can limit the scope of what this state manager pulls in.
  explicit DialogStateManager(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

  void setFlag(const string& flag, const string& value);
  [[nodiscard]] string getFlag(const string& flag) const;

private:
  MOBase::IOrganizer* mOrganizer;
  unordered_map<string, string> mFlags;
};



#endif //DIALOGSTATEMANAGER_H
