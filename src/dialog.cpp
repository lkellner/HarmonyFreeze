#include "dialog.h"
#include <QSlider>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

QWidget* createWidget(QWidget* parent, QString description, bool isEnabled)
{
	auto* widget = new QWidget(parent);
	auto* widgetVLayout = new QVBoxLayout(widget);

	auto* descrLabel = new QLabel(widget);
	descrLabel->setText(description);
	widgetVLayout->addWidget(descrLabel, Qt::AlignLeft);
	widgetVLayout->addSpacing(10);

	auto* widgetHLayout = new QHBoxLayout(widget);
	widgetVLayout->addLayout(widgetHLayout);

	auto* slider = new QSlider(Qt::Horizontal, widget);
	slider->setSingleStep(1);
	slider->setMinimum(0);
	slider->setMaximum(1);
	slider->setStyleSheet(QStringLiteral(
		"QSlider::groove:horizontal {"
		"	border-radius: 10px;"
		"	width: 40px;"
		"	height: 20px;"
		"	background: #333333;"
		"}"
		"QSlider::handle:horizontal {"
		"	background: #bbb;"
		"	border-radius: 10px;"
		"	width: 20px;"
		"	height: 20px;"
		"	margin: -1px -1px;"
		"}"
	));

	const int position = isEnabled ? 1 : 0;

	slider->setSliderPosition(position);
	slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	
	auto* onLabel = new QLabel(widget);
	onLabel->setText(QStringLiteral("ON"));
	onLabel->setEnabled(isEnabled);
	onLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


	auto* offLabel = new QLabel(widget);
	offLabel->setText(QStringLiteral("OFF"));
	offLabel->setEnabled(!isEnabled);
	offLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto toggleLabels =	[onLabel, offLabel](int value)
	{
		const bool isEnabled = value > 0;
		onLabel->setEnabled(isEnabled);
		offLabel->setEnabled(!isEnabled);
	};

	QObject::connect(slider, &QSlider::valueChanged, toggleLabels);

	widgetHLayout->addSpacing(10);
	widgetHLayout->addWidget(offLabel);
	widgetHLayout->addWidget(slider);
	widgetHLayout->addWidget(onLabel);
	widgetHLayout->addWidget(onLabel);
	widgetHLayout->addStretch();

	return widget;
}


void showDialog()
{
	QDialog* dialog = new QDialog();

	QGridLayout* mainLayout = new QGridLayout(dialog);


	QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	auto loadSetting = [&settings, mainLayout, dialog, i = 0](const QString& name, bool defaultValue) mutable
	{
		// ensure it gets written to back to the file
		if (!settings.contains(name))
			settings.setValue(name, defaultValue);

		QWidget *w = createWidget(dialog, name, settings.value(name).toBool());

		mainLayout->addWidget(w, i++, 0);
		w->show();
	};

	loadSetting(QStringLiteral("2dMode"), true);
	loadSetting(QStringLiteral("ExperimentalMode"), false);
	loadSetting(QStringLiteral("PassOnOglControllerTransformation"), true);
	loadSetting(QStringLiteral("UseMultithreading"), false);
	loadSetting(QStringLiteral("MoveUnusedPivots"), false);
	loadSetting(QStringLiteral("DebugMode"), false);
	loadSetting(QStringLiteral("SetInbetweenKeyframesMode"), false);

	QPushButton* button = new QPushButton(dialog);
	button->setDefault(true);
	mainLayout->addWidget(button, 7, 0);

	mainLayout->setVerticalSpacing(30);

	dialog->exec();
}
