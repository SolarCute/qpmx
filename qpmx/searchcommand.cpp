#include "searchcommand.h"
#include <iostream>
using namespace qpmx;

#define print(x) std::cout << QString(x).toStdString() << std::endl

SearchCommand::SearchCommand(QObject *parent) :
	Command(parent),
	_providerCache(),
	_searchResults()
{}

void SearchCommand::initialize(QCliParser &parser)
{
	try {
		_short = parser.isSet(QStringLiteral("short"));

		if(parser.positionalArguments().isEmpty())
			throw tr("You must specify a search query to perform a search");
		auto query = parser.positionalArguments().join(QLatin1Char(' '));

		auto providers = parser.values(QStringLiteral("provider"));
		if(!providers.isEmpty()) {
			foreach(auto provider, providers) {
				if(!registry()->sourcePlugin(provider)->canSearch())
					throw tr("Provider \"%1\" does not support searching").arg(provider);
			}
		} else {
			foreach(auto provider, registry()->providerNames()) {
				if(registry()->sourcePlugin(provider)->canSearch())
					providers.append(provider);
			}
			xDebug() << tr("Searching providers: %1").arg(providers.join(tr(", ")));
		}

		foreach(auto provider, providers) {
			auto plg = registry()->sourcePlugin(provider);
			auto plgobj = dynamic_cast<QObject*>(plg);
			connect(plgobj, SIGNAL(searchResult(int,QStringList)),
					this, SLOT(searchResult(int,QStringList)),
					Qt::QueuedConnection);
			connect(plgobj, SIGNAL(sourceError(int,QString)),
					this, SLOT(sourceError(int,QString)),
					Qt::QueuedConnection);

			int id;
			do {
				id = qrand();
			} while(_providerCache.contains(id));
			_providerCache.insert(id, provider);
			plg->searchPackage(id, provider, query);
		}
	} catch(QString &s) {
		xCritical() << s;
	}
}

void SearchCommand::searchResult(int requestId, const QStringList &packageNames)
{
	auto provider = _providerCache.take(requestId);
	if(provider.isNull())
		return;

	xDebug() << tr("Found %n result(s) for provider \"%1\"", "", packageNames.size()).arg(provider);
	if(!packageNames.isEmpty())
		_searchResults.append({provider, packageNames});
	if(_providerCache.isEmpty())
		printResult();
}

void SearchCommand::sourceError(int requestId, const QString &error)
{
   auto provider = _providerCache.take(requestId);
   if(provider.isNull())
	   return;

   xCritical() << tr("Failed to search provider \"%1\" with error: %2")
				  .arg(provider)
				  .arg(error);
}

void SearchCommand::printResult()
{
	if(_short) {
		QStringList resList;
		foreach(auto res, _searchResults) {
			foreach(auto pkg, res.second)
				resList.append(PackageInfo(res.first, pkg).toString(false));
		}
		print(resList.join(QLatin1Char(' ')));
	} else {
		foreach(auto res, _searchResults) {
			print(tr("--- %1").arg(res.first + QLatin1Char(' '), -76, QLatin1Char('-')));
			foreach(auto pkg, res.second)
				print(tr(" %1").arg(pkg));
		}
	}

	qApp->quit();
}