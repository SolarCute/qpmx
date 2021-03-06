#ifndef DEVCOMMAND_H
#define DEVCOMMAND_H

#include "command.h"

class DevCommand : public Command
{
	Q_OBJECT

public:
	explicit DevCommand(QObject *parent = nullptr);

	QString commandName() const override;
	QString commandDescription() const override;
	QSharedPointer<QCliNode> createCliNode() const override;

protected slots:
	void initialize(QCliParser &parser) override;

private:
	void addDev(const QCliParser &parser);
	void removeDev(const QCliParser &parser);
	void commitDev(const QCliParser &parser);

	void addAlias(const QCliParser &parser);
	void removeAlias(const QCliParser &parser);

	void runPublish(const QStringList &providers, const QpmxDevDependency &dep, const QVersionNumber &version);
};

#endif // DEVCOMMAND_H
