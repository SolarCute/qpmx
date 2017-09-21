#include "translatecommand.h"

#include <iostream>

TranslateCommand::TranslateCommand(QObject *parent) :
	Command(parent),
	_outDir(),
	_qmake(),
	_lconvert(),
	_format(),
	_tsFile(),
	_lrelease(),
	_qpmxTsFiles()
{}

void TranslateCommand::initialize(QCliParser &parser)
{
	try {
		if(parser.isSet(QStringLiteral("outdir")))
			_outDir = parser.value(QStringLiteral("outdir")) + QLatin1Char('/');
		else
			_outDir = QStringLiteral("./");
		_qmake = parser.value(QStringLiteral("qmake"));
		_lconvert = parser.value(QStringLiteral("lconvert"));
		QFileInfo qpmxFile = parser.value(QStringLiteral("qpmx"));
		_format = QpmxFormat::readFile(qpmxFile.dir(), qpmxFile.fileName(), true);
		_tsFile = parser.value(QStringLiteral("ts-file"));
		if(!QFile::exists(_tsFile))
			throw tr("You must specify the --ts-file option with a valid file!");

		auto pArgs = parser.positionalArguments();
		if(pArgs.isEmpty())
			throw tr("You must specify the path to the lrelease binary after the qpmx file as argument, with possible additional arguments");
		do {
			QString arg = pArgs.takeFirst();
			if(arg == QStringLiteral("%%")) {
				_qpmxTsFiles = pArgs;
				xDebug() << tr("Extracted translation files as: %1").arg(_qpmxTsFiles.join(tr(", ")));
				break;
			}
			_lrelease.append(arg.replace(QRegularExpression(QStringLiteral("^\\+")), QStringLiteral("-")));
		} while(!pArgs.isEmpty());
		xDebug() << tr("Extracted lrelease as: %1").arg(_lrelease.join(QStringLiteral(" ")));

		if(_format.source)
			srcTranslate();
		else
			binTranslate();

		qApp->quit();
	} catch(QString &s) {
		xCritical() << s;
	}
}

void TranslateCommand::binTranslate()
{
	if(!QFile::exists(_qmake))
		throw tr("Choosen qmake executable \"%1\" does not exist").arg(_qmake);

	//first: translate all the ts file
	QFileInfo tsInfo(_tsFile);
	QString qmFile = _outDir + tsInfo.completeBaseName() + QStringLiteral(".qm");
	QString qmBaseFile = _outDir + tsInfo.completeBaseName() + QStringLiteral(".qm-base");

	auto args = _lrelease;
	args.append({_tsFile, QStringLiteral("-qm"), qmBaseFile});
	execute(args);

	//now combine them into one
	auto locale = localeString();
	if(locale.isNull()) {
		QFile::rename(qmBaseFile, qmFile);
		return;
	}

	args = QStringList{
		_lconvert,
		QStringLiteral("-if"), QStringLiteral("qm"),
		QStringLiteral("-i"), qmBaseFile
	};

	foreach(auto dep, _format.dependencies) {
		auto bDir = buildDir(findKit(_qmake), dep);
		if(!bDir.cd(QStringLiteral("translations")))
			continue;

		bDir.setFilter(QDir::Files | QDir::Readable);
		bDir.setNameFilters({QStringLiteral("*.qm")});
		foreach(auto qpmxQmFile, bDir.entryInfoList()) {
			auto baseName = qpmxQmFile.completeBaseName();
			if(baseName.endsWith(locale))
				args.append({QStringLiteral("-i"), qpmxQmFile.absoluteFilePath()});
		}
	}

	args.append({QStringLiteral("-of"), QStringLiteral("qm")});
	args.append({QStringLiteral("-o"), qmFile});
	execute(args);
}

void TranslateCommand::srcTranslate()
{
	QFileInfo tsInfo(_tsFile);
	QString qmFile = _outDir + tsInfo.completeBaseName() + QStringLiteral(".qm");

	QStringList tsFiles(_tsFile);
	auto locale = localeString();
	if(!locale.isNull()) {
		//collect all possible qpmx ts files
		foreach(auto ts, _qpmxTsFiles) {
			auto baseName = QFileInfo(ts).completeBaseName();
			if(baseName.endsWith(locale))
				tsFiles.append(ts);
		}
	}

	auto args = _lrelease;
	args.append(tsFiles);
	args.append({QStringLiteral("-qm"), qmFile});
	execute(args);
}

void TranslateCommand::execute(QStringList command)
{
	xDebug() << tr("Running subcommand: %1").arg(command.join(QLatin1Char(' ')));

	auto pName = command.takeFirst();
	auto res = QProcess::execute(pName, command);
	switch (res) {
	case -2://not started
		throw tr("Failed to start \"%1\" to compile \"%2\"")
				.arg(pName)
				.arg(_tsFile);
		break;
	case -1://crashed
		throw tr("Failed to run \"%1\" to compile \"%2\" - it crashed")
				.arg(pName)
				.arg(_tsFile);
		break;
	case 0://success
		xDebug() << tr("Successfully ran \"%1\" to compile \"%2\"")
					.arg(pName)
					.arg(_tsFile);
		break;
	default:
		throw tr("Running \"%1\" to compile \"%2\" failed with exit code: %3")
				.arg(pName)
				.arg(_tsFile)
				.arg(res);//TODO exit with the given code instead
		break;
	}
}

QString TranslateCommand::localeString()
{
	auto parts = QFileInfo(_tsFile).baseName().split(QLatin1Char('_'));
	do {
		auto nName = parts.join(QLatin1Char('_'));
		QLocale locale(nName);
		if(locale != QLocale::c())
			return QLatin1Char('_') + nName;
		parts.removeFirst();
	} while(!parts.isEmpty());

	xWarning() << tr("Unable to detect locale of file \"%1\". Translation combination is skipped")
				  .arg(_tsFile);
	return {};
}