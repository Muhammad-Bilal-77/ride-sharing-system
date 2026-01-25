#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

class ResourceManager
{
public:
    /**
     * @brief Get the portable path for a resource file
     * @param filename - Name of the resource file (e.g., "user1.png", "preloader_ride_sharing_system.mp4")
     * @return Full path to the resource file if found, empty QString otherwise
     */
    static QString getResourcePath(const QString &filename)
    {
        // Strategy 1: Check in application directory (for packaged/portable builds)
        QString appPath = QCoreApplication::applicationDirPath() + "/" + filename;
        QFileInfo appFileInfo(appPath);
        if (appFileInfo.exists() && appFileInfo.isFile()) {
            qDebug() << "Resource found in app directory:" << appPath;
            return appPath;
        }

        // Strategy 2: Check in parent directory (for development builds)
        QDir exeDir(QCoreApplication::applicationDirPath());
        exeDir.cdUp();
        QString parentPath = exeDir.absoluteFilePath(filename);
        QFileInfo parentFileInfo(parentPath);
        if (parentFileInfo.exists() && parentFileInfo.isFile()) {
            qDebug() << "Resource found in parent directory:" << parentPath;
            return parentPath;
        }

        // Strategy 3: Check in project root (for development builds with relative paths)
        QDir projectDir(QCoreApplication::applicationDirPath());
        projectDir.cdUp();
        projectDir.cdUp();
        QString projectPath = projectDir.absoluteFilePath(filename);
        QFileInfo projectFileInfo(projectPath);
        if (projectFileInfo.exists() && projectFileInfo.isFile()) {
            qDebug() << "Resource found in project directory:" << projectPath;
            return projectPath;
        }

        // Strategy 4: Check current working directory
        QString currentPath = QDir::currentPath() + "/" + filename;
        QFileInfo currentFileInfo(currentPath);
        if (currentFileInfo.exists() && currentFileInfo.isFile()) {
            qDebug() << "Resource found in current directory:" << currentPath;
            return currentPath;
        }

        // Resource not found - log all paths checked
        qWarning() << "Resource NOT found:" << filename;
        qWarning() << "Checked paths:";
        qWarning() << "  1. App directory:" << appPath;
        qWarning() << "  2. Parent directory:" << parentPath;
        qWarning() << "  3. Project root:" << projectPath;
        qWarning() << "  4. Current directory:" << currentPath;

        return QString(); // Return empty string if not found
    }
};

#endif // RESOURCEMANAGER_H
