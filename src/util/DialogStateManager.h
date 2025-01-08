#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>
#include <unordered_map>

#include "FlagMap.h"
#include "ConditionTester.h"

class DialogStateManager {
public:
  explicit DialogStateManager(MOBase::IOrganizer* organizer, std::unique_ptr<ModuleConfiguration> fomodFile)
  : mOrganizer(organizer),
    mFomodFile(std::move(fomodFile)) {
    mConditionTester = new ConditionTester(organizer);
  }

  bool isStepVisible(const InstallStep &step);

  void setFlag(const std::string &flag, const std::string &value);
  std::string getFlag(const std::string &flag);

private:
  MOBase::IOrganizer* mOrganizer;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
  FlagMap mFlags;
  ConditionTester* mConditionTester;
};



#endif //DIALOGSTATEMANAGER_H
