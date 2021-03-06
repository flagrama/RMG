/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SettingsDialog.hpp"
#include "../../Globals.hpp"
#include "Utilities/SettingsID.hpp"

#include <QFileDialog>

using namespace UserInterface::Dialog;
using namespace M64P::Wrapper;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->treeWidget->topLevelItem(0)->setSelected(true);
    this->treeWidget->expandAll();

    this->inGame = g_EmuThread->isRunning();
    if (inGame)
    {
        g_MupenApi.Core.GetRomInfo(&this->gameInfo);
        g_MupenApi.Core.GetDefaultRomInfo(&this->defaultGameInfo);
    }
    else
    {
        this->hideEmulationInfoText();
        this->treeWidget->topLevelItem(1)->setDisabled(true);
    }

    for (int i = 0; i < this->stackedWidget->count(); i++)
        this->reloadSettings(i);

    int width = g_Settings.GetIntValue(SettingsID::GUI_SettingsDialogWidth);
    int height = g_Settings.GetIntValue(SettingsID::GUI_SettingsDialogHeight);

    if (width != 0 && height != 0)
    {
        // center current dialog
        this->setGeometry(
            QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(width, height), parent->geometry()));
    }
}

SettingsDialog::~SettingsDialog(void)
{
    g_Settings.SetValue(SettingsID::GUI_SettingsDialogWidth, this->size().width());
    g_Settings.SetValue(SettingsID::GUI_SettingsDialogHeight, this->size().height());
}

void SettingsDialog::restoreDefaults(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        loadDefaultCoreSettings();
        break;
    case 1:
        loadDefaultGameSettings();
        break;
    case 2:
        loadDefaultGameCoreSettings();
        break;
    case 3:
        loadDefaultGamePluginSettings();
        break;
    case 4:
        loadPluginSettings();
        break;
    case 5:
        loadDefaultDirectorySettings();
        break;
    case 6:
        loadDefaultKeybindSettings();
        break;
    case 7:
        loadDefaultBehaviorSettings();
        break;
    }
}

void SettingsDialog::reloadSettings(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        loadCoreSettings();
        break;
    case 1:
        loadGameSettings();
        break;
    case 2:
        loadGameCoreSettings();
        break;
    case 3:
        loadGamePluginSettings();
        break;
    case 4:
        loadPluginSettings();
        break;
    case 5:
        loadDirectorySettings();
        break;
    case 6:
        loadKeybindSettings();
        break;
    case 7:
        loadBehaviorSettings();
        break;
    }
}

void SettingsDialog::loadCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool debugger = false;
    bool overrideGameSettings = false;

    disableExtraMem = g_Settings.GetBoolValue(SettingsID::Core_DisableExtraMem);
    counterFactor = g_Settings.GetIntValue(SettingsID::Core_CountPerOp);
    cpuEmulator = g_Settings.GetIntValue(SettingsID::Core_CPU_Emulator);
    siDmaDuration = g_Settings.GetIntValue(SettingsID::Core_SiDmaDuration);
    randomizeInterrupt = g_Settings.GetBoolValue(SettingsID::Core_RandomizeInterrupt);
    debugger = g_Settings.GetBoolValue(SettingsID::Core_EnableDebugger);
    overrideGameSettings = g_Settings.GetBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySize->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactor->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDuration->setValue(siDmaDuration);
}

void SettingsDialog::loadGameSettings(void)
{
    this->gameGoodName->setText(this->gameInfo.Settings.goodname);
    this->gameMemorySize->setCurrentIndex(!this->gameInfo.Settings.disableextramem);
    this->gameSaveType->setCurrentIndex(this->gameInfo.Settings.savetype);
    this->gameCounterFactor->setCurrentIndex(this->gameInfo.Settings.countperop - 1);
    this->gameSiDmaDuration->setValue(this->gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0;

    QString section = QString(this->gameInfo.Settings.MD5);

    overrideEnabled = g_Settings.GetBoolValue(SettingsID::Game_OverrideCoreSettings, section);
    cpuEmulator = g_Settings.GetIntValue(SettingsID::Game_CPU_Emulator, section);
    randomizeInterrupt = g_Settings.GetBoolValue(SettingsID::Game_RandomizeInterrupt, section);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->pluginVideoPlugins, this->pluginAudioPlugins, this->pluginInputPlugins,
                                   this->pluginRspPlugins};
    SettingsID settingsId[4] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin, SettingsID::Game_INPUT_Plugin,
                        SettingsID::Game_RSP_Plugin};
    QComboBox *comboBox;

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->clear();
        comboBox->addItem("**Use Core Plugin Settings**");
    }

    QString section = this->gameInfo.Settings.MD5;

    for (const Plugin_t &p : g_Plugins.GetAvailablePlugins())
    {
        comboBox = comboBoxArray[(int)p.Type];
        comboBox->addItem(p.Name, p.FileName);
        if (g_Settings.GetStringValue(settingsId[(int)p.Type], section) == p.FileName)
        {
            comboBox->setCurrentText(p.Name);
        }
    }
}

