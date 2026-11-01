#pragma once

#include "../../share/FOMODData/ConditionEvaluator.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

class FomodDB;

enum class PatchFreshness { Fresh, Stale, Unavailable };

struct PatchHandle {
  std::uint64_t revision = 0;
  std::size_t mod        = 0;
  std::size_t option     = 0;
};

enum class PatchActionKind { Refresh, Dismiss, Reinstall, Rescan };

struct PatchAction {
  PatchActionKind kind = PatchActionKind::Refresh;
  PatchHandle handle;
};

struct PatchActionResult {
  bool accepted  = false;
  bool completed = false;
  std::string error;
};

struct PatchOptionRow {
  PatchHandle handle;
  FomodOption option;
  bool suggested = false;
  bool dismissed = false;
};

struct PatchModRow {
  int modId;
  std::string displayName;
  std::vector<PatchOptionRow> options;
};

struct PatchQuery {
  bool suggestionsOnly = true;
  std::string filter;
};

struct PatchView {
  PatchFreshness freshness = PatchFreshness::Unavailable;
  std::string diagnostic;
  std::vector<PatchModRow> mods;
  std::size_t trackedMods       = 0;
  std::size_t suggestedCount    = 0;
  std::size_t suggestedModCount = 0;
  bool canModify                = false;
  bool canRefresh               = true;
  bool canRescan                = true;
};

// Only MO2 operations cross this seam; database/dismissal files remain real in tests.
// Callbacks throw on failure and return false for a cancelled action.
struct PatchFinderInputs {
  struct InstalledState {
    std::unordered_set<std::string> plugins;
    PluginStateResolver resolve;
  };
  std::function<InstalledState()> captureInstalled;
  std::function<bool(int modId, const std::string& displayName)> reinstall;
  std::function<bool(FomodDB& workspace)> rescan;
};

class PatchFinder final {
public:
  PatchFinder(std::string basePath, PatchFinderInputs inputs);

  [[nodiscard]] PatchActionResult apply(PatchAction action);
  [[nodiscard]] PatchView view(const PatchQuery& query = {}) const;

private:
  std::string mBasePath;
  PatchFinderInputs mInputs;
  std::vector<PatchModRow> mMods;
  PatchFreshness mFreshness = PatchFreshness::Unavailable;
  std::string mDiagnostic;
  std::uint64_t mRevision  = 0;
  bool mBusy               = false;
  bool mDatabaseWasPresent = false;

  void refresh();
  void fail(const std::string& diagnostic);
};
