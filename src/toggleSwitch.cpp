/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#include "toggleSwitch.h"

#include <QPainter>

ToggleSwitch::ToggleSwitch(QWidget* parent)
	: QAbstractButton(parent)
{
	setCheckable(true);
	setCursor(Qt::PointingHandCursor);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize ToggleSwitch::sizeHint() const
{
	return { 36, 20 };
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	painter.setOpacity(isEnabled() ? 1.0 : 0.5);

	constexpr qreal aspect = 1.8;
	const qreal trackHeight = qMin<qreal>(height(), width() / aspect);
	QRectF track(0, 0, trackHeight * aspect, trackHeight);
	track.moveCenter(QRectF(rect()).center());
	const qreal trackRadius = track.height() / 2;

	painter.setBrush(isChecked() ? QColor(0x52, 0xc3, 0xbd) : QColor(255, 255, 255, 40));
	painter.drawRoundedRect(track, trackRadius, trackRadius);

	const qreal inset = track.height() * 0.1;
	const qreal knobHeight = track.height() - 2 * inset;
	const qreal knobWidth = knobHeight * 1.4;
	const qreal knobRadius = knobHeight / 2;
	const qreal knobX = isChecked() ? track.right() - inset - knobWidth : track.left() + inset;
	const QRectF knob(knobX, track.top() + inset, knobWidth, knobHeight);

	painter.setBrush(QColor(0, 0, 0, 50));
	painter.drawRoundedRect(knob.translated(0, 0.5).adjusted(-0.5, -0.5, 0.5, 0.5), knobRadius + 0.5, knobRadius + 0.5);

	painter.setBrush(isChecked() ? QColor(255, 255, 255) : QColor(0xe5, 0xe5, 0xe5));
	painter.drawRoundedRect(knob, knobRadius, knobRadius);
}
