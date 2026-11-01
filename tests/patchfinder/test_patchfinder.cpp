#include "../../patchfinder/lib/PatchFinder.h"
#include "FOMODData/FomodDB.h"

#include <QTemporaryDir>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
// Readable files with replacement denied exercise the real Windows commit path.
class ReadLockedFile final {
public:
  explicit ReadLockedFile(const QString& path)
      : mHandle(CreateFileW(path.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr))
  {
  }
  ~ReadLockedFile()
  {
    if (mHandle != INVALID_HANDLE_VALUE)
      CloseHandle(mHandle);
  }
  [[nodiscard]] bool isOpen() const { return mHandle != INVALID_HANDLE_VALUE; }

private:
  HANDLE mHandle;
};
} // namespace
#endif

class PatchFinderTest : public ::testing::Test {
protected:
  QTemporaryDir mDirectory;
  bool mCaptureFails = false;
  int mCaptures      = 0;
  std::function<bool(int, const std::string&)> mReinstall;
  std::function<bool(FomodDB&)> mRescan;
  PatchFinderInputs::InstalledState mInstalled { { "Master.esm" }, [](const std::string&) { return "Missing"; } };

  void writeDatabase(const nlohmann::json& data)
  {
    std::ofstream(mDirectory.filePath("fomod.db").toStdString()) << data.dump();
  }

  static nlohmann::json database()
  {
    return nlohmann::json::array({ FomodDbEntry(
        42, "Patch collection", { FomodOption("Useful patch", "patch.esp", { "Master.esm" }, "Step", "Group") })
            .toJson() });
  }

  PatchFinder makeFinder()
  {
    PatchFinderInputs inputs;
    inputs.captureInstalled = [this] {
      ++mCaptures;
      if (mCaptureFails)
        throw std::runtime_error("MO2 facts unavailable");
      return mInstalled;
    };
    inputs.reinstall = [this](int id, const std::string& name) { return mReinstall(id, name); };
    inputs.rescan    = [this](FomodDB& workspace) { return mRescan(workspace); };
    return PatchFinder(mDirectory.path().toStdString(), std::move(inputs));
  }
};

TEST_F(PatchFinderTest, RefreshPublishesEligibleSuggestions)
{
  writeDatabase(database());
  auto finder = makeFinder();

  const auto result = finder.apply({ PatchActionKind::Refresh });
  const auto view   = finder.view();

  EXPECT_TRUE(result.completed);
  EXPECT_EQ(view.freshness, PatchFreshness::Fresh);
  ASSERT_EQ(view.mods.size(), 1);
  EXPECT_EQ(view.mods[0].displayName, "Patch collection");
  EXPECT_EQ(view.mods[0].options[0].option.name, "Useful patch");
  EXPECT_EQ(view.suggestedCount, 1);
  EXPECT_TRUE(view.canModify);
}

TEST_F(PatchFinderTest, SearchUsesPublishedRowsAndKeepsUnfilteredCounts)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  for (const auto& term : { "MASTER", "USEFUL", "PATCH.ESP", "collection" }) {
    EXPECT_EQ(finder.view({ true, term }).mods.size(), 1);
    EXPECT_EQ(finder.view({ false, term }).mods.size(), 1);
  }
  EXPECT_TRUE(finder.view({ true, "unrelated" }).mods.empty());
  EXPECT_TRUE(finder.view({ false, "unrelated" }).mods.empty());
  EXPECT_EQ(finder.view({ true, "unrelated" }).suggestedCount, 1);
  EXPECT_EQ(mCaptures, 1);
}

