#include "dialog.h"
#include "toggleSwitch.h"
#include "warningLabel.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

static QWidget* createWidget(QWidget* parent, Setting& setting)
{
	auto* widget = new QWidget(parent);
	auto* widgetVLayout = new QVBoxLayout(widget);

	auto* widgetHLayout = new QHBoxLayout();
	widgetVLayout->addLayout(widgetHLayout);

	auto* descrLabel = new QLabel(widget);
	descrLabel->setText(QLatin1String(setting.name));

	auto* toggle = new ToggleSwitch(widget);
	toggle->setChecked(setting.value);
	toggle->setAccessibleName(descrLabel->text());

	QObject::connect(toggle, &QAbstractButton::toggled, [&setting](bool checked)
	{
		setting.value = checked;
	});

	widgetHLayout->addWidget(descrLabel);
	widgetHLayout->addStretch();
	widgetHLayout->addSpacing(20);
	widgetHLayout->addWidget(toggle);

	if (std::string_view(setting.name) == "ExperimentalMode")
	{
		auto* warningLabel = new WarningLabel(u"Warning: Undo/Redo commands are NOT available in Experimental Mode!"_s, widget);
		warningLabel->setVisible(setting.value);

		QObject::connect(toggle, &QAbstractButton::toggled, warningLabel, &QWidget::setVisible);

		widgetVLayout->addWidget(warningLabel);
	}

	return widget;
}

UIDialog::UIDialog(QWidget *parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
}

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
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);

	for (auto& setting : m_settings)
	{
		QWidget* w = createWidget(this, setting);

		mainLayout->addWidget(w);
		mainLayout->addSpacing(10);
		w->show();
	}

	mainLayout->addStretch();

	auto* buttonLayout = new QHBoxLayout();
	mainLayout->addLayout(buttonLayout);

	QPushButton* okayButton = new QPushButton(this);
	okayButton->setDefault(true);
	okayButton->setText(u"Okay"_s);
	okayButton->setFocus(Qt::ActiveWindowFocusReason);

	QObject::connect(okayButton, &QPushButton::clicked, this, &UIDialog::storeSettings);
	QObject::connect(okayButton, &QPushButton::clicked, this, &QDialog::done);

	buttonLayout->addWidget(okayButton);

	QPushButton* resetButton = new QPushButton(this);
	resetButton->setDefault(true);
	resetButton->setText(u"Reset to Default"_s);

	QObject::connect(resetButton, &QPushButton::clicked, this, &UIDialog::resetSettings);
	QObject::connect(resetButton, &QPushButton::clicked, this, &QDialog::done);

	buttonLayout->addWidget(resetButton);
}


void showDialog()
{
	UIDialog* dialog = new UIDialog();
	dialog->loadSettings();
	dialog->initializeWidgets();
	dialog->exec();
}
