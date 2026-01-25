#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "resourcemanager.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QFile>
#include <QTextStream>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QSpinBox>
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , sharedCity(nullptr)
    , sharedDispatchEngine(nullptr)
    , cityLoaded(false)
    , driversInitialized(false)
{
    ui->setupUi(this);
    stackedWidget = new QStackedWidget(this);
    homePage = new QWidget();
    rollbackManagerPage = nullptr;
    riderWindow = nullptr;
    QVBoxLayout *homeLayout = new QVBoxLayout(homePage);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(0);
    
    // Set window size
    this->resize(1000, 700);

    // Set gradient background
    this->setStyleSheet(
        "QMainWindow { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #FFFFFF, stop:1 rgb(232, 245, 233));"
        "}"
    );

    // Style the title
    ui->labelTitle->setStyleSheet(
        "color: #2E7D32;"
        "font-size: 32px;"
        "font-weight: bold;"
        "padding: 20px;"
        "background: transparent;"
    );

    // Create container for button and image
    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");
    
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(60, 30, 60, 60);
    contentLayout->setSpacing(60);

    // LEFT: Button container
    QWidget *buttonContainer = new QWidget();
    buttonContainer->setStyleSheet("background: transparent;");
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(15);

    // Choose User button
    chooseUserButton = new QPushButton("Choose User");
    chooseUserButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.55), stop:1 rgba(255,255,255,0.35));"
        "    color: #1B5E20;"
        "    border: 2px solid rgba(255, 255, 255, 0.6);"
        "    border-radius: 14px;"
        "    font-size: 20px;"
        "    font-weight: 600;"
        "    letter-spacing: 1px;"
        "    min-width: 170px;"
        "    min-height: 30px;"
        "    padding: 12px 22px;"
        "    backdrop-filter: blur(4px);"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.75), stop:1 rgba(255,255,255,0.55));"
        "    border: 2px solid rgba(255, 255, 255, 0.8);"
        "    color: #2E7D32;"
    "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.45), stop:1 rgba(255,255,255,0.30));"
        "    border: 2px solid rgba(255, 255, 255, 0.5);"
        "    padding-top: 16px;"
        "}"
    );
    chooseUserButton->setCursor(Qt::PointingHandCursor);

    // Add shadow effect to button
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(4);
    shadowEffect->setColor(QColor(0, 0, 0, 100));
    chooseUserButton->setGraphicsEffect(shadowEffect);
    buttonLayout->addWidget(chooseUserButton);

    // Rollback Manager button (same style as Choose User)
    rollbackManagerButton = new QPushButton("Rollback Manager");
    rollbackManagerButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.55), stop:1 rgba(255,255,255,0.35));"
        "    color: #1B5E20;"
        "    border: 2px solid rgba(255, 255, 255, 0.6);"
        "    border-radius: 14px;"
        "    font-size: 20px;"
        "    font-weight: 600;"
        "    letter-spacing: 1px;"
        "    min-width: 170px;"
        "    min-height: 30px;"
        "    padding: 12px 22px;"
        "    backdrop-filter: blur(4px);"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.75), stop:1 rgba(255,255,255,0.55));"
        "    border: 2px solid rgba(255, 255, 255, 0.8);"
        "    color: #2E7D32;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.45), stop:1 rgba(255,255,255,0.30));"
        "    border: 2px solid rgba(255, 255, 255, 0.5);"
        "    padding-top: 16px;"
        "}"
    );
    rollbackManagerButton->setCursor(Qt::PointingHandCursor);
    
    QGraphicsDropShadowEffect *rollbackShadow = new QGraphicsDropShadowEffect();
    rollbackShadow->setBlurRadius(20);
    rollbackShadow->setXOffset(0);
    rollbackShadow->setYOffset(4);
    rollbackShadow->setColor(QColor(0, 0, 0, 100));
    rollbackManagerButton->setGraphicsEffect(rollbackShadow);
    buttonLayout->addWidget(rollbackManagerButton);

    // Analytics button
    analyticsButton = new QPushButton("Analytics");
    analyticsButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.55), stop:1 rgba(255,255,255,0.35));"
        "    color: #1B5E20;"
        "    border: 2px solid rgba(255, 255, 255, 0.6);"
        "    border-radius: 14px;"
        "    font-size: 20px;"
        "    font-weight: 600;"
        "    letter-spacing: 1px;"
        "    min-width: 170px;"
        "    min-height: 30px;"
        "    padding: 12px 22px;"
        "    backdrop-filter: blur(4px);"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.75), stop:1 rgba(255,255,255,0.55));"
        "    border: 2px solid rgba(255, 255, 255, 0.8);"
        "    color: #2E7D32;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(255,255,255,0.45), stop:1 rgba(255,255,255,0.30));"
        "    border: 2px solid rgba(255, 255, 255, 0.5);"
        "    padding-top: 16px;"
        "}"
    );
    analyticsButton->setCursor(Qt::PointingHandCursor);
    QGraphicsDropShadowEffect *analyticsShadow = new QGraphicsDropShadowEffect();
    analyticsShadow->setBlurRadius(20);
    analyticsShadow->setXOffset(0);
    analyticsShadow->setYOffset(4);
    analyticsShadow->setColor(QColor(0, 0, 0, 100));
    analyticsButton->setGraphicsEffect(analyticsShadow);
    buttonLayout->addWidget(analyticsButton);
    buttonLayout->addStretch();

    // RIGHT: Image
    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    
    // Load image using portable resource manager
    QString imgPath = ResourceManager::getResourcePath("Mask_group.png");
    
    if (!imgPath.isEmpty()) {
        QPixmap pixmap(imgPath);
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(pixmap.scaled(500, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            qDebug() << "Image loaded successfully from:" << imgPath << "size:" << pixmap.size();
        } else {
            imageLabel->setText("Failed to load image");
            qDebug() << "Failed to load pixmap from:" << imgPath;
        }
    } else {
        imageLabel->setText("Image not found");
        imageLabel->setStyleSheet("color: red; font-size: 18px;");
        qDebug() << "Image file not found: Mask_group.png";
    }

    // Add to content layout with equal spacing
    contentLayout->addStretch(1);
    contentLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);
    contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);

    homeLayout->addWidget(contentWidget, 1);

    stackedWidget->addWidget(homePage);
    stackedWidget->setCurrentWidget(homePage);
    ui->verticalLayout->addWidget(stackedWidget, 1);

    // Create rollback manager page
    createRollbackManagerPage();
    // Create analytics page
    createAnalyticsPage();
    
    // Load riders from dataset and hook button
    loadRiders();
    connect(chooseUserButton, &QPushButton::clicked, this, &MainWindow::showRiderMenu);
    connect(rollbackManagerButton, &QPushButton::clicked, this, &MainWindow::showRollbackManagerPage);
    connect(analyticsButton, &QPushButton::clicked, this, &MainWindow::showAnalyticsPage);
}

MainWindow::~MainWindow()
{
    delete sharedDispatchEngine;
    delete sharedCity;
    delete ui;
}

MainWindow::RestoredTripInfo MainWindow::getRestoredTripForRider(const QString &riderCode)
{
    RestoredTripInfo empty;
    empty.tripId = -1;
    if (restoredTrips.contains(riderCode)) {
        return restoredTrips.value(riderCode);
    }
    return empty;
}

