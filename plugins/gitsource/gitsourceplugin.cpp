#include "gitsourceplugin.h"
#include <QUrl>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QSet>
#include <QTimer>

QRegularExpression GitSourcePlugin::_githubRegex(QStringLiteral(R"__(^com\.github\.([^\.]*)\.([^\.]*)$)__"));

GitSourcePlugin::GitSourcePlugin(QObject *parent) :
	QObject(parent),
	SourcePlugin(),
	_processCache()
{}

QString GitSourcePlugin::packageSyntax(const QString &provider) const
{
	if(provider == QStringLiteral("git"))
		return tr("<url>[#<prefix>]");
	else if(provider == QStringLiteral("github"))
		return tr("com.github.<user>.<repository>");
	else
		return {};
}

bool GitSourcePlugin::packageValid(const qpmx::PackageInfo &package) const
{
	if(package.provider() == QStringLiteral("git")) {
		QUrl url(package.package());
		return url.isValid() && url.path().endsWith(QStringLiteral(".git"));
	} else if(package.provider() == QStringLiteral("github"))
		return _githubRegex.match(package.package().toLower()).hasMatch();
	else
		return false;
}

void GitSourcePlugin::searchPackage(int requestId, const QString &provider, const QString &query)
{
	Q_UNUSED(query);
	Q_UNUSED(provider);
	//git does not support searching
	emit searchResult(requestId, {});
}

void GitSourcePlugin::listPackageVersions(int requestId, const qpmx::PackageInfo &package)
{
	try {
		auto url = pkgUrl(package);
		auto logDir = createLogDir(QStringLiteral("ls-remote"));

		auto proc = new QProcess(this);
		proc->setProgram(QStandardPaths::findExecutable(QStringLiteral("git")));
		proc->setStandardErrorFile(logDir.absoluteFilePath(QStringLiteral("stderr.log")));
		proc->setProperty("logDir", logDir.absolutePath());

		QStringList arguments{
								  QStringLiteral("ls-remote"),
								  QStringLiteral("--tags"),
								  QStringLiteral("--exit-code"),
								  url
							  };
		if(!package.version().isNull())
			arguments.append(package.version().toString());
		proc->setArguments(arguments);

		connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this, &GitSourcePlugin::finished);
		connect(proc, &QProcess::errorOccurred,
				this, &GitSourcePlugin::errorOccurred);

		_processCache.insert(proc, {requestId, false});
		proc->start();

		//timeout after 30 seconds
		QTimer::singleShot(30000, this, [proc, this](){
			if(_processCache.contains(proc)) {
				proc->kill();
				if(!proc->waitForFinished(1000))
					proc->terminate();
			}
		});
	} catch(QString &s) {
		emit sourceError(requestId, s);
	}
}

void GitSourcePlugin::getPackageSource(int requestId, const qpmx::PackageInfo &package, const QDir &targetDir, const QVariantHash &extraParameters)
{
	try {
		auto url = pkgUrl(package);
		auto tag = pkgTag(package);
		auto logDir = createLogDir(QStringLiteral("clone"));

		auto proc = new QProcess(this);
		proc->setProgram(QStandardPaths::findExecutable(QStringLiteral("git")));
		proc->setStandardOutputFile(logDir.absoluteFilePath(QStringLiteral("stdout.log")));
		proc->setStandardErrorFile(logDir.absoluteFilePath(QStringLiteral("stderr.log")));
		proc->setProperty("logDir", logDir.absolutePath());

		QStringList arguments{
								  QStringLiteral("clone"),
								  url,
								  QStringLiteral("--recurse-submodules"),
								  QStringLiteral("--branch"),
								  tag,
								  targetDir.absolutePath()
							  };
		if(extraParameters.contains(QStringLiteral("gitargs")))
			arguments.append(extraParameters.value(QStringLiteral("gitargs")).toStringList());
		proc->setArguments(arguments);

		connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this, &GitSourcePlugin::finished);
		connect(proc, &QProcess::errorOccurred,
				this, &GitSourcePlugin::errorOccurred);

		_processCache.insert(proc, {requestId, true});
		proc->start();

		//timeout after 30 seconds
		QTimer::singleShot(30000, this, [proc, this](){
			if(_processCache.contains(proc)) {
				proc->kill();
				if(!proc->waitForFinished(1000))
					proc->terminate();
			}
		});
	} catch(QString &s) {
		emit sourceError(requestId, s);
	}
}

