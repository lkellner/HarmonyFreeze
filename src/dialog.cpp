#include "dialog.h"
#include <QSlider>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>


void UIDialog::storeSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	for (const auto& setting : m_settings)
	{
		settings.setValue(QLatin1String(setting.name), setting.value);
	}
}


void UIDialog::loadSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	auto getSettingVal = [&settings](const QString& name, bool defaultValue) mutable
	{
		// ensure it gets written to back to the file
		if (!settings.contains(name))
			settings.setValue(name, defaultValue);

		return settings.value(name).toBool();
	};

	m_settings[0] = Setting{ "2dMode", getSettingVal(QStringLiteral("2dMode"), true) };
	m_settings[1] = Setting{ "ExperimentalMode", getSettingVal(QStringLiteral("ExperimentalMode"), true) };
	m_settings[2] = Setting{ "PassOnOglControllerTransformation", getSettingVal(QStringLiteral("PassOnOglControllerTransformation"), true) };
	m_settings[3] = Setting{ "UseMultithreading", getSettingVal(QStringLiteral("UseMultithreading"), true) };
	m_settings[4] = Setting{ "MoveUnusedPivots", getSettingVal(QStringLiteral("MoveUnusedPivots"), true) };
	m_settings[5] = Setting{ "DebugMode", getSettingVal(QStringLiteral("DebugMode"), true) };
	m_settings[6] = Setting{ "SetInbetweenKeyframesMode", getSettingVal(QStringLiteral("SetInbetweenKeyframesMode"), true) };
}


void UIDialog::initializeWidgets()
{
	auto* mainLayout = new QVBoxLayout(this);

	for (auto& setting : m_settings)
	{
		QWidget* w = createWidget(this, QLatin1String(setting.name), &setting.value);

		mainLayout->addWidget(w);
		mainLayout->addSpacing(10);
		w->show();
	}

	QPushButton* button = new QPushButton(this);
	button->setDefault(true);
	button->setText(QStringLiteral("Okay"));

	QObject::connect(button, &QPushButton::clicked, this, &UIDialog::storeSettings);
	QObject::connect(button, &QPushButton::clicked, this, &QDialog::done);

	mainLayout->addWidget(button);
}


QWidget* createWidget(QWidget* parent, QString description, bool* isEnabledSetting)
{
	auto* widget = new QWidget(parent);
	auto* widgetVLayout = new QVBoxLayout(widget);

	auto* descrLabel = new QLabel(widget);
	descrLabel->setText(description);
	widgetVLayout->addWidget(descrLabel, Qt::AlignLeft);
	widgetVLayout->addSpacing(10);

	auto* widgetHLayout = new QHBoxLayout();
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

	const int position = *isEnabledSetting ? 1 : 0;

	slider->setSliderPosition(position);
	slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto* onLabel = new QLabel(widget);
	onLabel->setText(QStringLiteral("ON"));
	onLabel->setEnabled(*isEnabledSetting);
	onLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto* offLabel = new QLabel(widget);
	offLabel->setText(QStringLiteral("OFF"));
	offLabel->setEnabled(!*isEnabledSetting);
	offLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto toggleLabels =	[onLabel, offLabel, isEnabledSetting](int value)
	{
		const bool isEnabled = value > 0;
		onLabel->setEnabled(isEnabled);
		offLabel->setEnabled(!isEnabled);
		*isEnabledSetting = isEnabled;
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
	UIDialog* dialog = new UIDialog();
	dialog->loadSettings();
	dialog->initializeWidgets();
	dialog->exec();
}