TEST_F(PatchFinderTest, PreservesSelectionMastersAndInstalledFileEligibility)
{
  std::vector<FomodOption> options { { "Unknown", "unknown.esp", { "Master.esm" }, "", "", SelectionState::Unknown },
    { "Available", "available.esp", { "Master.esm" }, "", "", SelectionState::Available },
    { "Selected", "selected.esp", { "Master.esm" }, "", "", SelectionState::Selected },
    { "Deselected", "deselected.esp", { "Master.esm" }, "", "", SelectionState::Deselected },
    { "Already installed", "dir\\installed.esp", { "Master.esm" }, "", "" },
    { "Missing master", "other.esp", { "Missing.esm" }, "", "" }, { "No masters", "alone.esp", {}, "", "" },
    { "Folder has no plugin signal", "", { "Master.esm" }, "", "" } };
  writeDatabase(nlohmann::json::array({ FomodDbEntry(42, "Collection", options).toJson() }));
  mInstalled.plugins.insert("installed.esp");
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto view = finder.view();
  EXPECT_EQ(view.suggestedCount, 2);
  ASSERT_EQ(view.mods.size(), 1);
  ASSERT_EQ(view.mods[0].options.size(), 2);
  EXPECT_EQ(view.mods[0].options[0].option.name, "Unknown");
  EXPECT_EQ(view.mods[0].options[1].option.name, "Available");
  EXPECT_EQ(finder.view({ false }).mods[0].options.size(), 8);
}

TEST_F(PatchFinderTest, ConditionsAreEvaluatedDuringPublicationNotByQueries)
{
  auto data                             = database();
  data[0]["options"][0]["fileName"]     = "";
  data[0]["options"][0]["masters"]      = nlohmann::json::array();
  data[0]["options"][0]["typePatterns"] = nlohmann::json::array({ { { "type", "Recommended" },
      { "dependencies",
          { { "fileDependencies",
              nlohmann::json::array({ { { "file", "textures/test.dds" }, { "state", "Active" } } }) } } } } });
  data[0]["options"].push_back(data[0]["options"][0]);
  writeDatabase(data);
  std::string state  = "Active";
  int reads          = 0;
  mInstalled.resolve = [&](const std::string&) {
    ++reads;
    return state;
  };
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  state = "Missing";
  EXPECT_EQ(finder.view().suggestedCount, 2);
  EXPECT_EQ(finder.view({ false, "Useful" }).mods[0].options.size(), 2);
  EXPECT_EQ(reads, 1); // One captured lookup, shared by both options.
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().suggestedCount, 0);
  EXPECT_EQ(reads, 2);
}

TEST_F(PatchFinderTest, DismissalPersistsButDoesNotChangeBrowseEligibility)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;

  EXPECT_TRUE(finder.apply({ PatchActionKind::Dismiss, handle }).completed);
  EXPECT_EQ(finder.view().suggestedCount, 0);
  EXPECT_TRUE(finder.view().mods.empty());
  const auto browse = finder.view({ false });
  ASSERT_EQ(browse.mods.size(), 1);
  EXPECT_TRUE(browse.mods[0].options[0].suggested);
  EXPECT_TRUE(browse.mods[0].options[0].dismissed);
  EXPECT_EQ(browse.mods[0].options[0].option.selectionState, SelectionState::Unknown);

  auto reopened = makeFinder();
  ASSERT_TRUE(reopened.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_TRUE(reopened.view().mods.empty());
}

TEST_F(PatchFinderTest, DismissalSuccessIsIndependentOfSubsequentRefreshFailure)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;
  mCaptureFails     = true;

  const auto result = finder.apply({ PatchActionKind::Dismiss, handle });
  EXPECT_TRUE(result.completed);
  EXPECT_FALSE(result.error.empty());
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1); // Last complete publication, now read-only.

  mCaptureFails = false;
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_TRUE(finder.view().mods.empty());
}

TEST_F(PatchFinderTest, FailedDismissalReadDoesNotPretendToHideSuggestion)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;
  // A directory at the destination deterministically prevents publication.
  ASSERT_TRUE(QDir(mDirectory.path()).mkdir("fomod-dismissed.json"));

  EXPECT_FALSE(finder.apply({ PatchActionKind::Dismiss, handle }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1);
}