void SettingsDialog::loadPluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    SettingsID settingsIdArray[] = {SettingsID::Core_GFX_Plugin, SettingsID::Core_AUDIO_Plugin, SettingsID::Core_INPUT_Plugin, SettingsID::Core_RSP_Plugin};
    PluginType type;
    QComboBox *comboBox;
    QString pluginFileName;
    int index = 0;

    for (const Plugin_t &p : g_Plugins.GetAvailablePlugins())
    {
        comboBox = comboBoxArray[(int)p.Type];
        pluginFileName = g_Settings.GetStringValue(settingsIdArray[(int)p.Type]);

        comboBox->addItem(p.Name, p.FileName);

        if (pluginFileName == p.FileName)
        {
            comboBox->setCurrentText(p.Name);
        }
    }
}

void SettingsDialog::loadDirectorySettings(void)
{
    // these need to be static, otherwise Qt will segfault
    static QString screenshotDir;
    static QString saveStateDir;
    static QString saveSramDir;
    static QString sharedDataDir;
    static bool overrideUserDirs = false;
    static QString userDataDir;
    static QString userCacheDir;

    screenshotDir = g_Settings.GetStringValue(SettingsID::Core_ScreenshotPath);
    saveStateDir = g_Settings.GetStringValue(SettingsID::Core_SaveStatePath);
    saveSramDir = g_Settings.GetStringValue(SettingsID::Core_SaveSRAMPath);
    sharedDataDir = g_Settings.GetStringValue(SettingsID::Core_SharedDataPath);

    overrideUserDirs = g_Settings.GetBoolValue(SettingsID::Core_OverrideUserDirs);
    userDataDir = g_Settings.GetStringValue(SettingsID::Core_UserDataDirOverride);
    userCacheDir = g_Settings.GetStringValue(SettingsID::Core_UserCacheDirOverride);

    this->screenshotDirLineEdit->setText(screenshotDir);
    this->saveStateDirLineEdit->setText(saveStateDir);
    this->saveSramDirLineEdit->setText(saveSramDir);
    this->sharedDataDirLineEdit->setText(sharedDataDir);
    this->overrideUserDirsGroupBox->setChecked(overrideUserDirs);
    this->userDataDirLineEdit->setText(userDataDir);
    this->userCacheDirLineEdit->setText(userCacheDir);
}

void SettingsDialog::loadKeybindSettings(void)
{
    KeyBindButton *buttons[] = {this->openRomKeyButton,   this->openComboKeyButton,      this->startEmuKeyButton,
                                this->endEmuKeyButton,    this->refreshRomListKeyButton, this->exitKeyButton,

                                this->softResetKeyButton, this->hardResetKeyButton,      this->generateBitmapKeyButton,
                                this->limitFPSKeyButton,  this->swapDiskKeyButton,       this->saveStateKeyButton,
                                this->saveAsKeyButton,    this->loadStateKeyButton,      this->loadKeyButton,
                                this->cheatsKeyButton,    this->gsButtonKeyButton,       this->fullscreenKeyButton,
                                this->settingsKeyButton};

    SettingsID id;
    for (int i = 0; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
    {
        id = (SettingsID)(SettingsID::KeyBinding_OpenROM + i);
        buttons[i]->setText(g_Settings.GetStringValue(id));
    }
}

void SettingsDialog::loadBehaviorSettings(void)
{
    bool pause = false, resume = false;

    /*
        pause = g_Settings.GetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss);
        resume = g_Settings.GetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus);
    */

    this->manualResizingCheckBox->setChecked(g_Settings.GetBoolValue(SettingsID::GUI_AllowManualResizing));
}

void SettingsDialog::loadDefaultCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool debugger = false;
    bool overrideGameSettings;

    disableExtraMem = g_Settings.GetDefaultBoolValue(SettingsID::Core_DisableExtraMem);
    counterFactor = g_Settings.GetDefaultIntValue(SettingsID::Core_CountPerOp);
    cpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Core_CPU_Emulator);
    siDmaDuration = g_Settings.GetDefaultIntValue(SettingsID::Core_SiDmaDuration);
    randomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Core_RandomizeInterrupt);
    debugger = g_Settings.GetDefaultBoolValue(SettingsID::Core_EnableDebugger);
    overrideGameSettings = g_Settings.GetDefaultBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!this->coreOverrideGameSettingsGroup->isChecked())
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySize->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactor->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDuration->setValue(siDmaDuration);
}

