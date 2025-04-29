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

#include "gui/about_dialog.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::setupUi()
{
    // Set window properties
    setWindowTitle("About x86Emulator");
    setFixedSize(400, 300);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Logo
    m_logoLabel = new QLabel(this);
    m_logoLabel->setPixmap(QPixmap(":/icons/app_icon.ico").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_logoLabel);
    
    // Title
    m_titleLabel = new QLabel("x86Emulator", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);
    
    // Version
    m_versionLabel = new QLabel("Version 1.0.0", this);
    m_versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_versionLabel);
    
    // Spacer
    mainLayout->addSpacing(10);
    
    // Copyright
    m_copyrightLabel = new QLabel("Copyright © 2024 frostbite2000", this);
    m_copyrightLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_copyrightLabel);
    
    // License
    m_licenseLabel = new QLabel(
        "This program is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or "
        "(at your option) any later version.", 
        this);
    m_licenseLabel->setAlignment(Qt::AlignCenter);
    m_licenseLabel->setWordWrap(true);
    mainLayout->addWidget(m_licenseLabel);
    
    // Spacer
    mainLayout->addSpacing(10);
    
    // Close button
    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &AboutDialog::accept);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Add some stretch at the bottom
    mainLayout->addStretch();
}