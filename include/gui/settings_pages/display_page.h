/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2024 frostbite2000
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DISPLAY_SETTINGS_PAGE_H
#define DISPLAY_SETTINGS_PAGE_H

#include "gui/settings_dialog.h"

/**
 * @brief The Display settings page for configuring video cards and display options
 */
class DisplaySettingsPage : public SettingsPage {
    Q_OBJECT
public:
    explicit DisplaySettingsPage(Emulator* emulator, QWidget* parent = nullptr);
    ~DisplaySettingsPage() override;

    void apply() override;
    void load() override;
    void reset() override;
    bool hasChanges() const override;

private slots:
    void onVideoCardChanged(int index);
    void onVideoConfigureClicked();
    void onVoodooModelChanged(int index);

private:
    void setupUi();
    void populateVideoCards();
    void populateVoodooCards();
    void populateRenderers();
    void populateWindowScales();
    void updateCardDependentControls();
    
    Emulator* m_emulator;
    
    // Original values for change detection
    QString m_origVideoCard;
    QString m_origVideoRam;
    QString m_origVoodooCard;
    QString m_origRenderer;
    QString m_origWindowScale;
    bool m_origVoodooEnabled;
    bool m_origVSync;
    bool m_origFullScreen;
    bool m_origHwCursor;
    
    // UI Controls
    QComboBox* m_videoCardCombo;
    QComboBox* m_videoRamCombo;
    QPushButton* m_videoConfigureButton;
    
    QGroupBox* m_voodooGroup;
    QCheckBox* m_voodooCheck;
    QComboBox* m_voodooModelCombo;
    
    QComboBox* m_rendererCombo;
    QComboBox* m_windowScaleCombo;
    QCheckBox* m_vsyncCheck;
    QCheckBox* m_fullScreenCheck;
    QCheckBox* m_hwCursorCheck;
    
    // Card configuration data
    QHash<QString, QStringList> m_vramByCard;
};

#endif // DISPLAY_SETTINGS_PAGE_H