TEST_F(PatchFinderTest, ObsoleteAndStaleActionsCannotModifyAnything)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto old = finder.view().mods[0].options[0].handle;
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_FALSE(finder.apply({ PatchActionKind::Dismiss, old }).accepted);
  EXPECT_FALSE(finder.apply({ PatchActionKind::Reinstall, old }).accepted);
  const auto current = finder.view().mods[0].options[0].handle;
  mCaptureFails      = true;
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_FALSE(finder.apply({ PatchActionKind::Dismiss, current }).accepted);
  EXPECT_FALSE(finder.apply({ PatchActionKind::Reinstall, current }).accepted);
  EXPECT_EQ(mCaptures, 3);
}

TEST_F(PatchFinderTest, ReinstallRefreshesOnceAfterInstallerWritesSelections)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;
  mReinstall        = [&](int id, const std::string& name) {
    EXPECT_EQ(id, 42);
    EXPECT_EQ(name, "Patch collection");
    auto data                               = database();
    data[0]["options"][0]["selectionState"] = "Selected";
    writeDatabase(data);
    return true;
  };
  const auto result = finder.apply({ PatchActionKind::Reinstall, handle });
  EXPECT_TRUE(result.completed);
  EXPECT_EQ(mCaptures, 2);
  EXPECT_TRUE(finder.view().mods.empty());
  EXPECT_EQ(finder.view({ false }).mods[0].options[0].option.selectionState, SelectionState::Selected);
}

TEST_F(PatchFinderTest, RescanUsesPrivateWorkspaceAndRejectsReentrantActions)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;
  mRescan           = [&](FomodDB& workspace) {
    workspace.getEntries()[0]->getOptionsMutable().clear();
    EXPECT_EQ(finder.view().suggestedCount, 1);
    EXPECT_FALSE(finder.view().canModify);
    EXPECT_FALSE(finder.view().canRefresh);
    EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).accepted);
    EXPECT_FALSE(finder.apply({ PatchActionKind::Dismiss, handle }).accepted);
    return true;
  };
  EXPECT_TRUE(finder.apply({ PatchActionKind::Rescan }).completed);
  EXPECT_TRUE(finder.view().mods.empty());
  EXPECT_EQ(mCaptures, 2);
  auto reopened = makeFinder();
  ASSERT_TRUE(reopened.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_TRUE(reopened.view().mods.empty());
}

TEST_F(PatchFinderTest, CancelledRescanDiscardsWorkspace)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  mRescan = [](FomodDB& workspace) {
    workspace.getEntries()[0]->getOptionsMutable().clear();
    return false;
  };
  const auto result = finder.apply({ PatchActionKind::Rescan });
  EXPECT_TRUE(result.accepted);
  EXPECT_FALSE(result.completed);
  EXPECT_EQ(finder.view().suggestedCount, 1);
  EXPECT_EQ(mCaptures, 2);
}

TEST_F(PatchFinderTest, RescanCanRecoverFromCorruptFirstLoad)
{
  writeDatabase(nlohmann::json::object());
  auto finder = makeFinder();
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  mRescan = [](FomodDB& workspace) {
    workspace.addEntry(std::make_shared<FomodDbEntry>(database()[0]));
    return true;
  };
  EXPECT_TRUE(finder.apply({ PatchActionKind::Rescan }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Fresh);
  EXPECT_EQ(finder.view().suggestedCount, 1);
}

TEST_F(PatchFinderTest, FailedRescanSaveRetainsLastView)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  mRescan = [&](FomodDB& workspace) {
    workspace.getEntries()[0]->getOptionsMutable().clear();
    EXPECT_TRUE(QFile::remove(mDirectory.filePath("fomod.db")));
    EXPECT_TRUE(QDir(mDirectory.path()).mkdir("fomod.db"));
    return true;
  };
  EXPECT_FALSE(finder.apply({ PatchActionKind::Rescan }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1);
}

TEST_F(PatchFinderTest, MissingDatabaseAfterPublicationIsStaleRatherThanEmpty)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  ASSERT_TRUE(QFile::remove(mDirectory.filePath("fomod.db")));
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1);
  EXPECT_FALSE(finder.view().canModify);
}

