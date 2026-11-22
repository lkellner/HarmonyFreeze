/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef WARNINGLABEL_H
#define WARNINGLABEL_H

#include <QLabel>

class WarningLabel : public QLabel
{
public:
	explicit WarningLabel(const QString& text, QWidget* parent = nullptr);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	void resizeEvent(QResizeEvent* event) override;
};

#endif
