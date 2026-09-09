#include "dialog.h"
#include <QSlider>
void showDialog()
{
	

	QDialog dialogue;

	QSlider slider01 = QSlider(Qt::Horizontal, &dialogue);
	slider01.setSingleStep(1);
	slider01.setMinimum(0);
	slider01.setMaximum(1);
	slider01.setStyleSheet(QStringLiteral(
		"QSlider::groove:horizontal {"
		"	width: 30px;"
		"	height: 20px;"
		"	background: #ccc;"
		"}"
		"QSlider::handle:horizontal {"
		"    background: #007ACC;"
		"    width: 20px;"
		"    height: 20px;"
		"    margin: -7px 0;"
		"}"
	));
	slider01.show();
	dialogue.exec();
}