void MainWindow::clearRestoredTrip(const QString &riderCode)
{
    restoredTrips.remove(riderCode);
}

void MainWindow::setRestoredTrip(const QString &riderCode, int tripId, const QString &pickup, 
                                const QString &dropoff, double fare, double distance, int driverId)
{
    RestoredTripInfo info;
    info.tripId = tripId;
    info.pickup = pickup;
    info.dropoff = dropoff;
    info.fare = fare;
    info.distance = distance;
    info.driverId = driverId;
    restoredTrips[riderCode] = info;
    qDebug() << "Stored restored trip" << tripId << "for rider" << riderCode;
}

bool MainWindow::ensureCityLoaded()
{
    if (cityLoaded && sharedCity)
        return true;
    
    if (!sharedCity)
        sharedCity = new City();
    
    QString locPath = resolveDataFile("city_locations_path_data/city-locations.csv");
    QString pathsPath = resolveDataFile("city_locations_path_data/paths.csv");
    
    if (locPath.isEmpty() || pathsPath.isEmpty())
    {
        qDebug() << "Could not find city data files";
        return false;
    }
    
    if (!sharedCity->loadLocations(locPath.toUtf8().constData()))
    {
        qDebug() << "Failed to load locations";
        return false;
    }
    
    if (!sharedCity->loadPaths(pathsPath.toUtf8().constData()))
    {
        qDebug() << "Failed to load paths";
        return false;
    }
    
    cityLoaded = true;
    qDebug() << "Shared city graph loaded successfully";
    return true;
}

void MainWindow::ensureDispatchEngine()
{
    if (!ensureCityLoaded())
        return;
    
    if (!sharedDispatchEngine)
    {
        sharedDispatchEngine = new DispatchEngine(sharedCity, 100, 200);
        qDebug() << "Shared dispatch engine created";
    }
    
    if (!driversInitialized)
    {
        initializeDrivers();
        driversInitialized = true;
    }
}

void MainWindow::initializeDrivers()
{
    if (!sharedDispatchEngine || !sharedCity)
        return;
    
    QHash<QString, QList<QString>> zoneStreetNodes;
    for (Node *n = sharedCity->getFirstNode(); n != nullptr; n = n->next)
    {
        QString type = QString::fromUtf8(n->locationType).toLower();
        if (!(type.contains("street") || type.contains("highway")))
            continue;
        QString zone = QString::fromUtf8(n->zone).trimmed();
        if (zone.isEmpty())
            continue;
        zoneStreetNodes[zone].append(QString::fromUtf8(n->id));
    }
    
    int driverId = 1;
    for (auto it = zoneStreetNodes.constBegin(); it != zoneStreetNodes.constEnd() && driverId <= 20; ++it)
    {
        int placed = 0;
        const QList<QString> &nodes = it.value();
        for (const QString &nodeId : nodes)
        {
            if (placed >= 5 || driverId > 20)
                break;
            sharedDispatchEngine->addDriver(driverId, nodeId.toUtf8().constData(), it.key().toUtf8().constData());
            driverId++;
            placed++;
        }
    }
    
    qDebug() << "Initialized 20 drivers across zones";
}


QString MainWindow::resolveDataFile(const QString &fileName) const
{
    // Look in executable dir
    QString path = QCoreApplication::applicationDirPath() + "/" + fileName;
    if (QFileInfo::exists(path)) return path;

    // Look two levels up (project root) + relative data folder
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cdUp();
    QString candidate = dir.filePath(fileName);
    if (QFileInfo::exists(candidate)) return candidate;

    // Look in project root + data folder
    QString dataCandidate = dir.filePath("city_locations_path_data/" + fileName);
    if (QFileInfo::exists(dataCandidate)) return dataCandidate;

    return QString();
}

void MainWindow::loadRiders()
{
    riders.clear();

    QString csvPath = resolveDataFile("city-locations.csv");
    if (csvPath.isEmpty()) {
        qDebug() << "city-locations.csv not found";
        return;
    }

    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open" << csvPath;
        return;
    }

    QTextStream in(&file);
    // Skip header
    if (!in.atEnd()) in.readLine();

    QHash<QString, int> perZoneCount;
    const int maxPerZone = 5;
    const int maxZones = 4; // 4 zones * 5 = 20 riders

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        // Simple CSV split (fields are quoted and do not contain commas apart from separators)
        QStringList parts = line.split(",");
        if (parts.size() < 8) continue;

        QString zone = parts[0].remove('"');
        QString colony = parts[1].remove('"');
        QString streetName = parts[3].remove('"');
        QString locName = parts[4].remove('"');
        QString locType = parts[5].remove('"');
        QString locId = parts[7].remove('"');

        if (locType != "home" && locType != "mall" && locType != "school") {
            continue; // focus on useful pickup locations
        }

        if (perZoneCount.value(zone, 0) >= maxPerZone) {
            continue;
        }

        // Limit to first few zones encountered
        if (perZoneCount.size() >= maxZones && !perZoneCount.contains(zone)) {
            continue;
        }

        Rider r;
        r.riderId = QString("R%1").arg(riders.size() + 1, 2, 10, QChar('0'));
        r.zone = zone;
        r.colony = colony;
        r.locationName = streetName + " / " + locName;
        r.locationId = locId;

        riders.append(r);
        perZoneCount[zone] += 1;
        
        // Initialize location tracking
        riderLocations[r.riderId] = r.locationId;

        if (riders.size() >= maxPerZone * maxZones) break;
    }

    qDebug() << "Loaded riders" << riders.size() << "from" << csvPath;
}