void SettingsDialog::loadDefaultGameSettings(void)
{
    this->gameGoodName->setText(this->gameInfo.Settings.goodname);
    this->gameMemorySize->setCurrentIndex(!this->gameInfo.Settings.disableextramem);
    this->gameSaveType->setCurrentIndex(this->gameInfo.Settings.savetype);
    this->gameCounterFactor->setCurrentIndex(this->gameInfo.Settings.countperop - 1);
    this->gameSiDmaDuration->setValue(this->gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadDefaultGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0;

    overrideEnabled = g_Settings.GetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    cpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Game_CPU_Emulator);
    randomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadDefaultGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->pluginVideoPlugins, this->pluginAudioPlugins, this->pluginInputPlugins,
                                   this->pluginRspPlugins};

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->setCurrentIndex(0);
    }
}

void SettingsDialog::loadDefaultDirectorySettings(void)
{
    this->screenshotDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_ScreenshotPath));
    this->saveStateDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SaveStatePath));
    this->saveSramDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SaveSRAMPath));
    this->sharedDataDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SharedDataPath));
    this->overrideUserDirsGroupBox->setChecked(g_Settings.GetDefaultBoolValue(SettingsID::Core_OverrideUserDirs));
    this->userDataDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_UserDataDirOverride));
    this->userCacheDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_UserCacheDirOverride));
}

void SettingsDialog::loadDefaultKeybindSettings(void)
{
    KeyBindButton *buttons[] = {this->openRomKeyButton,   this->openComboKeyButton,      this->startEmuKeyButton,
                                this->endEmuKeyButton,    this->refreshRomListKeyButton, this->exitKeyButton,

                                this->softResetKeyButton, this->hardResetKeyButton,      this->generateBitmapKeyButton,
                                this->limitFPSKeyButton,  this->swapDiskKeyButton,       this->saveStateKeyButton,
                                this->saveAsKeyButton,    this->loadStateKeyButton,      this->loadKeyButton,
                                this->cheatsKeyButton,    this->gsButtonKeyButton,       this->fullscreenKeyButton,
                                this->settingsKeyButton};

    SettingsID id;
    for (int i = 0; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
    {
        id = (SettingsID)(SettingsID::KeyBinding_OpenROM + i);
        buttons[i]->setText(g_Settings.GetDefaultStringValue(id));
    }
}

void SettingsDialog::loadDefaultBehaviorSettings(void)
{
    bool pause = false, resume = false;

    /*
        pause = g_Settings.GetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss);
        resume = g_Settings.GetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus);
    */

    this->manualResizingCheckBox->setChecked(g_Settings.GetDefaultBoolValue(SettingsID::GUI_AllowManualResizing));
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (inGame)
    {
        // clean 'game settings'
        QString section = this->gameInfo.Settings.MD5;
        g_MupenApi.Config.DeleteSection(section);
        this->saveGameSettings();
        this->saveGameCoreSettings();
        this->saveGamePluginSettings();
    }
    this->savePluginSettings();
    this->saveDirectorySettings();
    this->saveKeybindSettings();
    this->saveBehaviorSettings();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySize->currentIndex() == 0);
    int counterFactor = this->coreCounterFactor->currentIndex() + 1;
    int cpuEmulator = this->coreCpuEmulator->currentIndex();
    int siDmaDuration = this->coreSiDmaDuration->value();
    bool randomizeInterrupt = this->coreRandomizeTiming->isChecked();
    bool debugger = this->coreDebugger->isChecked();
    bool overrideGameSettings = this->coreOverrideGameSettingsGroup->isChecked();

    g_Settings.SetValue(SettingsID::Core_CPU_Emulator, cpuEmulator);
    g_Settings.SetValue(SettingsID::Core_RandomizeInterrupt, randomizeInterrupt);
    g_Settings.SetValue(SettingsID::Core_EnableDebugger, debugger);
    g_Settings.SetValue(SettingsID::Core_OverrideGameSpecificSettings, overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 0;
        siDmaDuration = -1;
    }

    g_Settings.SetValue(SettingsID::Core_DisableExtraMem, disableExtraMem);
    g_Settings.SetValue(SettingsID::Core_CountPerOp, counterFactor);
    g_Settings.SetValue(SettingsID::Core_SiDmaDuration, siDmaDuration);
}

