#include "hookcommand.h"
using namespace qpmx;

HookCommand::HookCommand(QObject *parent) :
	Command(parent)
{}

QString HookCommand::commandName()
{
	return QStringLiteral("hook");
}

QString HookCommand::commandDescription()
{
	return tr("Creates a source file for the given hook id");
}

QSharedPointer<QCliNode> HookCommand::createCliNode()
{
	auto hookNode = QSharedPointer<QCliLeaf>::create();
	hookNode->setHidden(true);
	hookNode->addOption({
							{QStringLiteral("o"), QStringLiteral("out")},
							tr("The <path> of the file to be generated (required!)."),
							tr("path")
						});
	hookNode->addPositionalArgument(QStringLiteral("hook_ids"),
									tr("The ids of the hooks to be added to the hookup. Typically defined by the "
									   "QPMX_STARTUP_HASHES qmake variable."),
									QStringLiteral("[<hook_id> ...]"));
	return hookNode;
}

void HookCommand::initialize(QCliParser &parser)
{
	try {
		auto outFile = parser.value(QStringLiteral("out"));
		if(outFile.isEmpty())
			throw tr("You must specify the name of the file to generate as --out option");

		QFile out(outFile);
		QFile hookCache(outFile + QStringLiteral(".cache"));

		if(out.exists() && hookCache.exists()) {
			if(hookCache.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream hookStream(&hookCache);
				auto allHooks = parser.positionalArguments();
				while(!hookStream.atEnd()) {
					auto line = hookStream.readLine().trimmed();
					if(line.isEmpty())
						continue;
					allHooks.removeAll(line);
				}
				hookCache.close();

				if(allHooks.isEmpty()) {
					xDebug() << tr("Unchanged hooks. Doing nothing");
					qApp->quit();
					return;
				} else
					xDebug() << tr("Hooks changed. Regeneration hook file");
			} else {
				xWarning() << tr("Failed to open hook cache. Regenerating hooks. Open Error: %1")
							  .arg(hookCache.errorString());
			}
		}

		if(!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
			throw tr("Failed to create %1 file with error: %2")
					.arg(out.fileName())
					.arg(out.errorString());
		}

		xDebug() << tr("Creating hook file");
		QTextStream stream(&out);
		stream << "#include <QtCore/QCoreApplication>\n\n"
			   << "namespace __qpmx_startup_hooks {\n";
		foreach(auto hook, parser.positionalArguments())
			stream << "\tvoid hook_" << hook << "();\n";
		stream << "}\n\n";
		stream << "using namespace __qpmx_startup_hooks;\n"
			   << "static void __qpmx_root_hook() {\n";
		foreach(auto hook, parser.positionalArguments())
			stream << "\thook_" << hook << "();\n";
		stream << "}\n"
			   << "Q_COREAPP_STARTUP_FUNCTION(__qpmx_root_hook)\n";
		stream.flush();
		out.close();

		if(hookCache.open(QIODevice::WriteOnly | QIODevice::Text)) {
			xDebug() << tr("Creating hook cache");
			QTextStream hookStream(&hookCache);
			foreach (auto hook, parser.positionalArguments())
				hookStream << hook << "\n";
			hookStream.flush();
			hookCache.close();
		} else {
			xWarning() << tr("Failed to write hook cache with error: %1")
						  .arg(hookCache.errorString());
		}

		qApp->quit();
	} catch (QString &s) {
		xCritical() << s;
	}
}