void MainWindow::showRiderMenu()
{
    if (riders.isEmpty()) {
        QMessageBox::warning(this, "No riders", "No rider data available.");
        return;
    }

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFFFFF, stop:1 #E8F5E9);"
        "  border: 1px solid rgba(46,125,50,0.35);"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "}"
        "QMenu::item {"
        "  padding: 10px 14px;"
        "  margin: 2px;"
        "  color: #1B5E20;"
        "  border-radius: 8px;"
        "}"
        "QMenu::item:selected {"
        "  background: rgba(102,187,106,0.35);"
        "  color: #1B5E20;"
        "}"
    );

    for (const Rider &r : std::as_const(riders)) {
        // Get current location (tracked or original)
        QString currentLocationId = riderLocations.value(r.riderId, r.locationId);
        QString displayText = QString("%1 | %2 | %3 | %4")
            .arg(r.riderId)
            .arg(r.zone)
            .arg(r.colony)
            .arg(currentLocationId);
        QAction *act = menu.addAction(displayText);
        act->setData(QVariant::fromValue(r.riderId));
        act->setProperty("locationId", r.locationId);
    }

    QAction *chosen = menu.exec(chooseUserButton->mapToGlobal(QPoint(0, chooseUserButton->height())));
    if (chosen) {
        QString riderId = chosen->data().toString();
        QString originalLocationId = chosen->property("locationId").toString();
        
        // Use tracked location if available, otherwise use original
        QString currentLocationId = riderLocations.value(riderId, originalLocationId);
        
        chooseUserButton->setText("Chosen: " + riderId);
        qDebug() << "Selected rider" << riderId << "at current location" << currentLocationId;
        
        // Switch to rider page within the same window
        if (riderWindow) {
            stackedWidget->removeWidget(riderWindow);
            riderWindow->deleteLater();
            riderWindow = nullptr;
        }

        riderWindow = new RiderWindow(riderId, currentLocationId, stackedWidget);
        
        // Ensure shared resources are initialized
        ensureCityLoaded();
        ensureDispatchEngine();
        
        // Pass shared resources to RiderWindow
        riderWindow->setSharedResources(sharedCity, sharedDispatchEngine);
        
        connect(riderWindow, &RiderWindow::backRequested, this, [this]() {
            stackedWidget->setCurrentWidget(homePage);
            ui->labelTitle->setVisible(true);
        });
        connect(riderWindow, &RiderWindow::locationUpdated, this, &MainWindow::updateRiderLocation);
        stackedWidget->addWidget(riderWindow);
        stackedWidget->setCurrentWidget(riderWindow);
        ui->labelTitle->setVisible(false);
        
        // Check if this rider has a restored trip - do this AFTER the window is shown
        if (restoredTrips.contains(riderId)) {
            RestoredTripInfo tripInfo = restoredTrips.value(riderId);
            clearRestoredTrip(riderId);
            
            qDebug() << "Found restored trip" << tripInfo.tripId << "for rider" << riderId;
            qDebug() << "Pickup:" << tripInfo.pickup << "Dropoff:" << tripInfo.dropoff;
            
            // Show the section after a delay to ensure UI is fully rendered
            QTimer::singleShot(500, this, [this, tripInfo]() {
                qDebug() << "Showing restored trip section";
                if (riderWindow) {
                    riderWindow->showRestoredTripSection(tripInfo.pickup, tripInfo.dropoff, 
                                                         tripInfo.fare, tripInfo.distance, tripInfo.tripId);
                }
            });
        }
    }
}

