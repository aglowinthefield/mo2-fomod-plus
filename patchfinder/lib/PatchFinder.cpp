#include "PatchFinder.h"
#include "../../share/FOMODData/FomodDB.h"

#include <QCoreApplication>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <algorithm>

using namespace std;

namespace {
string dismissKey(int modId, const FomodOption& option)
{
  return to_string(modId) + ":" + (option.fileName.empty() ? option.name : option.fileName);
}

unordered_set<string> loadDismissed(const string& basePath)
{
  const auto path = filesystem::path(basePath) / "fomod-dismissed.json";
  if (!filesystem::exists(path))
    return {};
  ifstream file(path);
  if (!file.is_open())
    throw runtime_error(QCoreApplication::translate("PatchFinder", "Cannot read dismissed patches.").toStdString());
  const auto json = nlohmann::json::parse(file);
  if (!json.is_array())
    throw runtime_error(
        QCoreApplication::translate("PatchFinder", "Dismissed patches must contain an array.").toStdString());
  unordered_set<string> dismissed;
  for (const auto& key : json)
    dismissed.insert(key.get<string>());
  return dismissed;
}

void saveDismissed(const string& basePath, const unordered_set<string>& dismissed)
{
  const auto path = filesystem::path(basePath) / "fomod-dismissed.json";
  QSaveFile file(QString::fromStdString(path.string()));
  const auto data = nlohmann::json(dismissed).dump(2);
  if (!file.open(QIODevice::WriteOnly)
      || file.write(data.data(), static_cast<qint64>(data.size())) != static_cast<qint64>(data.size())
      || !file.commit())
    throw runtime_error(QCoreApplication::translate("PatchFinder", "Cannot save dismissed patches: %1")
            .arg(file.errorString())
            .toStdString());
}

bool isSuggested(const FomodOption& option, const PatchFinderInputs::InstalledState& installed)
{
  if (option.selectionState != SelectionState::Available && option.selectionState != SelectionState::Unknown)
    return false;
  if (!option.fileName.empty()
      && installed.plugins.contains(option.fileName.substr(option.fileName.find_last_of("/\\") + 1)))
    return false;

  const bool mastersMatch = !option.fileName.empty() && !option.masters.empty()
      && ranges::all_of(option.masters, [&](const string& master) { return installed.plugins.contains(master); });
  const auto type
      = installed.resolve ? ConditionEvaluator::resolveMatchingType(option.typePatterns, installed.resolve) : string {};
  return mastersMatch || type == "Recommended" || type == "Required";
}
} // namespace

#pragma region Initialization
PatchFinder::PatchFinder(string basePath, PatchFinderInputs inputs)
    : mBasePath(std::move(basePath))
    , mInputs(std::move(inputs))
{
}
#pragma endregion

#pragma region Actions
PatchActionResult PatchFinder::apply(PatchAction action)
{
  if (mBusy)
    return { false, false,
      QCoreApplication::translate("PatchFinder", "Another Patch Finder action is already running.").toStdString() };
  if (action.kind == PatchActionKind::Dismiss || action.kind == PatchActionKind::Reinstall) {
    if (mFreshness != PatchFreshness::Fresh)
      return { false, false,
        QCoreApplication::translate("PatchFinder", "Refresh patch suggestions before changing them.").toStdString() };
    if (action.handle.revision != mRevision || action.handle.mod >= mMods.size()
        || action.handle.option >= mMods[action.handle.mod].options.size())
      return { false, false,
        QCoreApplication::translate("PatchFinder", "This patch belongs to an obsolete view. Please retry.")
            .toStdString() };
  }
  if (action.kind == PatchActionKind::Reinstall && !mInputs.reinstall)
    return { false, false, QCoreApplication::translate("PatchFinder", "Reinstall is unavailable.").toStdString() };
  if (action.kind == PatchActionKind::Rescan && !mInputs.rescan)
    return { false, false, QCoreApplication::translate("PatchFinder", "Rescan is unavailable.").toStdString() };
  QScopedValueRollback busy(mBusy, true);
  PatchActionResult result { true, false, {} };
  try {
    if (action.kind == PatchActionKind::Dismiss) {
      const auto& mod    = mMods[action.handle.mod];
      const auto& option = mod.options[action.handle.option];
      if (!option.suggested || option.dismissed)
        return { false, false,
          QCoreApplication::translate("PatchFinder", "This patch is not an undismissed suggestion.").toStdString() };
      auto dismissed = loadDismissed(mBasePath);
      dismissed.insert(dismissKey(mod.modId, option.option));
      saveDismissed(mBasePath, dismissed);
      result.completed = true;
    } else if (action.kind == PatchActionKind::Reinstall) {
      const auto& mod  = mMods[action.handle.mod];
      result.completed = mInputs.reinstall(mod.modId, mod.displayName);
    } else if (action.kind == PatchActionKind::Rescan) {
      auto workspace = [&] {
        try {
          return FomodDB::loadChecked(mBasePath).value_or(FomodDB::createEmpty(mBasePath));
        } catch (const exception&) {
          // Rescan is also the recovery path for a corrupt or missing database.
          return FomodDB::createEmpty(mBasePath);
        }
      }();
      if (mInputs.rescan(workspace)) {
        workspace.saveChecked();
        result.completed = true;
      }
    } else if (action.kind != PatchActionKind::Refresh) {
      return { false, false, QCoreApplication::translate("PatchFinder", "Unknown Patch Finder action.").toStdString() };
    }
    refresh();
    if (action.kind == PatchActionKind::Refresh)
      result.completed = true;
    return result;
  } catch (const exception& error) {
    fail(error.what());
  } catch (...) {
    fail(QCoreApplication::translate("PatchFinder", "Could not refresh patch suggestions.").toStdString());
  }
  result.error = mDiagnostic;
  return result;
}

