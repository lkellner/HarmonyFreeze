/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QAbstractButton>

class ToggleSwitch : public QAbstractButton
{
public:
	explicit ToggleSwitch(QWidget* parent = nullptr);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent* event) override;
};

#endif
