/*
* SPDX-FileCopyrightText: (C) 2021 Carl Schwan <carl@carlschwan.eu>
* SPDX-FileCopyrightText: (C) 2021 Claudio Cambra <claudio.cambra@gmail.com>
* 
* SPDX-LicenseRef: GPL-3.0-or-later
*/

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml>

#include <QUrl>
#include <QIcon>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <KLocalizedContext>
#include <KAboutData>
#include <KLocalizedString>

#include "kountdownmodel.h"
#include "importexport.h"
#include "aboutdatapasser.h"
#include "daykountdownconfig.h"
/* This last header file is auto-created by daykountdown.kcfgc
 * It allows us to easily instantiate the config class by the name provided there.
 */

// Define the database driver in a string
const QString DRIVER(QStringLiteral("QSQLITE"));

/* #ifdefs are ifs that affect the preprocessor.
 * We can use this to compile specific chunks of code depending on the platform!
 */
#ifdef Q_OS_ANDROID
Q_DECL_EXPORT 
#endif
int main(int argc, char *argv[])
{
	// Enable HiDPI scaling
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Material"));
#else
	// QApplication handles initialisation and includes extensive functionality
	QApplication app(argc, argv);
	if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
#endif

	KLocalizedString::setApplicationDomain("daykountdown");

	// KAboutData instances hold information about the application
	KAboutData about(QStringLiteral("daykountdown"), i18nc("@title", "DayKountdown"), QStringLiteral("0.1"),
						i18nc("@title", "A day countdown application"),
						KAboutLicense::GPL_V3);

	about.addAuthor(i18nc("@info:credit", "Claudio Cambra"), i18nc("@info:credit", "Creator"), 
					QStringLiteral("claudio.cambra@gmail.com"));
	about.addAuthor(i18nc("@info:credit", "Carl Schwan"), i18nc("@info:credit", "SQLite pro and code review"));

	// Sets the KAboutData instance
	KAboutData::setApplicationData(about);
	QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.daykountdown")));
	
	// We are instantiating the kcfg here
	auto config = DayKountdownConfig::self();

	// Q_ASSERTs hald the problem if the argument is false
	Q_ASSERT(QSqlDatabase::isDriverAvailable(DRIVER));
	
	/*
	 * .mkpath() creates the directory oath, including all parent directories (returning true if successful)
	 * cleanPath() returns the path with directory separators normalised ("/") and redundant ones removed 
	 * 	also "." and ".." resolved
	 * QStandardPaths is a class that provides methods to query standard locations on the filesystem
	 * writableLocation() returns the directory where files of QStandardPaths::DataLocation type should be written to
	 */
	Q_ASSERT(QDir().mkpath(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation))));
	// Creates SQLite database object instance
	QSqlDatabase db = QSqlDatabase::addDatabase(DRIVER);
	// The auto keyword automatically decides the type of the variable 'path' at compile time
	// This line defines a path for our application to have a folder to save stuff in
	// qApp macro returns a pointer to the running QApplication instance
	const auto path = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/") + qApp->applicationName());
	db.setDatabaseName(path);
	if (!db.open()) {
		qCritical() << db.lastError() << "while opening database at" << path;
	}

	// Let Qt parse and remove arguments meant to affect Qt
	QCommandLineParser parser;
	about.setupCommandLine(&parser);
	parser.process(app);
	about.processCommandLine(&parser);

	QQmlApplicationEngine engine;
	
	// We instantiate the AboutDataPasser class so we can easily pass the about data to QML
	AboutDataPasser AboutData;
	AboutData.setAboutData(about);
	
	// Lets you import instantiations of these classes into QML code
	qmlRegisterSingletonInstance("org.kde.daykountdown.private", 1, 0, "KountdownModel", new KountdownModel(qApp));
	qmlRegisterSingletonInstance("org.kde.daykountdown.private", 1, 0, "ImportExport", new ImportExport());
	qmlRegisterSingletonInstance("org.kde.daykountdown.private", 1, 0, "AboutData", &AboutData);
	qmlRegisterSingletonInstance("org.kde.daykountdown.private", 1, 0, "Config", config);

	// Set up localisation functionality
	engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
	// Load main.qml
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

	// Stop function if QML is empty
	if (engine.rootObjects().isEmpty()) {
		return -1;
	}
	
	return app.exec();
}