void MainWindow::updateRiderLocation(const QString &riderId, const QString &newLocation)
{
    riderLocations[riderId] = newLocation;
    qDebug() << "Updated location for rider" << riderId << "to" << newLocation;
}
void MainWindow::createRollbackManagerPage()
{
    rollbackManagerPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(rollbackManagerPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Title
    QLabel *titleLabel = new QLabel("Program Snapshots & Rollback Manager");
    titleLabel->setStyleSheet(
        "font-size: 24px; "
        "font-weight: bold; "
        "color: #2E7D32; "
        "padding: 10px;"
    );
    layout->addWidget(titleLabel);

    // Info label
    QLabel *infoLabel = new QLabel("This page displays all saved snapshots of the program state.");
    infoLabel->setStyleSheet("font-size: 14px; color: #555; padding: 5px;");
    layout->addWidget(infoLabel);

    // Snapshot display area with scrollable content
    QWidget *snapshotContainer = new QWidget();
    snapshotContainer->setStyleSheet(
        "QWidget { "
        "background: white; "
        "border: 2px solid #DDD; "
        "border-radius: 8px; "
        "padding: 15px; "
        "}"
    );
    snapshotContainerLayout = new QVBoxLayout(snapshotContainer);
    snapshotContainerLayout->setSpacing(10);

    QLabel *snapshotLabel = new QLabel("Current Program Snapshots:");
    snapshotLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2E7D32;");
    snapshotContainerLayout->addWidget(snapshotLabel);

    // Create scrollable area for snapshots
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { width: 10px; background: #F0F0F0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #CCC; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #AAA; }"
    );
    
    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(8);
    scrollArea->setWidget(scrollContent);
    
    snapshotContainerLayout->addWidget(scrollArea, 1);

    layout->addWidget(snapshotContainer, 1);

    // Button container at bottom
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    QPushButton *refreshButton = new QPushButton("Refresh Snapshots");
    refreshButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(46,125,50,0.8), stop:1 rgba(56,142,60,0.6));"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 10px 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(56,142,60,0.9), stop:1 rgba(66,133,66,0.7));"
        "}"
    );
    refreshButton->setCursor(Qt::PointingHandCursor);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshSnapshotDisplay);
    buttonLayout->addWidget(refreshButton);

    // Rollback k operations UI
    QHBoxLayout *rollbackKLayout = new QHBoxLayout();
    rollbackKLayout->setSpacing(10);
    
    QLabel *kLabel = new QLabel("Rollback last");
    kLabel->setStyleSheet("font-size: 13px; color: #333; font-weight: 500;");
    rollbackKLayout->addWidget(kLabel);
    
    QSpinBox *kSpinBox = new QSpinBox();
    kSpinBox->setMinimum(1);
    kSpinBox->setMaximum(100);
    kSpinBox->setValue(1);
    kSpinBox->setStyleSheet(
        "QSpinBox {"
        "    border: 1px solid #CCC;"
        "    border-radius: 4px;"
        "    padding: 5px;"
        "    background: white;"
        "    color: #333;"
        "}"
    );
    rollbackKLayout->addWidget(kSpinBox);
    
    QLabel *operLabel = new QLabel("operations");
    operLabel->setStyleSheet("font-size: 13px; color: #333; font-weight: 500;");
    rollbackKLayout->addWidget(operLabel);
    
    QPushButton *rollbackKBtn = new QPushButton("Rollback");
    rollbackKBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(244,67,54,0.8), stop:1 rgba(211,47,47,0.6));"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(244,67,54,0.9), stop:1 rgba(211,47,47,0.7));"
        "}"
    );
    rollbackKBtn->setCursor(Qt::PointingHandCursor);
    connect(rollbackKBtn, &QPushButton::clicked, this, [this, kSpinBox]() {
        int k = kSpinBox->value();
        if (sharedDispatchEngine && sharedDispatchEngine->getRollbackManager()) {
            RollbackManager *rollbackMgr = sharedDispatchEngine->getRollbackManager();
            
            // Before rollback, collect history entries that will be removed
            struct HistoryToRemove {
                QString riderCode;
                int tripId;
            };
            QVector<HistoryToRemove> historyEntriesToRemove;
            
            // Collect riders whose locations changed in the k operations
            QSet<QString> ridersWithLocationChanges;
            
            // Collect active trips that will be restored
            QHash<QString, int> tripsToRestore;  // riderCode -> tripId
            
            // Traverse the top k snapshots to find history entries and riders with location changes
            OperationSnapshot *snap = rollbackMgr->getSnapshotStack();
            int count = 0;
            while (snap && count < k) {
                if (snap->operationType == 20) {  // TRIP_HISTORY_ENTRY
                    HistoryToRemove entry;
                    entry.riderCode = QString::fromUtf8(snap->riderCode);
                    entry.tripId = snap->tripId;
                    historyEntriesToRemove.append(entry);
                }
                else if (snap->operationType == 3 && snap->riderCode[0] != '\0') {  // RIDER_LOCATION_CHANGE
                    QString riderCode = QString::fromUtf8(snap->riderCode);
                    ridersWithLocationChanges.insert(riderCode);
                }
                else if ((snap->operationType == 0 || snap->operationType == 1 || snap->operationType == 2 || snap->operationType == 11) 
                         && snap->tripId >= 0) {  // ASSIGN, CANCEL, COMPLETE, MOVEMENT
                    // Any trip with a snapshot in the rollback range could be restored
                    if (sharedDispatchEngine) {
                        Trip *trip = sharedDispatchEngine->getTrip(snap->tripId);
                        if (trip && snap->riderCode[0] != '\0') {
                            QString riderCode = QString::fromUtf8(snap->riderCode);
                            // Track this trip for potential restoration notification
                            tripsToRestore[riderCode] = snap->tripId;
                            qDebug() << "Detected trip" << snap->tripId << "for rider" << riderCode << "in rollback range";
                        }
                    }
                }
                snap = snap->next;
                count++;
            }
            
            // Perform rollback
            Trip **trips = new Trip*[sharedDispatchEngine->getTripCount()];
            for (int i = 0; i < sharedDispatchEngine->getTripCount(); i++) {
                trips[i] = sharedDispatchEngine->getTrip(i + 1);
            }
            
            bool success = rollbackMgr->rollbackLastK(k, trips, sharedDispatchEngine->getTripCount(), sharedDispatchEngine);
            delete[] trips;
            
            if (success) {
                // Remove history entries from session store
                for (const HistoryToRemove &entry : historyEntriesToRemove) {
                    RiderWindow::removeHistoryEntry(entry.riderCode, entry.tripId);
                }
                
                // Store restored trips with full info for later notification
                for (auto it = tripsToRestore.begin(); it != tripsToRestore.end(); ++it) {
                    QString riderCode = it.key();
                    int tripId = it.value();
                    
                    // Look for pickup/dropoff info in the snapshots
                    QString pickup, dropoff;
                    double fare = 0, distance = 0;
                    int driverId = -1;
                    
                    OperationSnapshot *snap2 = rollbackMgr->getSnapshotStack();
                    for (int j = 0; j < k && snap2; j++) {
                        if (snap2->tripId == tripId && snap2->operationType == 20) {  // TRIP_HISTORY_ENTRY
                            pickup = QString::fromUtf8(snap2->pickup);
                            dropoff = QString::fromUtf8(snap2->dropoff);
                            fare = snap2->fare;
                            distance = snap2->distance;
                            break;
                        }
                        snap2 = snap2->next;
                    }
                    
                    setRestoredTrip(riderCode, tripId, pickup, dropoff, fare, distance, driverId);
                }
                
                // After rollback, restore rider locations from remaining snapshots
                // For each rider whose location changed, find their most recent location in remaining stack
                for (const QString &riderCode : ridersWithLocationChanges) {
                    OperationSnapshot *remainingSnap = rollbackMgr->getSnapshotStack();
                    while (remainingSnap) {
                        if (remainingSnap->operationType == 3 && 
                            QString::fromUtf8(remainingSnap->riderCode) == riderCode &&
                            remainingSnap->riderLocation[0] != '\0') {
                            QString restoredLocation = QString::fromUtf8(remainingSnap->riderLocation);
                            riderLocations[riderCode] = restoredLocation;
                            qDebug() << "Restored rider" << riderCode << "to location" << restoredLocation;
                            break;
                        }
                        remainingSnap = remainingSnap->next;
                    }
                    
                    // If no location found in remaining snapshots, find from original rider data
                    if (!riderLocations.contains(riderCode)) {
                        for (const Rider &r : std::as_const(riders)) {
                            if (r.riderId == riderCode) {
                                riderLocations[riderCode] = r.locationId;
                                qDebug() << "Restored rider" << riderCode << "to original location" << r.locationId;
                                break;
                            }
                        }
                    }
                }
                
                QString msg = QString("Successfully rolled back last %1 operation(s).\n\nRider locations and driver states have been restored.").arg(k);
                if (!restoredTrips.isEmpty()) {
                    msg += QString("\n\n%1 trip(s) have been restored. Riders will be notified when they return to their booking page.").arg(restoredTrips.size());
                }
                
                QMessageBox::information(this, "Success", msg);
                refreshSnapshotDisplay();
            } else {
                QMessageBox::warning(this, "Rollback Failed", 
                    QString("Could not rollback %1 operation(s). Not enough operations or invalid state.").arg(k));
            }
        }
    });
    rollbackKLayout->addWidget(rollbackKBtn);
    rollbackKLayout->addStretch();
    
    buttonLayout->addLayout(rollbackKLayout);
    
    // Rollback entire trip UI (last trip)
    QHBoxLayout *rollbackTripLayout = new QHBoxLayout();
    rollbackTripLayout->setSpacing(10);
    
    QPushButton *rollbackLastTripBtn = new QPushButton("Rollback Last Entire Trip");
    rollbackLastTripBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(156,39,176,0.8), stop:1 rgba(123,31,162,0.6));"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(156,39,176,0.9), stop:1 rgba(123,31,162,0.7));"
        "}"
    );
    rollbackLastTripBtn->setCursor(Qt::PointingHandCursor);
    connect(rollbackLastTripBtn, &QPushButton::clicked, this, [this]() {
        if (!sharedDispatchEngine || !sharedDispatchEngine->getRollbackManager()) {
            QMessageBox::warning(this, "Error", "Rollback manager not available.");
            return;
        }
        
        RollbackManager *rollbackMgr = sharedDispatchEngine->getRollbackManager();
        
        // Find the most recent trip ID from snapshots
        int lastTripId = -1;
        OperationSnapshot *snap = rollbackMgr->getSnapshotStack();
        while (snap) {
            if ((snap->operationType == 0 || snap->operationType == 1 || snap->operationType == 2 || 
                 snap->operationType == 11 || snap->operationType == 20) && snap->tripId >= 0) {
                lastTripId = snap->tripId;
                break;
            }
            snap = snap->next;
        }
        
        if (lastTripId < 0) {
            QMessageBox::warning(this, "No Trip Found", 
                "No trip operations found in the snapshot stack.");
            return;
        }
        
        // Collect all snapshot indices related to this trip
        QVector<int> tripSnapshotIndices;
        snap = rollbackMgr->getSnapshotStack();
        int index = 0;
        while (snap) {
            if (snap->tripId == lastTripId) {
                tripSnapshotIndices.append(index);
            }
            snap = snap->next;
            index++;
        }
        
        if (tripSnapshotIndices.isEmpty()) {
            QMessageBox::warning(this, "Error", "No snapshots found for the last trip.");
            return;
        }
        
        // Get trip details for confirmation
        QString riderCode;
        QString pickup, dropoff;
        snap = rollbackMgr->getSnapshotStack();
        while (snap) {
            if (snap->tripId == lastTripId && snap->riderCode[0] != '\0') {
                riderCode = QString::fromUtf8(snap->riderCode);
                if (snap->operationType == 20) {
                    pickup = QString::fromUtf8(snap->pickup);
                    dropoff = QString::fromUtf8(snap->dropoff);
                    break;
                }
            }
            snap = snap->next;
        }
        
        // Confirm with user
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Rollback",
            QString("Rollback entire trip #%1?\n\nRider: %2\nPickup: %3\nDropoff: %4\n\nThis will rollback %5 operation(s) and restore rider/driver states to before this trip.\n\nContinue?")
                .arg(lastTripId).arg(riderCode, pickup.isEmpty() ? "N/A" : pickup, dropoff.isEmpty() ? "N/A" : dropoff)
                .arg(tripSnapshotIndices.size()),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply != QMessageBox::Yes) {
            return;
        }
        
        // Rollback each snapshot related to this trip (from most recent to oldest)
        Trip **trips = new Trip*[sharedDispatchEngine->getTripCount()];
        for (int i = 0; i < sharedDispatchEngine->getTripCount(); i++) {
            trips[i] = sharedDispatchEngine->getTrip(i + 1);
        }
        
        bool success = true;
        int rolledBackCount = 0;
        
        // Roll back snapshots one by one (they're already in order from most recent)
        for (int i = 0; i < tripSnapshotIndices.size(); i++) {
            // Each rollback removes the top snapshot, so we always rollback index 0 relative to current position
            // But we need to skip non-trip snapshots that are on top
            
            // Find current position of next trip snapshot
            snap = rollbackMgr->getSnapshotStack();
            int currentIndex = 0;
            while (snap && snap->tripId != lastTripId) {
                snap = snap->next;
                currentIndex++;
            }
            
            if (!snap) {
                break; // No more snapshots for this trip
            }
            
            // Rollback from top down to this snapshot
            if (!rollbackMgr->rollbackLastK(currentIndex + 1, trips, sharedDispatchEngine->getTripCount(), sharedDispatchEngine)) {
                success = false;
                break;
            }
            rolledBackCount++;
        }
        
        delete[] trips;
        
        if (success && rolledBackCount > 0) {
            // Remove history entry for this trip
            if (!riderCode.isEmpty()) {
                RiderWindow::removeHistoryEntry(riderCode, lastTripId);
            }
            
            // Find and restore rider location to before trip
            if (!riderCode.isEmpty()) {
                OperationSnapshot *remainingSnap = rollbackMgr->getSnapshotStack();
                bool foundLocation = false;
                while (remainingSnap) {
                    if (remainingSnap->operationType == 3 && 
                        QString::fromUtf8(remainingSnap->riderCode) == riderCode &&
                        remainingSnap->riderLocation[0] != '\0') {
                        QString restoredLocation = QString::fromUtf8(remainingSnap->riderLocation);
                        riderLocations[riderCode] = restoredLocation;
                        qDebug() << "Restored rider" << riderCode << "to location" << restoredLocation;
                        foundLocation = true;
                        break;
                    }
                    remainingSnap = remainingSnap->next;
                }
                
                if (!foundLocation) {
                    for (const Rider &r : std::as_const(riders)) {
                        if (r.riderId == riderCode) {
                            riderLocations[riderCode] = r.locationId;
                            qDebug() << "Restored rider" << riderCode << "to original location" << r.locationId;
                            break;
                        }
                    }
                }
            }
            
            QMessageBox::information(this, "Success", 
                QString("Successfully rolled back entire trip #%1.\n\nRider and driver have been restored to their pre-trip states.").arg(lastTripId));
            refreshSnapshotDisplay();
        } else {
            QMessageBox::warning(this, "Rollback Failed", 
                QString("Failed to rollback trip #%1. Rolled back %2 of %3 operations.")
                    .arg(lastTripId).arg(rolledBackCount).arg(tripSnapshotIndices.size()));
            refreshSnapshotDisplay();
        }
    });
    rollbackTripLayout->addWidget(rollbackLastTripBtn);
    
    QLabel *tripHelpLabel = new QLabel("(Rollbacks all operations of the most recent trip)");
    tripHelpLabel->setStyleSheet("font-size: 11px; color: #666; font-style: italic;");
    rollbackTripLayout->addWidget(tripHelpLabel);
    rollbackTripLayout->addStretch();
    
    buttonLayout->addLayout(rollbackTripLayout);

    QPushButton *backButton = new QPushButton("Back to Home");
    backButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(200,200,200,0.8), stop:1 rgba(180,180,180,0.6));"
        "    color: #333;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 10px 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 rgba(220,220,220,0.9), stop:1 rgba(200,200,200,0.7));"
        "}"
    );
    backButton->setCursor(Qt::PointingHandCursor);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::goBackToHome);
    buttonLayout->addWidget(backButton);

    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    stackedWidget->addWidget(rollbackManagerPage);
}

