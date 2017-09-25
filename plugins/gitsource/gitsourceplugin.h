#ifndef GITSOURCEPLUGIN_H
#define GITSOURCEPLUGIN_H

#include "../../qpmx/sourceplugin.h"

#include <QProcess>
#include <QHash>
#include <tuple>

class GitSourcePlugin : public QObject, public qpmx::SourcePlugin
{
	Q_OBJECT
	Q_INTERFACES(qpmx::SourcePlugin)
	Q_PLUGIN_METADATA(IID SourcePlugin_iid FILE "gitsource.json")

public:
	enum ProcessMode {
		Invalid,
		LsRemote,
		Clone,
		Tag,
		Push
	};
	Q_ENUM(ProcessMode)

	GitSourcePlugin(QObject *parent = nullptr);

	bool canSearch(const QString &provider) const override;
	bool canPublish(const QString &provider) const override;
	QString packageSyntax(const QString &provider) const override;
	bool packageValid(const qpmx::PackageInfo &package) const override;

	QJsonObject createPublisherInfo(const QString &provider) const override;

	void cancelAll(int timeout) override;

public slots:
	void searchPackage(int requestId, const QString &provider, const QString &query) override;
	void findPackageVersion(int requestId, const qpmx::PackageInfo &package) override;
	void getPackageSource(int requestId, const qpmx::PackageInfo &package, const QDir &targetDir) override;
	void publishPackage(int requestId, const QString &provider, const QDir &qpmxDir, const QVersionNumber &version, const QJsonObject &publisherInfo) override;

signals:
	void searchResult(int requestId, const QStringList &packageNames) final;
	void versionResult(int requestId, const QVersionNumber &version) final;
	void sourceFetched(int requestId) final;
	void packagePublished(int requestId) final;
	void sourceError(int requestId, const QString &error) final;

private slots:
	void finished(int exitCode, QProcess::ExitStatus exitStatus);
	void errorOccurred(QProcess::ProcessError error);

private:
	typedef std::tuple<int, ProcessMode, QVariantHash> tpl;
	static QRegularExpression _githubRegex;
	QHash<QProcess*, tpl> _processCache;

	QString pkgUrl(const qpmx::PackageInfo &package, QString *prefix = nullptr);
	QString pkgTag(const qpmx::PackageInfo &package);

	QDir createLogDir(const QString &action);
	QProcess *createProcess(const QString &type, const QStringList &arguments, bool stdLog = false, bool timeout = true);

	void lsRemoteDone(int requestId, QProcess *proc, int exitCode);
	void cloneDone(int requestId, QProcess *proc, int exitCode, const QVariantHash &params);
	void tagDone(int requestId, QProcess *proc, int exitCode, const QVariantHash &params);
	void pushDone(int requestId, QProcess *proc, int exitCode);
};

#endif // GITSOURCEPLUGIN_H