#ifdef _WIN32
TEST_F(PatchFinderTest, FailedDismissalCommitPreservesReadableFile)
{
  writeDatabase(database());
  const auto path = mDirectory.filePath("fomod-dismissed.json");
  std::ofstream(path.toStdString()) << "[]";
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto handle = finder.view().mods[0].options[0].handle;
  ReadLockedFile lock(path);
  ASSERT_TRUE(lock.isOpen());

  const auto result = finder.apply({ PatchActionKind::Dismiss, handle });
  EXPECT_FALSE(result.completed);
  EXPECT_NE(result.error.find("Cannot save dismissed patches"), std::string::npos);
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), QByteArray("[]"));
  EXPECT_EQ(finder.view().suggestedCount, 1);
}

TEST_F(PatchFinderTest, FailedRescanCommitPreservesReadableDatabase)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  mRescan = [](FomodDB& workspace) {
    workspace.getEntries()[0]->getOptionsMutable().clear();
    return true;
  };
  const auto path = mDirectory.filePath("fomod.db");
  ReadLockedFile lock(path);
  ASSERT_TRUE(lock.isOpen());

  const auto result = finder.apply({ PatchActionKind::Rescan });
  EXPECT_FALSE(result.completed);
  EXPECT_NE(result.error.find("Cannot save FOMOD database"), std::string::npos);
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll().toStdString(), database().dump());
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1);
}
#endif

TEST_F(PatchFinderTest, MissingFirstUseDatabaseIsNormalEmptyState)
{
  auto finder = makeFinder();
  EXPECT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Fresh);
  EXPECT_EQ(finder.view().trackedMods, 0);
  EXPECT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Fresh);
}

TEST_F(PatchFinderTest, FailedFirstLoadIsNotReportedAsEmptySuccess)
{
  writeDatabase(nlohmann::json::object());
  auto finder = makeFinder();
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Unavailable);
  EXPECT_FALSE(finder.view().diagnostic.empty());
  EXPECT_FALSE(finder.view().canModify);
  EXPECT_TRUE(finder.view().canRefresh);
}

TEST_F(PatchFinderTest, MalformedLaterEntryRetainsWholeLastSuccessfulViewAndRecovers)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);

  auto broken              = database();
  broken[0]["displayName"] = "Unpublished replacement";
  broken.push_back({ { "modId", "not an integer" } });
  writeDatabase(broken);
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  const auto stale = finder.view();
  EXPECT_EQ(stale.freshness, PatchFreshness::Stale);
  ASSERT_EQ(stale.mods.size(), 1);
  EXPECT_EQ(stale.mods[0].displayName, "Patch collection");
  EXPECT_FALSE(stale.canModify);

  writeDatabase(nlohmann::json::array());
  EXPECT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Fresh);
  EXPECT_TRUE(finder.view().mods.empty());
  EXPECT_TRUE(finder.view().diagnostic.empty());
  // A caller-owned projection cannot change after publication of another revision.
  EXPECT_EQ(stale.mods[0].displayName, "Patch collection");
}

TEST_F(PatchFinderTest, FailedInstalledFactsDoNotPublishNewDatabase)
{
  writeDatabase(database());
  auto finder = makeFinder();
  ASSERT_TRUE(finder.apply({ PatchActionKind::Refresh }).completed);
  writeDatabase(nlohmann::json::array());
  mCaptureFails = true;
  EXPECT_FALSE(finder.apply({ PatchActionKind::Refresh }).completed);
  EXPECT_EQ(finder.view().freshness, PatchFreshness::Stale);
  EXPECT_EQ(finder.view().suggestedCount, 1);
}