void MainWindow::createAnalyticsPage()
{
    analyticsPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(analyticsPage);
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(8);

    // Compact top bar: badge + small actions
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setSpacing(8);
    QLabel *badge = new QLabel("Analytics");
    badge->setStyleSheet(
        "QLabel {"
        "  background: rgba(76,175,80,0.12);"
        "  color: #1B5E20;"
        "  border: 1px solid rgba(46,125,50,0.22);"
        "  border-radius: 10px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
    );
    topBar->addWidget(badge);
    topBar->addStretch();

    QPushButton *refreshButton = new QPushButton("Refresh");
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setStyleSheet(
        "QPushButton {"
        "  background: #2E7D32;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover { background: #388E3C; }"
    );
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshAnalytics);
    topBar->addWidget(refreshButton);

    QPushButton *backButton = new QPushButton("Home");
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(
        "QPushButton {"
        "  background: rgba(0,0,0,0.06);"
        "  color: #333;"
        "  border: 1px solid rgba(0,0,0,0.12);"
        "  border-radius: 8px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover { background: rgba(0,0,0,0.10); }"
    );
    connect(backButton, &QPushButton::clicked, this, &MainWindow::goBackToHome);
    topBar->addWidget(backButton);
    layout->addLayout(topBar);

    // Premium metric tiles (single row)
    QHBoxLayout *tilesLayout = new QHBoxLayout();
    tilesLayout->setSpacing(12);

    auto makeTile = [&](const QString &title, QLabel *&valueLabel, const QString &bg1, const QString &bg2, const QString &emoji) {
        QFrame *tile = new QFrame();
        tile->setStyleSheet(QString(
            "QFrame {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
            "  border-radius: 14px;"
            "}"
        ).arg(bg1, bg2));
        QVBoxLayout *tl = new QVBoxLayout(tile);
        tl->setContentsMargins(14, 12, 14, 12);
        tl->setSpacing(6);
        QLabel *titleLabel = new QLabel(emoji + "  " + title);
        titleLabel->setStyleSheet("color: rgba(0,0,0,0.7); font-size: 12px; font-weight: 600;");
        valueLabel = new QLabel("0");
        valueLabel->setStyleSheet("color: #0B3D0B; font-size: 22px; font-weight: 800;");
        tl->addWidget(titleLabel);
        tl->addWidget(valueLabel);
        QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect();
        eff->setBlurRadius(18); eff->setXOffset(0); eff->setYOffset(6); eff->setColor(QColor(0,0,0,80));
        tile->setGraphicsEffect(eff);
        return tile;
    };

    QWidget *tileTrips = makeTile("Total Trips", tileTripsLabel, "#E8F5E9", "#FFFFFF", "📊");
    QWidget *tileCompleted = makeTile("Completed", tileCompletedLabel, "#D0F0D5", "#FFFFFF", "✅");
    QWidget *tileCancelled = makeTile("Cancelled", tileCancelledLabel, "#FDE2E2", "#FFFFFF", "❌");
    QWidget *tileDistance = makeTile("Distance (m)", tileDistanceLabel, "#E3F2FD", "#FFFFFF", "📏");
    QWidget *tileSpend = makeTile("Rider Spend (Rs)", tileSpendLabel, "#FFF3E0", "#FFFFFF", "💸");

    tilesLayout->addWidget(tileTrips);
    tilesLayout->addWidget(tileCompleted);
    tilesLayout->addWidget(tileCancelled);
    tilesLayout->addWidget(tileDistance);
    tilesLayout->addWidget(tileSpend);
    tilesLayout->addStretch(1);
    layout->addLayout(tilesLayout);

    // Secondary text summary under tiles
    analyticsSummaryLabel = new QLabel("Trips: 0  |  Completed: 0  |  Cancelled: 0\nTotal rider spend: Rs. 0.00\nTotal distance covered: 0.0 m");
    analyticsSummaryLabel->setStyleSheet("font-size: 12px; color: #666; padding: 2px 2px;");
    analyticsSummaryLabel->setWordWrap(true);
    layout->addWidget(analyticsSummaryLabel);

    // Scrollable content for rider/driver breakdowns
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { width: 10px; background: #F0F0F0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #CCC; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #AAA; }"
    );
    QWidget *scrollContent = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setSpacing(14);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // Rider pane (header + section)
    QVBoxLayout *panesLayout = new QVBoxLayout();
    panesLayout->setSpacing(10);
    panesLayout->setContentsMargins(0, 0, 0, 0);

    // Place rider/driver panes side-by-side using a row layout
    QHBoxLayout *rowLayout = new QHBoxLayout();
    rowLayout->setSpacing(12);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *riderPane = new QWidget();
    QVBoxLayout *riderPaneLayout = new QVBoxLayout(riderPane);
    riderPaneLayout->setSpacing(6);
    riderPaneLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *riderHeader = new QLabel("🧑‍🤝‍🧑 Rider Analytics");
    riderHeader->setStyleSheet("font-size: 16px; font-weight: 800; color: #1B5E20;");
    riderPaneLayout->addWidget(riderHeader);

    QFrame *riderSection = new QFrame();
    riderSection->setStyleSheet(
        "background: #FDFDFD; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 10px;"
    );
    QVBoxLayout *riderSectionLayout = new QVBoxLayout(riderSection);
    riderSectionLayout->setSpacing(8);
    riderSectionLayout->setContentsMargins(10, 10, 10, 10);
    riderAnalyticsLayout = new QVBoxLayout();
    riderAnalyticsLayout->setSpacing(8);
    riderAnalyticsLayout->setContentsMargins(0, 0, 0, 0);
    riderSectionLayout->addLayout(riderAnalyticsLayout);
    riderPaneLayout->addWidget(riderSection);

    QWidget *driverPane = new QWidget();
    QVBoxLayout *driverPaneLayout = new QVBoxLayout(driverPane);
    driverPaneLayout->setSpacing(6);
    driverPaneLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *driverHeader = new QLabel("🚗 Driver Analytics");
    driverHeader->setStyleSheet("font-size: 16px; font-weight: 800; color: #1B5E20;");
    driverPaneLayout->addWidget(driverHeader);

    QFrame *driverSection = new QFrame();
    driverSection->setStyleSheet(
        "background: #FDFDFD; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 10px;"
    );
    QVBoxLayout *driverSectionLayout = new QVBoxLayout(driverSection);
    driverSectionLayout->setSpacing(8);
    driverSectionLayout->setContentsMargins(10, 10, 10, 10);
    driverAnalyticsLayout = new QVBoxLayout();
    driverAnalyticsLayout->setSpacing(8);
    driverAnalyticsLayout->setContentsMargins(0, 0, 0, 0);
    driverSectionLayout->addLayout(driverAnalyticsLayout);
    driverPaneLayout->addWidget(driverSection);

    riderPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    driverPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rowLayout->addWidget(riderPane, 1);
    rowLayout->addWidget(driverPane, 1);
    contentLayout->addLayout(rowLayout);
    contentLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea, 1);

    stackedWidget->addWidget(analyticsPage);

    // Build initial empty state
    refreshAnalytics();
}

