/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>


void showDialog();
QWidget* createWidget(QWidget* parent, QString description, bool isEnabled);
void initWidgets(std::vector<QWidget*>& widgets, QWidget* parent);

#endif
