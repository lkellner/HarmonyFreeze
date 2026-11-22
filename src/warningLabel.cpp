/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#include "warningLabel.h"

using namespace Qt::StringLiterals;

WarningLabel::WarningLabel(const QString& text, QWidget* parent)
	: QLabel(parent)
{
	setWordWrap(true);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	setTextFormat(Qt::RichText);
	setText(u"<span style='color:#d29922'>&#x26A0;&#xFE0E;</span>&nbsp; "_s + text.toHtmlEscaped());
	setStyleSheet(QStringLiteral(
		"QLabel {"
		"	background-color: rgba(187, 128, 9, 38);"
		"	border: 1px solid rgba(187, 128, 9, 102);"
		"	border-radius: 6px;"
		"	padding: 8px 12px;"
		"	margin-top: 6px;"
		"}"
	));
}

QSize WarningLabel::sizeHint() const
{
	return { QLabel::sizeHint().width(), heightForWidth(width()) };
}

QSize WarningLabel::minimumSizeHint() const
{
	return { QLabel::minimumSizeHint().width(), heightForWidth(width()) };
}

void WarningLabel::resizeEvent(QResizeEvent* event)
{
	QLabel::resizeEvent(event);
	updateGeometry();
}