void PatchFinder::fail(const string& diagnostic)
{
  mFreshness  = mRevision == 0 ? PatchFreshness::Unavailable : PatchFreshness::Stale;
  mDiagnostic = diagnostic;
}

void PatchFinder::refresh()
{
  auto database = FomodDB::loadChecked(mBasePath);
  if (!database && mDatabaseWasPresent)
    throw runtime_error(
        QCoreApplication::translate("PatchFinder", "FOMOD database is missing. Rescan or restore it, then retry.")
            .toStdString());
  const auto dismissed = loadDismissed(mBasePath);
  auto installed       = mInputs.captureInstalled();
  if (installed.resolve)
    installed.resolve = makeCachedResolver(std::move(installed.resolve));
  vector<PatchModRow> mods;
  const auto revision = mRevision + 1;
  const auto entries  = database ? database->getEntries() : FOMODDBEntries {};
  for (const auto& entry : entries) {
    PatchModRow row { entry->getModId(), entry->getDisplayName(), {} };
    for (const auto& option : entry->getOptions()) {
      row.options.push_back({ { revision, mods.size(), row.options.size() }, option, isSuggested(option, installed),
          dismissed.contains(dismissKey(row.modId, option)) });
    }
    mods.push_back(std::move(row));
  }
  mDatabaseWasPresent = mDatabaseWasPresent || database.has_value();
  mMods               = std::move(mods);
  mRevision           = revision;
  mFreshness          = PatchFreshness::Fresh;
  mDiagnostic.clear();
}
#pragma endregion

#pragma region View
PatchView PatchFinder::view(const PatchQuery& query) const
{
  PatchView result;
  result.freshness   = mFreshness;
  result.diagnostic  = mDiagnostic;
  result.trackedMods = mMods.size();
  result.canModify   = mFreshness == PatchFreshness::Fresh && !mBusy;
  result.canRefresh  = !mBusy;
  result.canRescan   = !mBusy && static_cast<bool>(mInputs.rescan);
  const auto filter  = QString::fromStdString(query.filter).toLower();
  const auto matches = [&](const string& value) { return QString::fromStdString(value).toLower().contains(filter); };
  for (const auto& mod : mMods) {
    PatchModRow row { mod.modId, mod.displayName, {} };
    bool hasSuggestion = false;
    for (const auto& option : mod.options) {
      if (option.suggested && !option.dismissed) {
        ++result.suggestedCount;
        hasSuggestion = true;
      }
      if (query.suggestionsOnly && (!option.suggested || option.dismissed))
        continue;
      const auto& value = option.option;
      if (filter.isEmpty() || matches(mod.displayName) || matches(value.name) || matches(value.fileName)
          || ranges::any_of(value.masters, matches))
        row.options.push_back(option);
    }
    if (hasSuggestion)
      ++result.suggestedModCount;
    if (!row.options.empty())
      result.mods.push_back(std::move(row));
  }
  return result;
}
#pragma endregion
