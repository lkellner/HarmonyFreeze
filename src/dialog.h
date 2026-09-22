/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

constexpr int nSettings = 7;

struct Setting
{
	const char* name;
	bool value;
};

class UIDialog : public QDialog
{
public:
	UIDialog() = default;

	void loadSettings();
	void storeSettings();
	void initializeWidgets();

private:
	std::array<Setting, nSettings> m_settings;
};

void showDialog();
QWidget* createWidget(QWidget* parent, QString description, bool* isEnabled);

#endif
