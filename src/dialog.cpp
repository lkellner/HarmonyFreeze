#include "dialog.h"


#include <QSlider>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>


void UIDialog::storeSettings()
{
	::storeSettings(m_settings);
}


void UIDialog::resetSettings()
{
	m_settings = g_defaultSettings;

	storeSettings();
}

void UIDialog::loadSettings()
{
	m_settings = ::loadSettings();
}

void UIDialog::initializeWidgets()
{
	auto* mainLayout = new QVBoxLayout(this);

	for (auto& setting : m_settings)
	{
		QWidget* w = createWidget(this, setting);

		mainLayout->addWidget(w);
		mainLayout->addSpacing(10);
		w->show();
	}

	auto* buttonLayout = new QHBoxLayout();
	mainLayout->addLayout(buttonLayout);

	QPushButton* okayButton = new QPushButton(this);
	okayButton->setDefault(true);
	okayButton->setText(QStringLiteral("Okay"));
	okayButton->setFocus(Qt::ActiveWindowFocusReason);

	QObject::connect(okayButton, &QPushButton::clicked, this, &UIDialog::storeSettings);
	QObject::connect(okayButton, &QPushButton::clicked, this, &QDialog::done);

	buttonLayout->addWidget(okayButton);

	QPushButton* resetButton = new QPushButton(this);
	resetButton->setDefault(true);
	resetButton->setText(QStringLiteral("Reset to Default"));

	QObject::connect(resetButton, &QPushButton::clicked, this, &UIDialog::resetSettings);
	QObject::connect(resetButton, &QPushButton::clicked, this, &QDialog::done);

	buttonLayout->addWidget(resetButton);
}


QWidget* createWidget(QWidget* parent, Setting& setting)
{
	auto* widget = new QWidget(parent);
	auto* widgetVLayout = new QVBoxLayout(widget);

	auto* descrLabel = new QLabel(widget);
	descrLabel->setText(QLatin1String(setting.name));
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

	const int position = setting.value ? 1 : 0;

	slider->setSliderPosition(position);
	slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto* onLabel = new QLabel(widget);
	onLabel->setText(QStringLiteral("ON"));
	onLabel->setEnabled(setting.value);
	onLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto* offLabel = new QLabel(widget);
	offLabel->setText(QStringLiteral("OFF"));
	offLabel->setEnabled(!setting.value);
	offLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	auto toggleLabels =	[onLabel, offLabel, &setting](int value)
	{
		const bool isEnabled = value > 0;
		onLabel->setEnabled(isEnabled);
		offLabel->setEnabled(!isEnabled);
		setting.value = isEnabled;
	};

	QObject::connect(slider, &QSlider::valueChanged, toggleLabels);

	widgetHLayout->addSpacing(10);
	widgetHLayout->addWidget(offLabel);
	widgetHLayout->addWidget(slider);
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