void SettingsDialog::saveGameSettings(void)
{
    QString section = QString(this->gameInfo.Settings.MD5);

    bool disableExtraMem = this->gameMemorySize->currentIndex() == 0;
    int saveType = this->gameSaveType->currentIndex();
    int countPerOp = this->gameCounterFactor->currentIndex() + 1;
    int siDmaDuration = this->gameSiDmaDuration->value();

    if (this->defaultGameInfo.Settings.disableextramem != (unsigned char)disableExtraMem)
        g_Settings.SetValue(SettingsID::Game_DisableExtraMem, section, disableExtraMem);
    if (this->defaultGameInfo.Settings.savetype != saveType)
        g_Settings.SetValue(SettingsID::Game_SaveType, section, saveType);
    if (this->defaultGameInfo.Settings.countperop != countPerOp)
        g_Settings.SetValue(SettingsID::Game_CountPerOp, section, countPerOp);
    if (this->defaultGameInfo.Settings.sidmaduration != siDmaDuration)
        g_Settings.SetValue(SettingsID::Game_SiDmaDuration, section, siDmaDuration);
}

void SettingsDialog::saveGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    bool defaultOverrideEnabled, defaultRandomizeInterrupt;
    int cpuEmulator = 0, defaultCpuEmulator;

    QString section = QString(this->gameInfo.Settings.MD5);

    overrideEnabled = gameOverrideCoreSettingsGroupBox->isChecked();
    cpuEmulator = gameCoreCpuEmulatorComboBox->currentIndex();
    randomizeInterrupt = gameRandomizeTimingCheckBox->isChecked();

    defaultOverrideEnabled = g_Settings.GetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    defaultRandomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);
    defaultCpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Game_CPU_Emulator);

    if (defaultOverrideEnabled != overrideEnabled)
        g_Settings.SetValue(SettingsID::Game_OverrideCoreSettings, section, overrideEnabled);
    if (defaultCpuEmulator != cpuEmulator)
        g_Settings.SetValue(SettingsID::Game_CPU_Emulator, section, cpuEmulator);
    if (defaultRandomizeInterrupt != randomizeInterrupt)
        g_Settings.SetValue(SettingsID::Game_RandomizeInterrupt, section, randomizeInterrupt);
}

void SettingsDialog::saveGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->pluginVideoPlugins, this->pluginAudioPlugins, this->pluginInputPlugins,
                                   this->pluginRspPlugins};
    SettingsID settingsIdArray[4] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin, SettingsID::Game_INPUT_Plugin,
                        SettingsID::Game_RSP_Plugin};
    QComboBox *comboBox;
    SettingsID id;
    QString section = this->gameInfo.Settings.MD5;

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        id = settingsIdArray[i];

        if (comboBox->currentIndex() != 0)
            g_Settings.SetValue(id, section, comboBox->currentData().toString());
    }
}

void SettingsDialog::savePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    QComboBox *comboBox;

    for (const Plugin_t &p : g_Plugins.GetAvailablePlugins())
    {
        comboBox = comboBoxArray[(int)p.Type];

        if (p.FileName == comboBox->currentData().toString())
            g_Plugins.ChangePlugin(p);
    }
}

void SettingsDialog::saveDirectorySettings(void)
{
    g_Settings.SetValue(SettingsID::Core_ScreenshotPath, this->screenshotDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SaveStatePath, this->saveStateDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SaveSRAMPath, this->saveSramDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SharedDataPath, this->sharedDataDirLineEdit->text());

    g_Settings.SetValue(SettingsID::Core_OverrideUserDirs, this->overrideUserDirsGroupBox->isChecked());
    g_Settings.SetValue(SettingsID::Core_UserDataDirOverride, this->userDataDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_UserCacheDirOverride, this->userCacheDirLineEdit->text());
}

