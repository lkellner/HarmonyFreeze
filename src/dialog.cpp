#include "dialog.h"
#include <QSlider>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>

QWidget* createWidget(QWidget* parent, QString description, bool isEnabled)
{
	QWidget* widget = new QWidget(parent);
	QGridLayout* widgetLayout = new QGridLayout(widget);

	QLabel* descrLabel = new QLabel(widget);
	descrLabel->setText(description);
	widgetLayout->addWidget(descrLabel, 0, 0, 1, 4, Qt::AlignLeft);


	QSlider* slider = new QSlider(Qt::Horizontal, widget);
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

	if (isEnabled)
		slider->setSliderPosition(1);
	else
		slider->setSliderPosition(0);
	slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	widgetLayout->addWidget(slider, 1, 1);
	
	QLabel* onLabel = new QLabel(widget);
	onLabel->setText(QStringLiteral("ON"));
	onLabel->setEnabled(isEnabled);
	onLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	widgetLayout->addWidget(onLabel, 1, 2);


	QLabel* offLabel = new QLabel(widget);
	offLabel->setText(QStringLiteral("OFF"));
	offLabel->setEnabled(!isEnabled);
	offLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	widgetLayout->addWidget(offLabel, 1, 0, Qt::AlignLeft);

	auto toggleLabels =	[onLabel, offLabel](int value)
	{
		const bool isEnabled = value > 0;
		onLabel->setEnabled(isEnabled);
		offLabel->setEnabled(!isEnabled);
	};

	QObject::connect(slider, &QSlider::valueChanged, toggleLabels);
	
	//widgetLayout->setColumnStretch(1, 0);
	//widgetLayout->setColumnStretch(0, 0);
	//widgetLayout->setRowStretch(0, 0);
	//widgetLayout->setRowStretch(1, 0);

	widgetLayout->setVerticalSpacing(20);

	return widget;
}

void initWidgets(std::vector<QWidget*>& widgets, QWidget* parent)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	if (!settings.contains(QStringLiteral("2dMode")))
		settings.setValue(QStringLiteral("2dMode"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("2dMode"), settings.value(QStringLiteral("2dMode")).toBool()));


	if (!settings.contains(QStringLiteral("ExperimentalMode")))
		settings.setValue(QStringLiteral("ExperimentalMode"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("ExperimentalMode"), settings.value(QStringLiteral("ExperimentalMode")).toBool()));


	if (!settings.contains(QStringLiteral("PassOnOglControllerTransformation")))
		settings.setValue(QStringLiteral("PassOnOglControllerTransformation"), true);

	widgets.emplace_back(createWidget(parent, QStringLiteral("PassOnOglControllerTransformation"), settings.value(QStringLiteral("PassOnOglControllerTransformation")).toBool()));


	if (!settings.contains(QStringLiteral("UseMultithreading")))
		settings.setValue(QStringLiteral("UseMultithreading"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("UseMultithreading"), settings.value(QStringLiteral("UseMultithreading")).toBool()));


	if (!settings.contains(QStringLiteral("MoveUnusedPivots")))
		settings.setValue(QStringLiteral("MoveUnusedPivots"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("MoveUnusedPivots"), settings.value(QStringLiteral("MoveUnusedPivots")).toBool()));


	if (!settings.contains(QStringLiteral("DebugMode")))
		settings.setValue(QStringLiteral("DebugMode"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("DebugMode"), settings.value(QStringLiteral("DebugMode")).toBool()));

	if (!settings.contains(QStringLiteral("SetInbetweenKeyframesMode")))
		settings.setValue(QStringLiteral("SetInbetweenKeyframesMode"), false);

	widgets.emplace_back(createWidget(parent, QStringLiteral("SetInbetweenKeyframesMode"), settings.value(QStringLiteral("SetInbetweenKeyframesMode")).toBool()));
}


void showDialog()
{
	QDialog* dialog = new QDialog();

	QGridLayout* mainLayout = new QGridLayout(dialog);

	std::vector<QWidget*> widgets;
	initWidgets(widgets, dialog);

	for (int i = 0; i < widgets.size(); i++)
	{
		mainLayout->addWidget(widgets[i], i, 0);
		widgets[i]->show();
	}

	mainLayout->setVerticalSpacing(30);

	dialog->exec();
}
