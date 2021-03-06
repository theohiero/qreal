#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include "metaCompiler.h"

void myMessageOutput(QtMsgType type, const char *msg)
 {
	switch (type) {
		case QtDebugMsg:
			fprintf(stdout, "Debug: %s\n", msg);
			break;
		case QtWarningMsg:
			fprintf(stderr, "Warning: %s\n", msg);
			break;
		case QtCriticalMsg:
			fprintf(stderr, "Critical: %s\n", msg);
			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s\n", msg);
			abort();
	}
}

int main(int argc, char *argv[])
{
	qInstallMsgHandler(myMessageOutput);
	QCoreApplication app(argc, argv);

//	qDebug() << "Running " + args.join(" ");

//	if (args.count() != 2) {
//		qDebug() << "Usage: qrmc PATH_TO_WORKING_COPY";
//		return 1;
//	}

//	QString workingCopyDir = args[1];
	QString workingCopyDir = "../qrgui/save";

	MetaCompiler metaCompiler(workingCopyDir);
	if (!metaCompiler.compile()) {
		qDebug() << "compilation failed";
		return 1;
	}

	return 0;
}