void SettingsDialog::saveKeybindSettings(void)
{
    KeyBindButton *buttons[] = {this->openRomKeyButton,   this->openComboKeyButton,      this->startEmuKeyButton,
                                this->endEmuKeyButton,    this->refreshRomListKeyButton, this->exitKeyButton,

                                this->softResetKeyButton, this->hardResetKeyButton,      this->generateBitmapKeyButton,
                                this->limitFPSKeyButton,  this->swapDiskKeyButton,       this->saveStateKeyButton,
                                this->saveAsKeyButton,    this->loadStateKeyButton,      this->loadKeyButton,
                                this->cheatsKeyButton,    this->gsButtonKeyButton,       this->fullscreenKeyButton,
                                this->settingsKeyButton};

    SettingsID id;
    for (int i = 0; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
    {
        id = (SettingsID)(SettingsID::KeyBinding_OpenROM + i);
        g_Settings.SetValue(id, buttons[i]->text());
    }
}

void SettingsDialog::saveBehaviorSettings(void)
{
    bool pause = false, resume = false;

    g_Settings.SetValue(SettingsID::GUI_AllowManualResizing, this->manualResizingCheckBox->isChecked());
    // this->manualResizingCheckBox->setChecked(g_Settings.GetBoolValue(SettingsID::GUI_AllowManualResizing));
    /* TODO for someday
        g_Settings.SetValue(SettingsID::GUI_PauseEmulationOnFocusLoss, pause);
        g_Settings.SetValue(SettingsID::GUI_ResumeEmulationOnFocus, resume);
    */
}

void SettingsDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout, this->emulationInfoLayout_2, this->emulationInfoLayout_8};

    for (const QHBoxLayout *layout : layouts)
    {
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget *widget = layout->itemAt(i)->widget();
            widget->hide();
        }
    }
}

void SettingsDialog::chooseDirectory(QLineEdit *lineEdit)
{
    QFileDialog dialog;
    int ret;

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);

    ret = dialog.exec();
    if (!ret)
        return;

    lineEdit->setText(dialog.directory().path());
}

void SettingsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    int topLevelCount = this->treeWidget->topLevelItemCount();
    int index = this->treeWidget->indexOfTopLevelItem(current);

    // count all children up until our item
    for (int i = 0; i < topLevelCount && i < index; i++)
    {
        index += this->treeWidget->topLevelItem(i)->childCount();
    }

    // if we're a child ourselves,
    // get index of parent and add that onto our own index
    if (index == -1)
    {
        index = this->treeWidget->indexOfTopLevelItem(current->parent());
        index += current->parent()->indexOfChild(current) + 1;
    }

    this->stackedWidget->setCurrentIndex(index);
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *resetButton = this->buttonBox->button(QDialogButtonBox::Reset);
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *saveButton = this->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelButton = this->buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    if (pushButton == resetButton)
    {
        this->reloadSettings(this->stackedWidget->currentIndex());
    }
    else if (pushButton == defaultButton)
    {
        this->restoreDefaults(this->stackedWidget->currentIndex());
    }
    else if (pushButton == saveButton)
    {
        this->saveSettings();
    }
    else if (pushButton == cancelButton)
    {
        this->reject();
    }
    else if (pushButton == okButton)
    {
        this->saveSettings();
        this->accept();
    }
}

void SettingsDialog::on_changeScreenShotDirButton_clicked(void)
{
    this->chooseDirectory(this->screenshotDirLineEdit);
}

void SettingsDialog::on_changeSaveStateDirButton_clicked(void)
{
    this->chooseDirectory(this->saveStateDirLineEdit);
}

void SettingsDialog::on_changeSaveSramDirButton_clicked(void)
{
    this->chooseDirectory(this->saveSramDirLineEdit);
}

void SettingsDialog::on_changeSharedDataDirButton_clicked(void)
{
    this->chooseDirectory(this->sharedDataDirLineEdit);
}

void SettingsDialog::on_changeUserDataDirButton_clicked(void)
{
    this->chooseDirectory(this->userDataDirLineEdit);
}

void SettingsDialog::on_changeUserCacheDirButton_clicked(void)
{
    this->chooseDirectory(this->userCacheDirLineEdit);
}
