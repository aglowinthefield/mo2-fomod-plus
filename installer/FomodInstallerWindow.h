﻿#ifndef FOMODINSTALLERWINDOW_H
#define FOMODINSTALLERWINDOW_H

#include <qboxlayout.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "FomodPlusInstaller.h"
#include "xml/ModuleConfiguration.h"
#include "ui/ClickableWidget.h"

#include <QDialog>
#include <qradiobutton.h>
#include <QStackedWidget>
#include <QTextEdit>
#include <ui/ScaleLabel.h>

#include "FomodInstallerWindow.h"
#include "lib/FileInstaller.h"
#include "ui/FomodViewModel.h"

#include <QSplitter>

using namespace MOBase;

struct PluginData {
    std::shared_ptr<PluginViewModel> plugin;
    QAbstractButton* uiElement;
};


class FomodPlusInstaller;
/**
 * @class FomodInstallerWindow
 * @brief This class represents a window for the FOMOD installer.
 *
 * The FomodInstallerWindow class is designed to handle and manage the FOMOD installation
 * process. It integrates functionalities specific to FOMOD package installations.
 * By inheriting from QObject, it supports signal-slot mechanisms, enabling interaction
 * and communication with other components in the application.
 *
 * This class is primarily intended to provide a user interface and functionality
 * to process and run the FOMOD installer in a structured manner.
 */
class FomodInstallerWindow final : public QDialog {
    Q_OBJECT

public:
    FomodInstallerWindow(FomodPlusInstaller* installer,
        GuessedValue<QString>& modName,
        const std::shared_ptr<IFileTree>& tree,
        QString fomodPath,
        const std::shared_ptr<FomodViewModel>& viewModel,
        const nlohmann::json& fomodJson,
        QWidget* parent = nullptr);

    void closeEvent(QCloseEvent* event) override;

    void saveGeometryAndState() const;

    void restoreGeometryAndState();

    void populatePluginMap();


    // So FomodPlusInstaller can check if the user wants to manually install
    [[nodiscard]] bool isManualInstall() const
    {
        return mIsManualInstall;
    }

    [[nodiscard]] std::shared_ptr<FileInstaller> getFileInstaller() const { return mViewModel->getFileInstaller(); }

private slots:
    void onNextClicked();

    void updateCheckboxStates() const;

    void onPluginToggled(bool selected, const std::shared_ptr<GroupViewModel>& group,
        const std::shared_ptr<PluginViewModel>& plugin) const;

    void onPluginHovered(const std::shared_ptr<PluginViewModel>& plugin) const;

    void onSelectPreviousClicked() { this->selectPreviouslySelectedOptions(); }

    void onBackClicked() const;

    void onCancelClicked()
    {
        this->saveGeometryAndState();
        this->reject();
    }

    void onManualClicked()
    {
        mIsManualInstall = true;
        this->saveGeometryAndState();
        this->reject();
    }

    void onInstallClicked();

    [[deprecated]] void toggleImagesShown() const;

private:
    Logger& log = Logger::getInstance();
    FomodPlusInstaller* mInstaller;
    QString mFomodPath;
    GuessedValue<QString>& mModName;
    std::shared_ptr<IFileTree> mTree;
    std::shared_ptr<FomodViewModel> mViewModel;
    bool mInitialized{ false };
    std::unordered_map<QString, PluginData> mPluginMap;


    // Meta
    bool mIsManualInstall{};
    nlohmann::json mFomodJson;

    // Buttons
    QPushButton* mNextInstallButton{};
    QPushButton* mBackButton{};
    QPushButton* mCancelButton{};
    QPushButton* mManualButton{};
    QPushButton* mSelectPreviousButton{};
    QPushButton* mHideImagesButton{};

    void updateButtons() const;

    // Widgets
    QStackedWidget* mInstallStepStack{};
    QLabel* mDescriptionBox{};
    QComboBox* mModNameInput{};
    ScaleLabel* mImageLabel{};
    QWidget* mTopRow{};
    QSplitter* mCenterRow{};
    QWidget* mBottomRow{};

    // Fn
    void setupUi();

    void updateInstallStepStack();

    void updateDisplayForActivePlugin() const;

    void applyFnFromJson(const std::function<void(QAbstractButton*)>& fn);

    void stylePreviouslySelectedOptions();

    void selectPreviouslySelectedOptions() const;

    [[nodiscard]] QString getColorStyle() const;

    [[nodiscard]] QBoxLayout* createContainerLayout();

    [[nodiscard]] QSplitter* createCenterRow();

    [[nodiscard]] QWidget* createTopRow();

    [[nodiscard]] QComboBox* createModNameComboBox();

    [[nodiscard]] QWidget* createBottomRow();

    [[nodiscard]] QWidget* createLeftPane();

    [[nodiscard]] QWidget* createRightPane();

    [[nodiscard]] QWidget* createStepWidget(const std::shared_ptr<StepViewModel>& installStep);

    [[nodiscard]] QWidget* renderGroup(const std::shared_ptr<GroupViewModel>& group);

    static QString createObjectName(const std::shared_ptr<PluginViewModel>& plugin,
        const std::shared_ptr<GroupViewModel>& group);

    QRadioButton* createPluginRadioButton(const std::shared_ptr<PluginViewModel>& plugin,
        const std::shared_ptr<GroupViewModel>& group, QWidget* parent);

    QCheckBox* createPluginCheckBox(const std::shared_ptr<PluginViewModel>& plugin,
        const std::shared_ptr<GroupViewModel>& group, QWidget* parent);

    void renderSelectExactlyOne(QWidget* parent, QLayout* parentLayout, const std::shared_ptr<GroupViewModel>& group);

    void renderCheckboxGroup(QWidget* parent, QLayout* parentLayout,
        const std::shared_ptr<GroupViewModel>& group);

    QButtonGroup* renderRadioGroup(QWidget* parent, QLayout* parentLayout,
        const std::shared_ptr<GroupViewModel>& group);

    void logMessage(LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[WINDOW] " + message);
    };

};


#endif //FOMODINSTALLERWINDOW_H