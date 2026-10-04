/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef DIALOG_H
#define DIALOG_H

#include "settings.h"

#include <QDialog>

class UIDialog : public QDialog
{
public:
	UIDialog() = default;

	void loadSettings();
	void storeSettings();
	void resetSettings();
	void initializeWidgets();

private:
	SettingsDesc m_settings;
};

void showDialog();
QWidget* createWidget(QWidget* parent, Setting& setting);

#endif
