#include "listcommand.h"
#include "compilecommand.h"

#include <iostream>
using namespace qpmx;

#define print(x) std::cout << QString(x).toStdString() << std::endl

ListCommand::ListCommand(QObject *parent) :
	Command(parent)
{}

QString ListCommand::commandName() const
{
	return QStringLiteral("list");
}

QString ListCommand::commandDescription() const
{
	return tr("List things like providers, qmake versions and other components of qpmx.");
}

QSharedPointer<QCliNode> ListCommand::createCliNode() const
{
	auto listNode = QSharedPointer<QCliContext>::create();
	listNode->addOption({
							QStringLiteral("short"),
							QStringLiteral("Only list provider/qmake names, no other details.")
						});
	listNode->addLeafNode(QStringLiteral("providers"),
						  tr("List the package provider backends that are available for qpmx."));
	listNode->addLeafNode(QStringLiteral("kits"),
						  tr("List all currently known Qt kits and the corresponding qmake executable."));

	return listNode;
}

void ListCommand::initialize(QCliParser &parser)
{
	try {
		if(parser.enterContext(QStringLiteral("providers")))
			listProviders(parser);
		else if(parser.enterContext(QStringLiteral("kits")))
			listKits(parser);
		else
			Q_UNREACHABLE();
		qApp->quit();
	} catch (QString &s) {
		xCritical() << s;
	}
	parser.leaveContext();
}

void ListCommand::listProviders(const QCliParser &parser)
{
	if(parser.isSet(QStringLiteral("short")))
		print(registry()->providerNames().join(QLatin1Char(' ')));
	else {
		const auto providers = registry()->providerNames();
		QList<QStringList> rows;
		rows.reserve(providers.size());
		for(const auto &provider : providers) {
			auto plugin = registry()->sourcePlugin(provider);
			rows.append({
							provider,
							plugin->canSearch(provider) ? tr("yes") : tr("no"),
							plugin->canPublish(provider) ? tr("yes") : tr("no"),
							plugin->packageSyntax(provider)
						});
		}

		printTable({tr("Provider"), tr("Can Search"), tr("Can Publish"), tr("Package Syntax")},
				   {10, 5, 5, 0},
				   rows);
	}
}

void ListCommand::listKits(const QCliParser &parser)
{
	auto kits = QtKitInfo::readFromSettings(buildDir());

	if(parser.isSet(QStringLiteral("short"))) {
		QStringList paths;
		paths.reserve(kits.size());
		for(const auto &kit : kits)
			paths.append(kit.path);
		print(paths.join(QLatin1Char(' ')));
	} else {
		QList<QStringList> rows;
		rows.reserve(kits.size());
		for(const auto &kit : kits) {
			rows.append({
							kit.qtVer.toString(),
							kit.qmakeVer.toString(),
							kit.xspec,
							kit.id.toString(),
							kit.path
						});
		}

		printTable({tr("Qt"), tr("qmake"), tr("xspec"), tr("ID"), tr("qmake path")},
				   {6, 5, 15, 38, 0},
				   rows);
	}
}
