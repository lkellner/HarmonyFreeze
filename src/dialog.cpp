#include "dialog.h"
#include <QSlider>
#include <QGridLayout>
#include <QLabel>

QWidget* createWidget(QWidget* parent)
{
	

	QWidget* widget = new QWidget(parent);
	QSlider* slider01 = new QSlider(Qt::Horizontal, widget);
	slider01->setSingleStep(1);
	slider01->setMinimum(0);
	slider01->setMaximum(1);
	slider01->setStyleSheet(QStringLiteral(
		"QSlider::groove:horizontal {"
		"	width: 30px;"
		"	height: 20px;"
		"	background: #ccc;"
		"}"
		"QSlider::handle:horizontal {"
		"	background: #007ACC;"
		"	width: 20px;"
		"	height: 20px;"
		"	margin: -7px 0;"
		"}"
	));
	QGridLayout* widgetLayout = new QGridLayout(widget);
	QLabel* descrLabel = new QLabel(widget);
	descrLabel->setText(QStringLiteral("This is a description"));

	QLabel* onLabel = new QLabel(widget);
	onLabel->setText(QStringLiteral("ON"));
	QLabel* offLabel = new QLabel(widget);
	offLabel->setText(QStringLiteral("OFF"));

	widgetLayout->addWidget(onLabel, 1, 0);
	widgetLayout->addWidget(offLabel, 1, 2, Qt::AlignLeft);
	widgetLayout->addWidget(slider01, 1, 1);
	widgetLayout->addWidget(descrLabel, 0, 0, 1, 3, Qt::AlignLeft);
	widgetLayout->setColumnStretch(1, 0);
	widgetLayout->setColumnStretch(0, 0);
	widgetLayout->setRowStretch(0, 0);
	widgetLayout->setRowStretch(1, 0);

	widgetLayout->setVerticalSpacing(20);

	return widget;
}

void showDialog()
{
	QDialog* dialogue = new QDialog();
	QWidget* widget = createWidget(dialogue);

	QGridLayout* mainLayout = new QGridLayout(dialogue);
	mainLayout->addWidget(widget, 0, 0);
	
	widget->show();
	dialogue->exec();
}
