#ifndef CREATECOMMAND_H
#define CREATECOMMAND_H

#include "command.h"

class CreateCommand : public Command
{
	Q_OBJECT

public:
	explicit CreateCommand(QObject *parent = nullptr);

	QString commandName() override;
	QString commandDescription() override;
	QSharedPointer<QCliNode> createCliNode() override;

protected slots:
	void initialize(QCliParser &parser) override;

private:
	void runBaseInit();
	void runPrepare(const QString &provider);
};

#endif // CREATECOMMAND_H
