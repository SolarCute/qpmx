#ifndef TRANSLATECOMMAND_H
#define TRANSLATECOMMAND_H

#include "command.h"

#include <QProcess>
#include <functional>

class TranslateCommand : public Command
{
	Q_OBJECT

public:
	explicit TranslateCommand(QObject *parent = nullptr);

public slots:
	void initialize(QCliParser &parser) override;

private:
	QString _outDir;
	QString _qmake;
	QString _lconvert;
	QpmxFormat _format;
	QString _tsFile;
	QStringList _lrelease;
	QStringList _qpmxTsFiles;

	void binTranslate();
	void srcTranslate();

	void execute(QStringList command);
	QString localeString();
};

#endif // TRANSLATECOMMAND_H
