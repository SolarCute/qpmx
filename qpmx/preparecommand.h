#ifndef PREPARECOMMAND_H
#define PREPARECOMMAND_H

#include "command.h"

class PrepareCommand : public Command
{
	Q_OBJECT

public:
	explicit PrepareCommand(QObject *parent = nullptr);

	QString commandName() override;
	QString commandDescription() override;
	QSharedPointer<QCliNode> createCliNode() override;

protected slots:
	void initialize(QCliParser &parser) override;
};

#endif // PREPARECOMMAND_H
