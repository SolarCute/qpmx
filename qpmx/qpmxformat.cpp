#include "qpmxformat.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonSerializer>
#include <QSaveFile>

QpmxDependency::QpmxDependency() :
	provider(),
	package(),
	version(),
	source(false)
{}

QpmxDependency::QpmxDependency(const qpmx::PackageInfo &package, bool source) :
	provider(package.provider()),
	package(package.package()),
	version(package.version()),
	source(source)
{}

bool QpmxDependency::operator==(const QpmxDependency &other) const
{
	//only provider and package "identify" the dependency
	return provider == other.provider &&
			package == other.package;
}

qpmx::PackageInfo QpmxDependency::pkg(const QString &provider) const
{
	return {provider.isEmpty() ? this->provider : provider, package, version};
}

QpmxDependency::operator QString() const
{
	auto res = package;
	if(!provider.isNull())
		res.prepend(provider + QStringLiteral("::"));
	if(!version.isNull())
		res.append(QLatin1Char('@') + version.toString());
	return res;
}

QpmxFormat::QpmxFormat() :
	dependencies()
{}

QpmxFormat QpmxFormat::readDefault(bool mustExist)
{
	QFile qpmxFile(QStringLiteral("./qpmx.json"));
	if(qpmxFile.exists()) {
		if(!qpmxFile.open(QIODevice::ReadOnly | QIODevice::Text))
			throw tr("Failed to open qpmx.json with error: %1").arg(qpmxFile.errorString());

		try {
			QJsonSerializer ser;
			ser.addJsonTypeConverter(new VersionConverter());
			return ser.deserializeFrom<QpmxFormat>(&qpmxFile);
		} catch(QJsonSerializerException &e) {
			qDebug() << e.what();
			throw tr("qpmx.json contains invalid data");
		}
	} else if(mustExist)
		throw tr("qpmx.json file does not exist");
	else
		return {};
}

void QpmxFormat::writeDefault(const QpmxFormat &data)
{
	QSaveFile qpmxFile(QStringLiteral("./qpmx.json"));
	if(!qpmxFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throw tr("Failed to open qpmx.json with error: %1").arg(qpmxFile.errorString());

	try {
		QJsonSerializer ser;
		ser.addJsonTypeConverter(new VersionConverter());
		//ser.serializeTo(&qpmxFile, data);
		auto json = ser.serialize(data);
		qpmxFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
	} catch(QJsonSerializerException &e) {
		qDebug() << e.what();
		throw tr("Failed to write qpmx.json");
	}

	if(!qpmxFile.commit())
		throw tr("Failed to save qpmx.json with error: %1").arg(qpmxFile.errorString());
}



bool VersionConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<QVersionNumber>();
}

QList<QJsonValue::Type> VersionConverter::jsonTypes() const
{
	return {QJsonValue::String};
}

QJsonValue VersionConverter::serialize(int propertyType, const QVariant &value, const SerializationHelper *helper) const
{
	Q_ASSERT(propertyType == qMetaTypeId<QVersionNumber>());
	Q_UNUSED(helper)
	return value.value<QVersionNumber>().toString();
}

QVariant VersionConverter::deserialize(int propertyType, const QJsonValue &value, QObject *parent, const SerializationHelper *helper) const
{
	Q_ASSERT(propertyType == qMetaTypeId<QVersionNumber>());
	Q_UNUSED(helper)
	Q_UNUSED(parent)
	return QVariant::fromValue(QVersionNumber::fromString(value.toString()));
}