void MainWindow::showAnalyticsPage()
{
    ensureCityLoaded();
    ensureDispatchEngine();
    refreshAnalytics();
    stackedWidget->setCurrentWidget(analyticsPage);
    ui->labelTitle->setVisible(false);
    qDebug() << "Showing Analytics page";
}

void MainWindow::refreshAnalytics()
{
    ensureCityLoaded();
    ensureDispatchEngine();

    if (!riderAnalyticsLayout || !driverAnalyticsLayout)
        return;

    // Utility to clear existing dynamic widgets
    auto clearLayout = [&](QLayout *layout, auto &&clearRef) -> void {
        if (!layout)
            return;
        while (layout->count() > 0) {
            QLayoutItem *item = layout->takeAt(0);
            if (item->layout()) {
                clearRef(item->layout(), clearRef);
                delete item->layout();
            }
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    };

    clearLayout(riderAnalyticsLayout, clearLayout);
    clearLayout(driverAnalyticsLayout, clearLayout);

    const QMap<QString, QVector<TripHistoryRecord>> &historyMap = RiderWindow::getSessionHistory();

    struct RiderStats {
        int total = 0;
        int completed = 0;
        int cancelled = 0;
        double distance = 0.0;
        double paid = 0.0;
    };

    struct DriverStats {
        int total = 0;
        int completed = 0;
        int cancelled = 0;
        double distance = 0.0;
        double earned = 0.0;
    };

    QMap<QString, RiderStats> riderStats;
    QMap<int, DriverStats> driverStats;

    int totalTrips = 0;
    int totalCompleted = 0;
    int totalCancelled = 0;
    double totalDistance = 0.0;
    double totalFare = 0.0;

    // Pre-populate riders with zeros so UI shows everyone upfront
    for (const Rider &r : std::as_const(riders)) {
        riderStats.insert(r.riderId, RiderStats());
    }

    // Pre-populate drivers with zeros if dispatch engine is ready
    if (sharedDispatchEngine) {
        for (int id = 1; id <= 100; ++id) {
            if (sharedDispatchEngine->getDriver(id)) {
                driverStats.insert(id, DriverStats());
            }
        }
    }

    // Fold in history values
    for (auto it = historyMap.constBegin(); it != historyMap.constEnd(); ++it) {
        const QString riderCode = it.key();
        for (const TripHistoryRecord &record : it.value()) {
            RiderStats &rs = riderStats[riderCode];
            rs.total++;
            totalTrips++;

            const QString status = QString::fromStdString(record.status).toUpper();
            const bool isCompleted = status == "COMPLETED";
            const bool isCancelled = status == "CANCELLED";

            if (isCompleted) {
                rs.completed++;
                totalCompleted++;
                rs.distance += record.distance;
                rs.paid += record.fare;
                totalDistance += record.distance;
                totalFare += record.fare;
            } else if (isCancelled) {
                rs.cancelled++;
                totalCancelled++;
            }

            if (record.driverId >= 0) {
                DriverStats &ds = driverStats[record.driverId];
                ds.total++;
                if (isCompleted) {
                    ds.completed++;
                    ds.distance += record.distance;
                    ds.earned += record.fare;
                } else if (isCancelled) {
                    ds.cancelled++;
                }
            }
        }
    }

    if (analyticsSummaryLabel) {
        analyticsSummaryLabel->setText(
            QString("Trips: %1  |  Completed: %2  |  Cancelled: %3\n"
                    "Total rider spend: Rs. %4\n"
                    "Total distance covered: %5 m")
                .arg(totalTrips)
                .arg(totalCompleted)
                .arg(totalCancelled)
                .arg(totalFare, 0, 'f', 2)
                .arg(totalDistance, 0, 'f', 1)
        );
    }

    // Update premium tiles
    if (tileTripsLabel) tileTripsLabel->setText(QString::number(totalTrips));
    if (tileCompletedLabel) tileCompletedLabel->setText(QString::number(totalCompleted));
    if (tileCancelledLabel) tileCancelledLabel->setText(QString::number(totalCancelled));
    if (tileDistanceLabel) tileDistanceLabel->setText(QString::number(totalDistance, 'f', 1));
    if (tileSpendLabel) tileSpendLabel->setText(QString::number(totalFare, 'f', 2));

    auto createCard = [](const QString &title, const QList<QPair<QString, QString>> &rows) -> QWidget * {
        QFrame *card = new QFrame();
        card->setStyleSheet(
            "QFrame {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFFFFF, stop:1 #FAFAFA);"
            "  border: 1px solid #E0E0E0;"
            "  border-radius: 10px;"
            "}"
        );
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(6);
        cardLayout->setContentsMargins(12, 10, 12, 10);

        QLabel *header = new QLabel(title);
        header->setStyleSheet("font-weight: 800; color: #1B5E20; font-size: 14px;");
        cardLayout->addWidget(header);

        QGridLayout *grid = new QGridLayout();
        grid->setHorizontalSpacing(16);
        grid->setVerticalSpacing(4);
        for (int i = 0; i < rows.size(); ++i) {
            QLabel *keyLabel = new QLabel(rows[i].first);
            keyLabel->setStyleSheet("color: #666; font-size: 12px; font-weight: 600;");
            QLabel *valueLabel = new QLabel(rows[i].second);
            valueLabel->setStyleSheet("color: #1B5E20; font-size: 13px; font-weight: 700;");
            grid->addWidget(keyLabel, i, 0);
            grid->addWidget(valueLabel, i, 1);
        }
        cardLayout->addLayout(grid);
        QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect();
        eff->setBlurRadius(16); eff->setXOffset(0); eff->setYOffset(4); eff->setColor(QColor(0,0,0,60));
        card->setGraphicsEffect(eff);
        return card;
    };

    for (auto it = riderStats.constBegin(); it != riderStats.constEnd(); ++it) {
        const RiderStats &rs = it.value();
        QList<QPair<QString, QString>> rows;
        rows.append(QPair<QString, QString>("Rides", QString("%1 (Completed %2 | Cancelled %3)").arg(rs.total).arg(rs.completed).arg(rs.cancelled)));
        rows.append(QPair<QString, QString>("Distance covered", QString("%1 m").arg(rs.distance, 0, 'f', 1)));
        rows.append(QPair<QString, QString>("Amount paid", QString("Rs. %1").arg(rs.paid, 0, 'f', 2)));
        riderAnalyticsLayout->addWidget(createCard(QString("Rider %1").arg(it.key()), rows));
    }
    riderAnalyticsLayout->addStretch();

    if (driverStats.isEmpty()) {
        QLabel *noDriverData = new QLabel("No drivers initialized yet.");
        noDriverData->setStyleSheet("color: #666; font-size: 13px;");
        driverAnalyticsLayout->addWidget(noDriverData);
    } else {
        for (auto it = driverStats.constBegin(); it != driverStats.constEnd(); ++it) {
            const DriverStats &ds = it.value();
            QList<QPair<QString, QString>> rows;
            rows.append(QPair<QString, QString>("Rides", QString("%1 (Completed %2 | Cancelled %3)").arg(ds.total).arg(ds.completed).arg(ds.cancelled)));
            rows.append(QPair<QString, QString>("Distance driven", QString("%1 m").arg(ds.distance, 0, 'f', 1)));
            rows.append(QPair<QString, QString>("Earnings", QString("Rs. %1").arg(ds.earned, 0, 'f', 2)));
            driverAnalyticsLayout->addWidget(createCard(QString("Driver %1").arg(it.key()), rows));
        }
        driverAnalyticsLayout->addStretch();
    }
}

void MainWindow::showRollbackManagerPage()
{
    ensureCityLoaded();
    ensureDispatchEngine();
    refreshSnapshotDisplay();
    stackedWidget->setCurrentWidget(rollbackManagerPage);
    ui->labelTitle->setVisible(false);
    qDebug() << "Showing Rollback Manager page";
}

void MainWindow::goBackToHome()
{
    stackedWidget->setCurrentWidget(homePage);
    ui->labelTitle->setVisible(true);
    qDebug() << "Returned to home page";
}

void MainWindow::refreshSnapshotDisplay()
{
    if (!sharedDispatchEngine) {
        qWarning() << "Dispatch engine not available";
        return;
    }

    // Get rollback manager from dispatch engine
    RollbackManager *rollbackMgr = sharedDispatchEngine->getRollbackManager();
    if (!rollbackMgr) {
        qWarning() << "Rollback manager not available";
        return;
    }

    // Get the scroll area from snapshotContainerLayout
    // It should be the second widget (index 1) after the title label
    QWidget *scrollAreaWidget = nullptr;
    for (int i = 0; i < snapshotContainerLayout->count(); ++i) {
        QLayoutItem *item = snapshotContainerLayout->itemAt(i);
        if (item && item->widget()) {
            QScrollArea *sa = qobject_cast<QScrollArea*>(item->widget());
            if (sa) {
                scrollAreaWidget = sa->widget();
                break;
            }
        }
    }

    if (!scrollAreaWidget) {
        qWarning() << "Could not find scroll area widget";
        return;
    }

    // Get the layout of the scroll content
    QVBoxLayout *scrollLayout = qobject_cast<QVBoxLayout*>(scrollAreaWidget->layout());
    if (!scrollLayout) {
        qWarning() << "Could not find scroll layout";
        return;
    }

    // Clear existing snapshot widgets
    while (scrollLayout->count() > 0) {
        QLayoutItem *item = scrollLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    int snapshotCount = rollbackMgr->getOperationCount();
    
    if (snapshotCount == 0) {
        QLabel *noSnapshotsLabel = new QLabel(
            "No snapshots recorded yet.\n\n"
            "Snapshots are created when:\n"
            "• Trips are assigned to drivers\n"
            "• Trips are cancelled\n"
            "• Trips are completed\n\n"
            "Each snapshot captures the state before an operation."
        );
        noSnapshotsLabel->setStyleSheet(
            "font-size: 13px; "
            "color: #999; "
            "padding: 20px; "
            "background: #F5F5F5; "
            "border-radius: 5px;"
        );
        noSnapshotsLabel->setAlignment(Qt::AlignCenter);
        scrollLayout->addWidget(noSnapshotsLabel);
        scrollLayout->addStretch();
        return;
    }

    // Display snapshot count
    QLabel *countLabel = new QLabel(QString("Total Snapshots: %1").arg(snapshotCount));
    countLabel->setStyleSheet(
        "font-size: 14px; "
        "font-weight: bold; "
        "color: #2E7D32; "
        "padding: 10px;"
    );
    scrollLayout->addWidget(countLabel);

    // Get the snapshot stack and display each snapshot
    OperationSnapshot *snapshot = rollbackMgr->getSnapshotStack();
    int snapshotIndex = 1;
    
    while (snapshot) {
        // Create snapshot card
        QWidget *snapshotCard = new QWidget();
        snapshotCard->setStyleSheet(
            "QWidget { "
            "background: #F9F9F9; "
            "border: 1px solid #E0E0E0; "
            "border-radius: 6px; "
            "padding: 12px; "
            "margin: 5px 0px; "
            "}"
        );
        QVBoxLayout *cardLayout = new QVBoxLayout(snapshotCard);
        cardLayout->setSpacing(6);
        cardLayout->setContentsMargins(12, 8, 12, 8);

        // Operation type
        QString opType;
        QString opColor = "#1B5E20";
        switch (snapshot->operationType) {
            case 0: opType = "Trip Assignment"; break;
            case 1: opType = "Trip Cancellation"; break;
            case 2: opType = "Trip Completion"; break;
            case 3: opType = "Rider Location Change"; opColor = "#1976D2"; break;
            case 4: opType = "Driver Availability Change"; opColor = "#F57C00"; break;
            case 10: opType = "Driver Added"; opColor = "#6D4C41"; break;
            case 11: opType = "Movement Step"; opColor = "#00796B"; break;
            case 20: opType = "Trip History Entry"; opColor = "#512DA8"; break;
            default: opType = "Unknown Operation";
        }

        QLabel *opLabel = new QLabel(QString("[Snapshot #%1] Operation: %2").arg(snapshotIndex).arg(opType));
        opLabel->setStyleSheet(QString("font-weight: bold; color: %1; font-size: 13px;").arg(opColor));
        cardLayout->addWidget(opLabel);

        // Trip ID
        if (snapshot->tripId >= 0) {
            QLabel *tripLabel = new QLabel(QString("  • Trip ID: %1").arg(snapshot->tripId));
            tripLabel->setStyleSheet("color: #333; font-size: 12px;");
            cardLayout->addWidget(tripLabel);
        }

        // Rider ID
        if (snapshot->riderId >= 0 || snapshot->riderCode[0] != '\0') {
            QString riderDisplay = (snapshot->riderCode[0] != '\0')
                ? QString::fromUtf8(snapshot->riderCode)
                : QString::number(snapshot->riderId);
            QLabel *riderLabel = new QLabel(QString("  • Rider: %1").arg(riderDisplay));
            riderLabel->setStyleSheet("color: #E91E63; font-size: 12px; font-weight: 600;");
            cardLayout->addWidget(riderLabel);
        }

        // Driver ID
        if (snapshot->driverId >= 0) {
            QLabel *driverLabel = new QLabel(QString("  • Driver ID: %1").arg(snapshot->driverId));
            driverLabel->setStyleSheet("color: #333; font-size: 12px;");
            cardLayout->addWidget(driverLabel);
        }

        // Trip History details
        if (snapshot->operationType == 20) {
            QString status = QString::fromUtf8(snapshot->details);
            QString pickup = QString::fromUtf8(snapshot->pickup);
            QString dropoff = QString::fromUtf8(snapshot->dropoff);
            QLabel *statusLabel = new QLabel(QString("  • Status: %1").arg(status));
            statusLabel->setStyleSheet("color: #512DA8; font-size: 12px; font-weight: 600;");
            cardLayout->addWidget(statusLabel);

            if (!pickup.isEmpty()) {
                QLabel *pickupLabel = new QLabel(QString("  • Pickup: %1").arg(pickup));
                pickupLabel->setStyleSheet("color: #333; font-size: 12px;");
                cardLayout->addWidget(pickupLabel);
            }
            if (!dropoff.isEmpty()) {
                QLabel *dropoffLabel = new QLabel(QString("  • Dropoff: %1").arg(dropoff));
                dropoffLabel->setStyleSheet("color: #333; font-size: 12px;");
                cardLayout->addWidget(dropoffLabel);
            }
            QLabel *fareLabel = new QLabel(QString("  • Fare: Rs. %1").arg(snapshot->fare, 0, 'f', 2));
            fareLabel->setStyleSheet("color: #2E7D32; font-size: 12px; font-weight: 600;");
            cardLayout->addWidget(fareLabel);
            QLabel *distLabel = new QLabel(QString("  • Distance: %1 m").arg(snapshot->distance, 0, 'f', 0));
            distLabel->setStyleSheet("color: #2E7D32; font-size: 12px; font-weight: 600;");
            cardLayout->addWidget(distLabel);
        }

        // Rider Location
        QString riderLoc = QString::fromUtf8(snapshot->riderLocation);
        if (!riderLoc.isEmpty()) {
            QLabel *riderLocationLabel = new QLabel(QString("  • Rider Location: %1").arg(riderLoc));
            riderLocationLabel->setStyleSheet("color: #E91E63; font-size: 12px; font-weight: 500;");
            cardLayout->addWidget(riderLocationLabel);
        }

        // Driver Location
        QString driverLoc = QString::fromUtf8(snapshot->driverLocation);
        if (!driverLoc.isEmpty()) {
            QLabel *locationLabel = new QLabel(QString("  • Driver Location: %1").arg(driverLoc));
            locationLabel->setStyleSheet("color: #1976D2; font-size: 12px; font-weight: 500;");
            cardLayout->addWidget(locationLabel);
        }

        // Previous state (omit for trip history entries)
        if (snapshot->operationType != 20) {
            QString stateStr;
            switch (snapshot->previousState) {
                case REQUESTED: stateStr = "Requested"; break;
                case ASSIGNED: stateStr = "Assigned"; break;
                case PICKUP_IN_PROGRESS: stateStr = "Pickup In Progress"; break;
                case ONGOING: stateStr = "Ongoing"; break;
                case COMPLETED: stateStr = "Completed"; break;
                case CANCELLED: stateStr = "Cancelled"; break;
                default: stateStr = "Unknown";
            }
            QLabel *stateLabel = new QLabel(QString("  • Previous State: %1").arg(stateStr));
            stateLabel->setStyleSheet("color: #555; font-size: 12px;");
            cardLayout->addWidget(stateLabel);
        }

        // Driver availability (only show for non-rider-location-change and non-history operations)
        if (snapshot->operationType != 3 && snapshot->operationType != 20) {
            QString availStr = snapshot->driverWasAvailable ? "Available" : "Unavailable";
            QLabel *availLabel = new QLabel(QString("  • Driver Was: %1").arg(availStr));
            availLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                .arg(snapshot->driverWasAvailable ? "#2E7D32" : "#D32F2F"));
            cardLayout->addWidget(availLabel);

            if (snapshot->operationType == 4) {
                QString newAvailStr = snapshot->driverNewAvailable ? "Available" : "Unavailable";
                QLabel *newAvailLabel = new QLabel(QString("  • Driver Now: %1").arg(newAvailStr));
                newAvailLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;")
                    .arg(snapshot->driverNewAvailable ? "#2E7D32" : "#D32F2F"));
                cardLayout->addWidget(newAvailLabel);
            }
        }

        scrollLayout->addWidget(snapshotCard);

        snapshot = snapshot->next;
        snapshotIndex++;
    }

    scrollLayout->addStretch();
    qDebug() << "Displayed" << snapshotCount << "snapshots";
}
