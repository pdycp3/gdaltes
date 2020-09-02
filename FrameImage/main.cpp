#include "QFrameImage.h"
#include <QTranslator>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	QTranslator qtTranslator;
	bool bTrue=qtTranslator.load("frameimage_zh.qm");
	a.installTranslator(&qtTranslator);
	QFrameImage w;
	w.setWindowTitle(QObject::tr("Frame Image"));
	w.setWindowIcon(QIcon("rater_clip.ico"));
	w.show();
	return a.exec();
}
