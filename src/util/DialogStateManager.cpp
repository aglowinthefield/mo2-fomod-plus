#include "DialogStateManager.h"

#include <QString>
#include <ipluginlist.h>

void DialogStateManager::setFlag(const std::string &flag, const std::string &value) {
  mFlags[flag] = value;
}

std::string DialogStateManager::getFlag(const std::string &flag) const {
  const auto it = mFlags.find(flag);
  return it == mFlags.end() ? "" : it->second;
}

FileDependencyTypeEnum DialogStateManager::getFileDependencyStateForPlugin(const string &pluginName) const {
  const QFlags<MOBase::IPluginList::PluginState> pluginState = mOrganizer->pluginList()->state(QString::fromStdString(pluginName));

  if (pluginState & MOBase::IPluginList::STATE_MISSING) {
    return FileDependencyTypeEnum::Missing;
  }
  if (pluginState & MOBase::IPluginList::STATE_INACTIVE) {
    return FileDependencyTypeEnum::Inactive;
  }
  if (pluginState & MOBase::IPluginList::STATE_ACTIVE) {
    return FileDependencyTypeEnum::Active;
  }

  return FileDependencyTypeEnum::UNKNOWN_STATE;
}