void GitSourcePlugin::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if(exitStatus == QProcess::CrashExit)
		errorOccurred(QProcess::Crashed);
	else {
		auto proc = qobject_cast<QProcess*>(sender());
		auto data = _processCache.value(proc, {-1, false});
		if(data.first != -1) {
			_processCache.remove(proc);
			if(data.second) {
				if(exitCode == EXIT_SUCCESS)
					emit sourceFetched(data.first);
				else {
					emit sourceError(data.first,
									 tr("Failed to clone source with exit code %1. "
										"Check the logs at \"%2\" for more details")
									 .arg(exitCode)
									 .arg(proc->property("logDir").toString()));
				}
			} else {
				if(exitCode == EXIT_SUCCESS) {
					//parse output
					QSet<QVersionNumber> versions;
					QRegularExpression tagRegex(QStringLiteral(R"__(^\w+\trefs\/tags\/(.*)$)__"));
					foreach(auto line, proc->readAllStandardOutput().split('\n')) {
						auto match = tagRegex.match(QString::fromUtf8(line));
						if(match.hasMatch())
							versions.insert(QVersionNumber::fromString(match.captured(1)));
					}
					emit versionResult(data.first, versions.toList());
				} else {
					if(exitCode == 2) {
						emit versionResult(data.first, {});
					} else {
						emit sourceError(data.first,
										 tr("Failed to list versions with exit code %1. "
											"Check the logs at \"%2\" for more details")
										 .arg(exitCode)
										 .arg(proc->property("logDir").toString()));
					}
				}
			}
		}
		proc->deleteLater();
	}
}

void GitSourcePlugin::errorOccurred(QProcess::ProcessError error)
{
	Q_UNUSED(error)
	auto proc = qobject_cast<QProcess*>(sender());
	auto data = _processCache.value(proc, {-1, false});
	if(data.first != -1) {
		_processCache.remove(proc);
		if(data.second) {
			emit sourceError(data.first,
							 tr("Failed to clone source with process error: %1")
							 .arg(proc->errorString()));
		} else {
			emit sourceError(data.first,
							 tr("Failed to list versions with process error: %1")
							 .arg(proc->errorString()));
		}
	}
	proc->deleteLater();
}

QString GitSourcePlugin::pkgUrl(const qpmx::PackageInfo &package)
{
	QString pkgUrl;
	if(package.provider() == QStringLiteral("git"))
		pkgUrl = package.package();
	else if(package.provider() == QStringLiteral("github")) {
		auto match = _githubRegex.match(package.package().toLower());
		if(!match.hasMatch())
			throw tr("Package \"%1\" is not a valid github package").arg(package.toString());
		pkgUrl = QStringLiteral("https://github.com/%1/%2.git")
				 .arg(match.captured(1))
				 .arg(match.captured(2));
	} else
		throw tr("Unknown provider type \"%1\"").arg(package.provider());

	return pkgUrl;
}

QString GitSourcePlugin::pkgTag(const qpmx::PackageInfo &package)
{
	QUrl packageUrl(package.package());
	if(!packageUrl.isValid())
		throw tr("The given package name is not a valid url");

	QString tag;
	if(packageUrl.hasFragment()) {
		auto prefix = packageUrl.fragment();
		if(prefix.contains(QStringLiteral("%1")))
			tag = prefix.arg(package.version().toString());
		else
			tag = prefix + package.version().toString();
	} else
		tag = package.version().toString();
	return tag;
}

QDir GitSourcePlugin::createLogDir(const QString &action)
{
	auto subPath = QStringLiteral("qpmx.logs/%1").arg(action);
	QDir pDir(QDir::temp());
	pDir.mkpath(subPath);
	if(!pDir.cd(subPath))
		throw tr("Failed to create log directory \"%1\"").arg(pDir.absolutePath());

	QTemporaryDir tDir(pDir.absoluteFilePath(QStringLiteral("XXXXXX")));
	tDir.setAutoRemove(false);
	if(tDir.isValid())
		return tDir.path();
	else
		throw tr("Failed to create log directory \"%1\"").arg(tDir.path());
}
