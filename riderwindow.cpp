#include "riderwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPixmap>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QFrame>
#include <QEvent>
#include <QPropertyAnimation>
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include <QMouseEvent>
#include <QDialog>
#include <QMessageBox>
#include "citymapview.h"
#include <algorithm>
#include <ctime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "core/rollbackmanager.h"

// Define static session-wide history store
QMap<QString, QVector<TripHistoryRecord>> RiderWindow::s_sessionHistory;

RiderWindow::RiderWindow(const QString &riderId, const QString &locationId, QWidget *parent)
        : QWidget(parent), riderId(riderId), locationId(locationId), dropoffNodeId(""),
            cityGraph(nullptr), cityLoaded(false), dispatchEngine(nullptr), driversInitialized(false),
            nextTripId(1), tripTimer(new QTimer(this)), tripStatusLabel(nullptr), driverStatusLabel(nullptr),
            usingSharedResources(false), retryTimer(new QTimer(this)), retryCount(0), maxRetries(5),
            restoredTripWidget(nullptr), cancelRideButton(nullptr), currentTripId(-1)
{
    // Load any existing session history for this rider before building UI
    if (RiderWindow::s_sessionHistory.contains(this->riderId)) {
        tripHistory = RiderWindow::s_sessionHistory.value(this->riderId);
    }

    loadLocationData();
    loadStreetNodes();
    setupUI();
    
    // Setup auto-retry timer for driver requests
    connect(retryTimer, &QTimer::timeout, this, &RiderWindow::retryRequestRide);
}

RiderWindow::~RiderWindow()
{
    // Do not clear session history here; it's memory-only and will reset on app exit
    
    // Only delete if we own these resources
    if (!usingSharedResources)
    {
        delete cityGraph;
        delete dispatchEngine;
    }
}

void RiderWindow::setSharedResources(City *city, DispatchEngine *engine)
{
    // Use shared resources instead of creating new ones
    cityGraph = city;
    dispatchEngine = engine;
    cityLoaded = (city != nullptr);
    driversInitialized = true; // Drivers already initialized in MainWindow
    usingSharedResources = true;
    
    qDebug() << "RiderWindow using shared City and DispatchEngine";
}

void RiderWindow::setupUI()
{
    setWindowTitle("Rider Dashboard");
    setMinimumSize(720, 650);
    
    // Match app theme
    setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #FFFFFF, stop:1 #E8F5E9);"
        "}"
    );
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Header
    QWidget *header = new QWidget();
    header->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 rgba(255,255,255,0.85), stop:1 rgba(255,255,255,0.65));"
        "border-bottom: 1px solid rgba(46,125,50,0.15);"
    );
    header->setFixedHeight(70);
    
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(30, 15, 30, 15);
    
    // Back button
    backButton = new QPushButton("Back", header);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(
        "QPushButton {"
        "  background: rgba(255,255,255,0.8);"
        "  color: #2E7D32;"
        "  border: 1px solid rgba(46,125,50,0.25);"
        "  border-radius: 10px;"
        "  padding: 10px 14px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
        "  border: 1px solid rgba(76,175,80,0.4);"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(76,175,80,0.2);"
        "}"
    );
    connect(backButton, &QPushButton::clicked, this, [this]() {
        emit backRequested();
    });

    mapButton = new QPushButton("Map", header);
    mapButton->setCursor(Qt::PointingHandCursor);
    mapButton->setStyleSheet(
        "QPushButton {"
        "  background: #2E7D32;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 10px 18px;"
        "  font-size: 14px;"
        "  font-weight: 700;"
        "  letter-spacing: 0.3px;"
        "}"
        "QPushButton:hover {"
        "  background: #388E3C;"
        "}"
        "QPushButton:pressed {"
        "  background: #1B5E20;"
        "}"
    );
    connect(mapButton, &QPushButton::clicked, this, &RiderWindow::onMapButtonClick);

    headerLayout->addWidget(backButton, 0, Qt::AlignLeft);
    headerLayout->addWidget(mapButton, 0, Qt::AlignLeft);
    headerLayout->addStretch(1);
    
    // Welcome message (center)
    welcomeLabel = new QLabel(QString("Welcome, %1 !").arg(riderId), header);
    welcomeLabel->setStyleSheet(
        "color: #1B5E20;"
        "font-size: 24px;"
        "font-weight: 500;"
        "font-family: 'Segoe UI', Arial, sans-serif;"
        "letter-spacing: 0.5px;"
        "background: transparent;"
    );
    welcomeLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(welcomeLabel, 2);
    
    // Right spacer before profile
    headerLayout->addStretch(1);
    
    // Profile image container with wrapper for better styling
    QWidget *profileContainer = new QWidget(header);
    profileContainer->setFixedSize(56, 56);
    profileContainer->setStyleSheet("background: transparent;");
    
    QVBoxLayout *profileContainerLayout = new QVBoxLayout(profileContainer);
    profileContainerLayout->setContentsMargins(0, 0, 0, 0);
    
    // Profile image (right corner)
    profileImageLabel = new QLabel(profileContainer);
    profileImageLabel->setAlignment(Qt::AlignCenter);
    profileImageLabel->setFixedSize(56, 56);
    profileImageLabel->setStyleSheet(
        "border: 3px solid qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 rgba(76,175,80,0.6), stop:1 rgba(46,125,50,0.4));"
        "border-radius: 28px;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 #FFFFFF, stop:1 #F1F8F4);"
        "padding: 2px;"
    );
    
    // Load user profile image
    QString imgPath = QCoreApplication::applicationDirPath() + "/user1.png";
    QFileInfo imgInfo(imgPath);
    
    if (!imgInfo.exists()) {
        QDir exeDir(QCoreApplication::applicationDirPath());
        exeDir.cdUp();
        exeDir.cdUp();
        imgPath = exeDir.absoluteFilePath("user1.png");
        imgInfo.setFile(imgPath);
    }
    
    if (imgInfo.exists()) {
        QPixmap pixmap(imgPath);
        if (!pixmap.isNull()) {
            // Create rounded pixmap
            QPixmap rounded(56, 56);
            rounded.fill(Qt::transparent);
            QPainter painter(&rounded);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            
            QPainterPath path;
            path.addEllipse(0, 0, 56, 56);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, 56, 56, pixmap);
            
            profileImageLabel->setPixmap(rounded);
        }
    } else {
        qDebug() << "user1.png not found";
        profileImageLabel->setText("👤");
        profileImageLabel->setStyleSheet(
            profileImageLabel->styleSheet() + 
            "font-size: 28px; color: #2E7D32;"
        );
    }
    
    // Add shadow effect to profile image
    QGraphicsDropShadowEffect *profileShadow = new QGraphicsDropShadowEffect();
    profileShadow->setBlurRadius(15);
    profileShadow->setXOffset(0);
    profileShadow->setYOffset(2);
    profileShadow->setColor(QColor(0, 0, 0, 60));
    profileImageLabel->setGraphicsEffect(profileShadow);
    
    profileContainerLayout->addWidget(profileImageLabel);
    headerLayout->addWidget(profileContainer);
    
    mainLayout->addWidget(header);
    
    // Content area with sidebar
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("background: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    
    // Sidebar
    createSidebar(contentArea);
    
    // Main content area (stacked widget for different views)
    contentStack = new QStackedWidget();
    contentStack->setStyleSheet("background: transparent;");
    
    // Book Ride Page with location selection interface
    QWidget *bookRidePage = createBookRidePage();
    
    // History Page
    QWidget *historyPage = createHistoryPage();
    
    contentStack->addWidget(bookRidePage);
    contentStack->addWidget(historyPage);
    
    contentLayout->addWidget(contentStack, 1);
    
    mainLayout->addWidget(contentArea, 1);
}

void RiderWindow::createSidebar(QWidget *parent)
{
    QWidget *sidebar = new QWidget(parent);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet(
        "QWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 rgba(255,255,255,0.9), stop:1 rgba(232,245,233,0.7));"
        "  border-right: 1px solid rgba(46,125,50,0.15);"
        "}"
    );
    
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 20, 15, 20);
    sidebarLayout->setSpacing(10);
    
    // Book Ride button
    bookRideButton = new QPushButton("📍  Book Ride", sidebar);
    bookRideButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #4CAF50, stop:1 #45A049);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 16px 18px;"
        "  font-size: 15px;"
        "  font-weight: 600;"
        "  text-align: left;"
        "  letter-spacing: 0.3px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
        "  color: #2E7D32;"
        "  border: 1px solid rgba(76,175,80,0.4);"
        "  backdrop-filter: blur(10px);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #388E3C, stop:1 #2E7D32);"
        "}"
    );
    bookRideButton->setCursor(Qt::PointingHandCursor);
    
    // History button
    historyButton = new QPushButton("📜  History", sidebar);
    historyButton->setStyleSheet(
        "QPushButton {"
        "  background: rgba(255,255,255,0.6);"
        "  color: #2E7D32;"
        "  border: 1px solid rgba(46,125,50,0.2);"
        "  border-radius: 12px;"
        "  padding: 16px 18px;"
        "  font-size: 15px;"
        "  font-weight: 600;"
        "  text-align: left;"
        "  letter-spacing: 0.3px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
        "  border: 1px solid rgba(76,175,80,0.4);"
        "  color: #1B5E20;"
        "  backdrop-filter: blur(10px);"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(76,175,80,0.25);"
        "}"
    );
    historyButton->setCursor(Qt::PointingHandCursor);
    
    sidebarLayout->addWidget(bookRideButton);
    sidebarLayout->addWidget(historyButton);
    sidebarLayout->addStretch();
    
    // Connect buttons to switch pages
    connect(bookRideButton, &QPushButton::clicked, [this]() {
        contentStack->setCurrentIndex(0);
        bookRideButton->setStyleSheet(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "    stop:0 #4CAF50, stop:1 #45A049);"
            "  color: white;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 16px 18px;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  text-align: left;"
            "  letter-spacing: 0.3px;"
            "}"
        );
        historyButton->setStyleSheet(
            "QPushButton {"
            "  background: rgba(255,255,255,0.6);"
            "  color: #2E7D32;"
            "  border: 1px solid rgba(46,125,50,0.2);"
            "  border-radius: 12px;"
            "  padding: 16px 18px;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  text-align: left;"
            "  letter-spacing: 0.3px;"
            "}"
            "QPushButton:hover {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
            "  border: 1px solid rgba(76,175,80,0.4);"
            "  color: #1B5E20;"
            "  backdrop-filter: blur(10px);"
            "}"
        );
    });
    
    connect(historyButton, &QPushButton::clicked, [this]() {
        contentStack->setCurrentIndex(1);
        historyButton->setStyleSheet(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "    stop:0 #4CAF50, stop:1 #45A049);"
            "  color: white;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 16px 18px;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  text-align: left;"
            "  letter-spacing: 0.3px;"
            "}"
        );
        bookRideButton->setStyleSheet(
            "QPushButton {"
            "  background: rgba(255,255,255,0.6);"
            "  color: #2E7D32;"
            "  border: 1px solid rgba(46,125,50,0.2);"
            "  border-radius: 12px;"
            "  padding: 16px 18px;"
            "  font-size: 15px;"
            "  font-weight: 600;"
            "  text-align: left;"
            "  letter-spacing: 0.3px;"
            "}"
            "QPushButton:hover {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
            "  border: 1px solid rgba(76,175,80,0.4);"
            "  color: #1B5E20;"
            "  backdrop-filter: blur(10px);"
            "}"
        );
    });
    
    parent->layout()->addWidget(sidebar);
}

QWidget* RiderWindow::createBookRidePage()
{
    QWidget *page = new QWidget();
    page->setStyleSheet("background: transparent;");
    
    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea(page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QScrollBar:vertical {"
        "  background: rgba(46,125,50,0.05);"
        "  width: 10px;"
        "  border-radius: 5px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(76,175,80,0.4);"
        "  border-radius: 5px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(76,175,80,0.6);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"
    );
    
    // Content widget inside scroll area
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    scrollContent->setMaximumWidth(650);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(20);
    
    // ===== SECTION 1: Current Location Display Card =====
    QWidget *currentLocationCard = new QWidget();
    currentLocationCard->setMinimumHeight(100);
    currentLocationCard->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 rgba(76,175,80,0.15), stop:1 rgba(46,125,50,0.1));"
        "border: 2px solid rgba(46,125,50,0.3);"
        "border-radius: 14px;"
        "padding: 20px 22px;"
    );
    QVBoxLayout *locCardLayout = new QVBoxLayout(currentLocationCard);
    locCardLayout->setContentsMargins(0, 0, 0, 0);
    locCardLayout->setSpacing(10);
    
    QLabel *locTitle = new QLabel("📍 Your Current Location");
    locTitle->setStyleSheet(
        "color: #1B5E20;"
        "font-size: 14px;"
        "font-weight: 700;"
        "letter-spacing: 0.5px;"
        "background: transparent;"
    );
    
    currentLocationLabel = new QLabel(locationId);
    currentLocationLabel->setMinimumHeight(50);
    currentLocationLabel->setStyleSheet(
        "color: #2E7D32;"
        "font-size: 17px;"
        "font-weight: 600;"
        "background: rgba(255,255,255,0.5);"
        "border-radius: 10px;"
        "padding: 12px 14px;"
        "border: 1px solid rgba(76,175,80,0.2);"
    );
    currentLocationLabel->setWordWrap(true);
    
    locCardLayout->addWidget(locTitle);
    locCardLayout->addWidget(currentLocationLabel);
    mainLayout->addWidget(currentLocationCard);
    
    // ===== SECTION 2: Map Preview Card =====
    QWidget *mapCard = new QWidget();
    mapCard->setMinimumHeight(220);
    mapCard->setMaximumHeight(220);
    mapCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mapCard->setStyleSheet(
        "background: white;"
        "border: 2px solid rgba(76,175,80,0.2);"
        "border-radius: 16px;"
    );
    
    QVBoxLayout *mapCardLayout = new QVBoxLayout(mapCard);
    mapCardLayout->setContentsMargins(4, 4, 4, 4);
    
    // Map image
    QLabel *mapImage = new QLabel();
    this->mapImage = mapImage;
    mapImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapImage->setAttribute(Qt::WA_Hover);
    mapImage->installEventFilter(this);
    mapImage->setCursor(Qt::PointingHandCursor);
    mapImage->setStyleSheet(
        "border: none;"
        "background: transparent;"
        "border-radius: 14px;"
    );
    mapImage->setProperty("clickable", "map");
    
    QString mapPath = QCoreApplication::applicationDirPath() + "/Group23.png";
    QFileInfo mapInfo(mapPath);
    
    if (!mapInfo.exists()) {
        QDir exeDir(QCoreApplication::applicationDirPath());
        exeDir.cdUp();
        exeDir.cdUp();
        mapPath = exeDir.absoluteFilePath("Group23.png");
        mapInfo.setFile(mapPath);
    }
    
    if (mapInfo.exists()) {
        QPixmap mapPixmap(mapPath);
        if (!mapPixmap.isNull()) {
            mapImage->setPixmap(mapPixmap);
            mapImage->setScaledContents(true);
        }
    } else {
        mapImage->setText("🗺️ Map Preview");
        mapImage->setStyleSheet(
            "color: #666;"
            "font-size: 18px;"
            "background: #F5F5F5;"
            "border: none;"
            "border-radius: 14px;"
        );
    }
    
    // Expand icon overlay
    QLabel *expandIcon = new QLabel(mapCard);
    expandIcon->setText("⛶");
    expandIcon->setStyleSheet(
        "background: rgba(255,255,255,0.9);"
        "color: #2E7D32;"
        "border: 1px solid rgba(46,125,50,0.2);"
        "border-radius: 8px;"
        "padding: 8px;"
        "font-size: 16px;"
    );
    expandIcon->setFixedSize(36, 36);
    expandIcon->move(mapCard->width() - 50, mapCard->height() - 50);
    expandIcon->raise();
    
    mapCardLayout->addWidget(mapImage);
    
    mainLayout->addWidget(mapCard);
    
    // ===== SECTION 3: Destination Selection =====
    QWidget *pickupSection = new QWidget();
    pickupSection->setStyleSheet("background: transparent;");
    QVBoxLayout *pickupLayout = new QVBoxLayout(pickupSection);
    pickupLayout->setContentsMargins(0, 10, 0, 0);
    pickupLayout->setSpacing(14);
    
    // ===== Destination Input Field =====
    QLabel *destLabel = new QLabel("Select Dropoff Location");
    destLabel->setStyleSheet(
        "color: #1B5E20;"
        "font-size: 15px;"
        "font-weight: 700;"
        "background: transparent;"
        "margin-top: 8px;"
    );
    pickupLayout->addWidget(destLabel);
    
    QWidget *destInput = new QWidget();
    destInput->setMinimumHeight(60);
    destInput->setStyleSheet(
        "background: rgba(255,255,255,0.8);"
        "border: 1px solid rgba(76,175,80,0.3);"
        "border-radius: 14px;"
        "padding: 16px 18px;"
    );
    destInput->setCursor(Qt::PointingHandCursor);
    
    QHBoxLayout *destLayout = new QHBoxLayout(destInput);
    destLayout->setContentsMargins(0, 0, 0, 0);
    destLayout->setSpacing(14);
    
    QLabel *destIcon = new QLabel("📍");
    destIcon->setStyleSheet(
        "font-size: 20px;"
        "background: transparent;"
        "border: none;"
    );
    
    QLabel *destPlaceholder = new QLabel("Where do you wanna go?");
    this->destPlaceholder = destPlaceholder;
    destPlaceholder->setStyleSheet(
        "color: rgba(46,125,50,0.6);"
        "font-size: 16px;"
        "font-weight: 500;"
        "background: transparent;"
        "border: none;"
    );
    
    destLayout->addWidget(destIcon);
    destLayout->addWidget(destPlaceholder, 1);
    
    QPushButton *pickFromMapBtn = new QPushButton("Pick on Map");
    pickFromMapBtn->setCursor(Qt::PointingHandCursor);
    pickFromMapBtn->setMinimumHeight(38);
    pickFromMapBtn->setStyleSheet(
        "QPushButton {"
        "  background: #7e57c2;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 10px 16px;"
        "  font-weight: 600;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background: #6a46b0; }"
        "QPushButton:pressed { background: #5c3c9c; }"
    );
    connect(pickFromMapBtn, &QPushButton::clicked, this, &RiderWindow::onPickFromMapClicked);
    destLayout->addWidget(pickFromMapBtn);
    destLayout->addStretch();
    
    // Make destination input clickable
    destInput->setProperty("clickable", "destination");
    destInput->installEventFilter(this);
    
    pickupLayout->addWidget(destInput);
    mainLayout->addWidget(pickupSection);
    
    // Divider
    mainLayout->addSpacing(10);
    QFrame *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet(
        "background: rgba(46,125,50,0.15);"
        "border: none;"
        "max-height: 1px;"
    );
    
    mainLayout->addWidget(divider);
    mainLayout->addSpacing(10);
    
    // ===== Request Ride Button =====
    requestRideButton = new QPushButton("Request Ride");
    requestRideButton->setCursor(Qt::PointingHandCursor);
    requestRideButton->setMinimumHeight(55);
    requestRideButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #4CAF50, stop:1 #2e7d32);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 18px 20px;"
        "  font-size: 17px;"
        "  font-weight: 700;"
        "  letter-spacing: 0.5px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #66BB6A, stop:1 #388E3C);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #2E7D32, stop:1 #1B5E20);"
        "}"
    );
    connect(requestRideButton, &QPushButton::clicked, this, &RiderWindow::onRequestRideClicked);
    mainLayout->addWidget(requestRideButton);

    // ===== Trip Status Panel =====
    QWidget *tripStatusPanel = new QWidget();
    tripStatusPanel->setMinimumHeight(80);
    tripStatusPanel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 rgba(76,175,80,0.08), stop:1 rgba(46,125,50,0.05));"
        "border: 1px solid rgba(46,125,50,0.2);"
        "border-radius: 12px;"
        "padding: 16px 18px;"
    );
    QVBoxLayout *tripLayout = new QVBoxLayout(tripStatusPanel);
    tripLayout->setContentsMargins(0,0,0,0);
    tripLayout->setSpacing(10);

    tripStatusLabel = new QLabel("No active trip");
    tripStatusLabel->setStyleSheet(
        "color: #1B5E20; "
        "font-size: 16px; "
        "font-weight: 600;"
    );
    driverStatusLabel = new QLabel("");
    driverStatusLabel->setStyleSheet(
        "color: #2E7D32; "
        "font-size: 14px; "
        "font-weight: 500;"
    );

    tripLayout->addWidget(tripStatusLabel);
    tripLayout->addWidget(driverStatusLabel);
    
    // Cancel ride button (initially hidden)
    cancelRideButton = new QPushButton("🚫 Cancel Ride");
    cancelRideButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 rgba(244,67,54,0.8), stop:1 rgba(211,47,47,0.7));"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "  margin-top: 8px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 rgba(229,57,53,0.9), stop:1 rgba(198,40,40,0.8));"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 rgba(211,47,47,1.0), stop:1 rgba(183,28,28,0.9));"
        "}"
    );
    cancelRideButton->setCursor(Qt::PointingHandCursor);
    cancelRideButton->setVisible(false);  // Hidden by default
    connect(cancelRideButton, &QPushButton::clicked, this, [this]() {
        if (!dispatchEngine) return;
        
        Trip *trip = dispatchEngine->getTrip(currentTripId);
        if (!trip) return;
        
        TripState state = trip->getState();
        if (state != PICKUP_IN_PROGRESS) {
            QMessageBox::warning(this, "Cannot Cancel", 
                "Ride can only be cancelled while driver is enroute to pickup.");
            return;
        }
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Cancel Ride",
            "Are you sure you want to cancel this ride?\n\nDriver is currently on the way to pick you up.",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            // Cancel the trip
            dispatchEngine->cancelTrip(currentTripId);
            
            // Add to history as cancelled
            QString pickup = QString::fromUtf8(trip->getPickupNodeId());
            QString dropoff = QString::fromUtf8(trip->getDropoffNodeId());
            double fare = trip->calculateTotalFare();
            double distance = trip->getRideDistance();
            int driverId = trip->getDriverId();
            
            addTripToHistory(currentTripId, pickup, dropoff, "CANCELLED", fare, distance, driverId);
            refreshHistoryPage();
            
            // Stop trip timer and update UI
            if (tripTimer->isActive()) {
                tripTimer->stop();
            }
            
            tripStatusLabel->setText("No active trip");
            driverStatusLabel->setText("");
            cancelRideButton->setVisible(false);
            
            // Reset destination
            this->destPlaceholder->setText("Where do you wanna go?");
            this->destPlaceholder->setStyleSheet(
                "color: rgba(46,125,50,0.5);"
                "font-size: 15px;"
                "font-weight: 500;"
                "background: transparent;"
                "border: none;"
            );
            dropoffNodeId = "";
            rejectedDriverIds.clear();
            
            QMessageBox::information(this, "Ride Cancelled", 
                "Your ride has been cancelled. The driver has been notified.");
        }
    });
    tripLayout->addWidget(cancelRideButton);
    
    mainLayout->addWidget(tripStatusPanel);
    mainLayout->addStretch();
    
    // Set scroll content and add to page
    scrollArea->setWidget(scrollContent);
    
    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setAlignment(Qt::AlignHCenter);
    pageLayout->addWidget(scrollArea);
    
    return page;
}

QWidget* RiderWindow::createRestoredTripWidget(const QString &pickup, const QString &dropoff, 
                                               double fare, double distance, int tripId)
{
    QWidget *card = new QWidget();
    card->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 rgba(255,152,0,0.2), stop:1 rgba(255,87,34,0.15));"
        "border: 2px solid #FF9800;"
        "border-radius: 14px;"
        "padding: 20px;"
    );
    
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    
    // Title
    QLabel *titleLabel = new QLabel("⚠️ Trip Restored from Rollback");
    titleLabel->setStyleSheet(
        "color: #E65100;"
        "font-size: 15px;"
        "font-weight: 700;"
        "letter-spacing: 0.5px;"
    );
    layout->addWidget(titleLabel);
    
    // Trip details
    QLabel *detailsLabel = new QLabel(
        QString("Trip #%1\n\nPickup: %2\nDropoff: %3\nDistance: %4 km\nEstimated Fare: PKR %5")
            .arg(tripId).arg(pickup, dropoff).arg(distance, 0, 'f', 1).arg(fare, 0, 'f', 2)
    );
    detailsLabel->setStyleSheet(
        "color: #333;"
        "font-size: 13px;"
        "line-height: 1.6;"
        "padding: 10px;"
        "background: rgba(255,255,255,0.3);"
        "border-radius: 6px;"
    );
    layout->addWidget(detailsLabel);
    
    // Buttons layout
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    // Continue button
    QPushButton *continueBtn = new QPushButton("✓ Continue Trip");
    continueBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #4CAF50, stop:1 #45a049);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "    padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #45a049, stop:1 #3d8b40);"
        "}"
    );
    connect(continueBtn, &QPushButton::clicked, this, [this, tripId]() {
        qDebug() << "User chose to continue restored trip" << tripId;
        currentTripId = tripId;
        
        if (dispatchEngine) {
            Trip *trip = dispatchEngine->getTrip(tripId);
            if (trip) {
                startTripProgress(tripId);
            }
        }
        
        hideRestoredTripSection();
    });
    btnLayout->addWidget(continueBtn);
    
    // Cancel button
    QPushButton *cancelBtn = new QPushButton("✕ Cancel Trip");
    cancelBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #f44336, stop:1 #d32f2f);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "    padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #d32f2f, stop:1 #b71c1c);"
        "}"
    );
    connect(cancelBtn, &QPushButton::clicked, this, [this, tripId, pickup, dropoff, fare, distance]() {
        qDebug() << "User chose to cancel restored trip" << tripId;
        
        if (dispatchEngine) {
            Trip *trip = dispatchEngine->getTrip(tripId);
            if (trip) {
                int driverId = trip->getDriverId();
                
                // Cancel the trip through dispatch engine
                dispatchEngine->cancelTrip(tripId);
                
                // Add to history as CANCELLED
                addTripToHistory(tripId, pickup, dropoff, "CANCELLED", fare, distance, driverId);
                
                // Refresh the UI to show the cancelled trip in history
                refreshHistoryPage();
            }
        }
        
        hideRestoredTripSection();
    });
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    return card;
}

void RiderWindow::showRestoredTripSection(const QString &pickup, const QString &dropoff, 
                                         double fare, double distance, int tripId)
{
    qDebug() << "Showing restored trip section for trip" << tripId;
    
    // Store the info
    currentRestoredTrip.tripId = tripId;
    currentRestoredTrip.pickup = pickup;
    currentRestoredTrip.dropoff = dropoff;
    currentRestoredTrip.fare = fare;
    currentRestoredTrip.distance = distance;
    
    // Create and show the widget
    if (restoredTripWidget) {
        delete restoredTripWidget;
    }
    restoredTripWidget = createRestoredTripWidget(pickup, dropoff, fare, distance, tripId);
    
    // Show on the current page
    if (contentStack && contentStack->currentWidget()) {
        QWidget *currentPage = contentStack->currentWidget();
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(currentPage->layout());
        if (!layout) {
            // If no layout, create one
            layout = new QVBoxLayout(currentPage);
            currentPage->setLayout(layout);
        }
        
        // Insert at the top
        layout->insertWidget(0, restoredTripWidget, 0, Qt::AlignTop);
    }
}

void RiderWindow::hideRestoredTripSection()
{
    qDebug() << "Hiding restored trip section";
    if (restoredTripWidget) {
        restoredTripWidget->hide();
        restoredTripWidget->deleteLater();
        restoredTripWidget = nullptr;
    }
}

QWidget* RiderWindow::createHistoryPage()
{
    QWidget *page = new QWidget();
    page->setStyleSheet("background: transparent;");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(20);
    
    // Title
    QLabel *titleLabel = new QLabel("Trip History");
    titleLabel->setStyleSheet(
        "color: #1B5E20;"
        "font-size: 28px;"
        "font-weight: 700;"
        "letter-spacing: 0.5px;"
    );
    titleLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(titleLabel);
    
    // Scroll area for trip history
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QScrollBar:vertical {"
        "  background: rgba(255,255,255,0.3);"
        "  width: 10px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(76,175,80,0.5);"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(76,175,80,0.7);"
        "}"
    );
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout *historyLayout = new QVBoxLayout(scrollContent);
    historyLayout->setSpacing(15);
    historyLayout->setContentsMargins(0, 0, 10, 0);
    
    // Display trip history
    if (tripHistory.empty())
    {
        QLabel *emptyLabel = new QLabel("No trip history yet");
        emptyLabel->setStyleSheet(
            "color: rgba(46,125,50,0.6);"
            "font-size: 18px;"
            "font-weight: 500;"
            "padding: 40px;"
        );
        emptyLabel->setAlignment(Qt::AlignCenter);
        historyLayout->addWidget(emptyLabel);
    }
    else
    {
        // Display trips in reverse order (newest first)
        for (int i = tripHistory.size() - 1; i >= 0; --i)
        {
            const TripHistoryRecord &record = tripHistory[i];
            
            QWidget *tripCard = new QWidget();
            tripCard->setStyleSheet(
                "QWidget {"
                "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                "    stop:0 rgba(255,255,255,0.95), stop:1 rgba(255,255,255,0.85));"
                "  border: 1px solid rgba(76,175,80,0.2);"
                "  border-radius: 12px;"
                "}"
            );
            
            QVBoxLayout *cardLayout = new QVBoxLayout(tripCard);
            cardLayout->setContentsMargins(20, 15, 20, 15);
            cardLayout->setSpacing(10);
            
            // Trip ID and Status
            QHBoxLayout *headerLayout = new QHBoxLayout();
            QLabel *tripIdLabel = new QLabel(QString("Trip #%1").arg(record.tripId));
            tripIdLabel->setStyleSheet(
                "color: #1B5E20;"
                "font-size: 16px;"
                "font-weight: 700;"
            );
            
            QLabel *statusLabel = new QLabel(QString::fromStdString(record.status));
            QString statusColor = (record.status == "COMPLETED") ? "#4CAF50" : "#F44336";
            statusLabel->setStyleSheet(QString(
                "color: white;"
                "background: %1;"
                "font-size: 12px;"
                "font-weight: 600;"
                "padding: 5px 12px;"
                "border-radius: 10px;"
            ).arg(statusColor));
            
            headerLayout->addWidget(tripIdLabel);
            headerLayout->addStretch();
            headerLayout->addWidget(statusLabel);
            cardLayout->addLayout(headerLayout);
            
            // Pickup and Dropoff
            QLabel *routeLabel = new QLabel(QString(
                "📍 <b>From:</b> %1<br>"
                "📌 <b>To:</b> %2"
            ).arg(QString::fromStdString(record.pickupNode))
             .arg(QString::fromStdString(record.dropoffNode)));
            routeLabel->setStyleSheet(
                "color: #424242;"
                "font-size: 13px;"
                "line-height: 1.6;"
            );
            cardLayout->addWidget(routeLabel);
            
            // Details
            QLabel *detailsLabel = new QLabel(QString(
                "🚗 Driver: #%1 | 📏 Distance: %2 m | 💰 Fare: Rs. %3"
            ).arg(record.driverId)
             .arg(record.distance, 0, 'f', 0)
             .arg(record.fare, 0, 'f', 2));
            detailsLabel->setStyleSheet(
                "color: #757575;"
                "font-size: 12px;"
                "font-weight: 500;"
            );
            cardLayout->addWidget(detailsLabel);
            
            // Timestamp
            QLabel *timeLabel = new QLabel(QString("🕒 %1").arg(QString::fromStdString(record.timestamp)));
            timeLabel->setStyleSheet(
                "color: #9E9E9E;"
                "font-size: 11px;"
                "font-style: italic;"
            );
            cardLayout->addWidget(timeLabel);
            
            historyLayout->addWidget(tripCard);
        }
    }
    
    historyLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
    
    // Store reference to scroll content for dynamic updates
    page->setProperty("historyLayout", QVariant::fromValue((void*)historyLayout));
    
    return page;
}

bool RiderWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle mouse clicks on map and destination input
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QString clickable = obj->property("clickable").toString();
            if (clickable == "map") {
                onMapClick();
                return true;
            } else if (clickable == "destination") {
                onDestinationClick();
                return true;
            }
        }
    }
    
    // Handle hover effects for map
    QLabel *label = qobject_cast<QLabel*>(obj);
    if (label && label->cursor().shape() == Qt::PointingHandCursor) {
        if (event->type() == QEvent::HoverEnter) {
            // Add shadow effect
            QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
            shadow->setBlurRadius(20);
            shadow->setColor(QColor(46, 125, 50, 80));
            shadow->setOffset(0, 4);
            label->setGraphicsEffect(shadow);
            
            // Scale animation
            QPropertyAnimation *anim = new QPropertyAnimation(label, "geometry");
            anim->setDuration(250);
            QRect current = label->geometry();
            int margin = 10;
            QRect zoomed = current.adjusted(-margin, -margin, margin, margin);
            anim->setStartValue(current);
            anim->setEndValue(zoomed);
            anim->setEasingCurve(QEasingCurve::OutBack);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
            label->raise();
        } else if (event->type() == QEvent::HoverLeave) {
            // Remove shadow effect
            label->setGraphicsEffect(nullptr);
            
            // Scale back animation
            QPropertyAnimation *anim = new QPropertyAnimation(label, "geometry");
            anim->setDuration(250);
            QRect current = label->geometry();
            int margin = 10;
            QRect normal = current.adjusted(margin, margin, -margin, -margin);
            anim->setStartValue(current);
            anim->setEndValue(normal);
            anim->setEasingCurve(QEasingCurve::InBack);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void RiderWindow::loadLocationData()
{
    QString csvPath = QCoreApplication::applicationDirPath() + "/../../../city_locations_path_data/city-locations.csv";
    QFileInfo csvInfo(csvPath);
    
    if (!csvInfo.exists()) {
        QDir dir(QCoreApplication::applicationDirPath());
        dir.cdUp();
        dir.cdUp();
        csvPath = dir.absoluteFilePath("city_locations_path_data/city-locations.csv");
        csvInfo.setFile(csvPath);
    }
    
    if (!csvInfo.exists()) {
        qDebug() << "City locations CSV not found";
        return;
    }
    
    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open city locations file";
        return;
    }
    
    QTextStream in(&file);
    in.readLine(); // Skip header
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        // Simple CSV parsing - handle quoted fields
        QStringList fields;
        QString currentField;
        bool inQuotes = false;
        
        for (int i = 0; i < line.length(); i++) {
            QChar c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                fields.append(currentField.trimmed());
                currentField.clear();
            } else {
                currentField += c;
            }
        }
        fields.append(currentField.trimmed()); // Add last field
        
        if (fields.size() >= 8) {
            QString zoneName = fields[0].trimmed();
            QString colonyName = fields[1].trimmed();
            QString streetNo = fields[2].trimmed();
            QString locationName = fields[4].trimmed();
            QString locationType = fields[5].trimmed();
            QString locationId = fields[7].trimmed();
            
            // Extract zone number
            QString zone = zoneName.replace("zone", "").trimmed();
            
            LocationInfo loc;
            loc.id = locationId;
            loc.name = locationName;
            loc.type = locationType;
            
            // Initialize hierarchy: zone -> colony -> street -> locations
            if (!zoneData.contains(zone)) {
                zoneData[zone] = QMap<QString, QMap<QString, QList<LocationInfo>>>();
            }
            if (!zoneData[zone].contains(colonyName)) {
                zoneData[zone][colonyName] = QMap<QString, QList<LocationInfo>>();
            }
            if (!zoneData[zone][colonyName].contains(streetNo)) {
                zoneData[zone][colonyName][streetNo] = QList<LocationInfo>();
            }
            zoneData[zone][colonyName][streetNo].append(loc);
        }
    }
    
    file.close();
    qDebug() << "Loaded" << zoneData.keys().size() << "zones with location data";
    qDebug() << "Zone keys:" << zoneData.keys();
    for (const QString &zone : zoneData.keys()) {
        qDebug() << "  Zone" << zone << "has" << zoneData[zone].keys().size() << "colonies";
    }
}

void RiderWindow::loadStreetNodes()
{
    QString csvPath = QCoreApplication::applicationDirPath() + "/../../../city_locations_path_data/paths.csv";
    QFileInfo csvInfo(csvPath);
    
    if (!csvInfo.exists()) {
        QDir dir(QCoreApplication::applicationDirPath());
        dir.cdUp();
        dir.cdUp();
        csvPath = dir.absoluteFilePath("city_locations_path_data/paths.csv");
        csvInfo.setFile(csvPath);
    }
    
    if (!csvInfo.exists()) {
        qDebug() << "Paths CSV not found";
        return;
    }
    
    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open paths file";
        return;
    }
    
    QTextStream in(&file);
    in.readLine(); // Skip header
    
    QSet<QString> processedNodes;
    QSet<QString> processedHighway;
    QSet<QString> processedConnectors;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(',');
        
        if (fields.size() >= 6) {
            QString zoneName = fields[0].trimmed();
            QString colonyName = fields[1].trimmed();
            QString streetNo = fields[2].trimmed();
            QString nodeId = fields[5].trimmed();
            
            if (zoneName.contains("Highway", Qt::CaseInsensitive)) {
                if (!processedHighway.contains(nodeId)) {
                    highwayNodes.append(nodeId);
                    processedHighway.insert(nodeId);
                }
            } else if (colonyName.contains("Zone Connector", Qt::CaseInsensitive) || 
                       nodeId.contains("ZoneConnector", Qt::CaseInsensitive)) {
                if (!processedConnectors.contains(nodeId)) {
                    zoneConnectors.append(nodeId);
                    processedConnectors.insert(nodeId);
                }
            } else if (!zoneName.isEmpty() && zoneName != "No Zone") {
                QString zone = zoneName.replace("zone", "").trimmed();
                QString zoneColonyKey = zone + "_" + colonyName;
                
                if (!processedNodes.contains(nodeId)) {
                    if (!streetNodes.contains(zoneColonyKey)) {
                        streetNodes[zoneColonyKey] = QMap<QString, QStringList>();
                    }
                    if (!streetNodes[zoneColonyKey].contains(streetNo)) {
                        streetNodes[zoneColonyKey][streetNo] = QStringList();
                    }
                    streetNodes[zoneColonyKey][streetNo].append(nodeId);
                    processedNodes.insert(nodeId);
                }
            }
        }
    }
    
    file.close();
    highwayNodes.sort();
    zoneConnectors.sort();
    qDebug() << "Loaded street nodes," << highwayNodes.size() << "highway nodes," << zoneConnectors.size() << "zone connectors";
    qDebug() << "Zone-colony keys:" << streetNodes.keys();
    qDebug() << "Sample highway nodes:" << highwayNodes.mid(0, qMin(5, highwayNodes.size()));
    qDebug() << "Sample zone connectors:" << zoneConnectors.mid(0, qMin(5, zoneConnectors.size()));
}

void RiderWindow::onDestinationClick()
{
    showZoneSelection();
}

void RiderWindow::onMapClick()
{
    showMapDialog();
}

void RiderWindow::showZoneSelection()
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 14px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    // Add regular zones
    QStringList zones = zoneData.keys();
    zones.sort();
    for (const QString &zone : zones) {
        QAction *action = menu->addAction("🏘️ Zone " + zone);
        connect(action, &QAction::triggered, this, [this, zone]() {
            showColonySelection(zone);
        });
    }
    
    // Add separators and special zones
    if (!highwayNodes.isEmpty() || !zoneConnectors.isEmpty()) {
        menu->addSeparator();
    }
    
    if (!highwayNodes.isEmpty()) {
        QAction *highwayAction = menu->addAction("🛣️ Highway Zone");
        connect(highwayAction, &QAction::triggered, this, [this]() {
            showHighwayNodes();
        });
    }
    
    if (!zoneConnectors.isEmpty()) {
        QAction *connectorAction = menu->addAction("🔗 Zone Connectors");
        connect(connectorAction, &QAction::triggered, this, [this]() {
            showZoneConnectors();
        });
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RiderWindow::showColonySelection(const QString &zone)
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 14px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    if (!zoneData.contains(zone)) {
        menu->addAction("No colonies available")->setEnabled(false);
    } else {
        QStringList colonies = zoneData[zone].keys();
        colonies.sort();
        
        for (const QString &colony : colonies) {
            QAction *action = menu->addAction("🏙️ " + colony);
            connect(action, &QAction::triggered, this, [this, zone, colony]() {
                showStreetSelection(zone, colony);
            });
        }
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RiderWindow::showStreetSelection(const QString &zone, const QString &colony)
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 14px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    if (!zoneData.contains(zone) || !zoneData[zone].contains(colony)) {
        menu->addAction("No streets available")->setEnabled(false);
    } else {
        QStringList streets = zoneData[zone][colony].keys();
        
        // Sort streets numerically
        std::sort(streets.begin(), streets.end(), [](const QString &a, const QString &b) {
            return a.toInt() < b.toInt();
        });
        
        for (const QString &street : streets) {
            QAction *action = menu->addAction("🛤️ Street " + street);
            connect(action, &QAction::triggered, this, [this, zone, colony, street]() {
                showLocationSelection(zone, colony, street);
            });
        }
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RiderWindow::showLocationSelection(const QString &zone, const QString &colony, const QString &street)
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   max-height: 400px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 13px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    if (!zoneData.contains(zone) || !zoneData[zone].contains(colony) || !zoneData[zone][colony].contains(street)) {
        menu->addAction("No locations available")->setEnabled(false);
    } else {
        QList<LocationInfo> locations = zoneData[zone][colony][street];
        
        for (const LocationInfo &location : locations) {
            QString displayText;
            QString icon;
            
            if (location.type == "home") {
                icon = "🏠";
            } else if (location.type == "mall") {
                icon = "🏬";
            } else if (location.type == "hospital") {
                icon = "🏥";
            } else if (location.type == "park") {
                icon = "🌳";
            } else if (location.type == "school") {
                icon = "🏫";
            } else if (location.type == "restaurant") {
                icon = "🍴";
            } else {
                icon = "📍";
            }
            
            displayText = icon + " " + location.name;
            
            QAction *action = menu->addAction(displayText);
            connect(action, &QAction::triggered, this, [this, location]() {
                destPlaceholder->setText(location.name + " - " + location.id);
                destPlaceholder->setStyleSheet(
                    "color: #2E7D32;"
                    "font-size: 15px;"
                    "font-weight: 600;"
                    "background: transparent;"
                    "border: none;"
                );
                dropoffNodeId = location.id;
            });
        }
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RiderWindow::showHighwayNodes()
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   max-height: 400px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 13px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    QStringList nodes = highwayNodes;
    nodes.sort();
    
    for (const QString &node : nodes) {
        QAction *action = menu->addAction("🛣️ " + node);
        connect(action, &QAction::triggered, this, [this, node]() {
            destPlaceholder->setText(node);
            destPlaceholder->setStyleSheet(
                "color: #2E7D32;"
                "font-size: 15px;"
                "font-weight: 600;"
                "background: transparent;"
                "border: none;"
            );
            dropoffNodeId = node;
        });
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RiderWindow::showZoneConnectors()
{
    QMenu *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu {"
        "   background: white;"
        "   border: 2px solid rgba(76,175,80,0.3);"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   max-height: 400px;"
        "}"
        "QMenu::item {"
        "   padding: 10px 30px 10px 20px;"
        "   border-radius: 6px;"
        "   color: #2E7D32;"
        "   font-size: 13px;"
        "}"
        "QMenu::item:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 rgba(76,175,80,0.15), stop:1 rgba(76,175,80,0.05));"
        "}"
    );
    
    QStringList connectors = zoneConnectors;
    connectors.sort();
    
    for (const QString &connector : connectors) {
        QAction *action = menu->addAction("🔗 " + connector);
        connect(action, &QAction::triggered, this, [this, connector]() {
            destPlaceholder->setText(connector);
            destPlaceholder->setStyleSheet(
                "color: #2E7D32;"
                "font-size: 15px;"
                "font-weight: 600;"
                "background: transparent;"
                "border: none;"
            );
            dropoffNodeId = connector;
        });
    }
    
    menu->exec(QCursor::pos());
    menu->deleteLater();
}

bool RiderWindow::ensureCityLoaded()
{
    if (cityLoaded)
        return true;

    if (!cityGraph)
        cityGraph = new City();

    QString locationsPath = resolveDataFile("city-locations.csv");
    QString pathsPath = resolveDataFile("paths.csv");

    if (locationsPath.isEmpty() || pathsPath.isEmpty()) {
        QMessageBox::warning(this, "Data missing", "City data files could not be found.");
        return false;
    }

    bool okLoc = cityGraph->loadLocations(locationsPath.toUtf8().constData());
    bool okPath = cityGraph->loadPaths(pathsPath.toUtf8().constData());
    cityLoaded = okLoc && okPath;

    if (!cityLoaded) {
        QMessageBox::warning(this, "Load error", "Failed to load city graph data.");
    }
    return cityLoaded;
}

bool RiderWindow::ensureDispatchEngine()
{
    if (!ensureCityLoaded())
        return false;
    if (!dispatchEngine)
        dispatchEngine = new DispatchEngine(cityGraph, 100, 200);
    if (!driversInitialized)
    {
        initializeDrivers();
        driversInitialized = true;
    }
    return true;
}

void RiderWindow::initializeDrivers()
{
    if (!dispatchEngine || !cityGraph)
        return;

    QHash<QString, QList<QString>> zoneStreetNodes;
    for (Node *n = cityGraph->getFirstNode(); n != nullptr; n = n->next)
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
            dispatchEngine->addDriver(driverId, nodeId.toUtf8().constData(), it.key().toUtf8().constData());
            driverId++;
            placed++;
        }
    }
}

QString RiderWindow::resolveDataFile(const QString &fileName) const
{
    QString path = QCoreApplication::applicationDirPath() + "/" + fileName;
    if (QFileInfo::exists(path)) return path;

    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cdUp();
    QString candidate = dir.filePath(fileName);
    if (QFileInfo::exists(candidate)) return candidate;

    QString dataCandidate = dir.filePath("city_locations_path_data/" + fileName);
    if (QFileInfo::exists(dataCandidate)) return dataCandidate;

    return QString();
}

void RiderWindow::showMapDialog()
{
    if (!ensureCityLoaded()) return;

    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("City Map");
    dialog->resize(1000, 800);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    CityMapView *view = new CityMapView(cityGraph, dialog);
    
    // Set user's current location to show purple marker
    view->setUserLocation(locationId);
    
    layout->addWidget(view);

    dialog->show();
}

void RiderWindow::onPickFromMapClicked()
{
    if (!ensureCityLoaded()) return;

    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Select Dropoff on Map");
    dialog->resize(1000, 800);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    CityMapView *view = new CityMapView(cityGraph, dialog);
    view->setUserLocation(locationId);
    view->enterSelectionMode();

    connect(view, &CityMapView::locationPicked, this, [this, dialog](const QString &locId, const QString &locName){
        QMessageBox::StandardButton reply = QMessageBox::question(dialog,
            "Confirm Dropoff",
            QString("Use %1 as dropoff location?").arg(locName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (reply == QMessageBox::Yes)
        {
            destPlaceholder->setText(locName + " - " + locId);
            destPlaceholder->setStyleSheet(
                "color: #2E7D32;"
                "font-size: 15px;"
                "font-weight: 600;"
                "background: transparent;"
                "border: none;"
            );
            dropoffNodeId = locId;
            dialog->accept();
        }
    });

    layout->addWidget(view);
    dialog->show();
}

void RiderWindow::onRequestRideClicked()
{
    if (dropoffNodeId.isEmpty())
    {
        QMessageBox::information(this, "Select destination", "Please pick a dropoff location first.");
        return;
    }

    // Prevent requesting a new ride if one is already active
    if (dispatchEngine)
    {
        bool okId = false;
        int riderNumericIdCheck = riderId.toInt(&okId);
        if (!okId) riderNumericIdCheck = 1;

        bool hasActiveRide = false;

        // Check current tracked trip state if available
        if (currentTripId > 0)
        {
            Trip *existing = dispatchEngine->getTrip(currentTripId);
            if (existing)
            {
                TripState s = existing->getState();
                if (s == REQUESTED || s == ASSIGNED || s == PICKUP_IN_PROGRESS || s == ONGOING)
                {
                    hasActiveRide = true;
                }
            }
        }

        // Fallback: scan active trips list to detect ongoing rides for this rider
        if (!hasActiveRide)
        {
            ActiveTrip *head = dispatchEngine->getActiveTripsHead();
            while (head)
            {
                if (head->trip && head->trip->getRiderId() == riderNumericIdCheck)
                {
                    TripState s = head->trip->getState();
                    if (s == PICKUP_IN_PROGRESS || s == ONGOING)
                    {
                        hasActiveRide = true;
                        break;
                    }
                }
                head = head->next;
            }
        }

        if (hasActiveRide)
        {
            QMessageBox::warning(this, "Ride already in progress",
                                 "You already have an active ride. Please cancel or complete it before requesting another.");
            return;
        }
    }

    if (!ensureDispatchEngine())
        return;

    bool ok = false;
    int riderNumericId = riderId.toInt(&ok);
    if (!ok) riderNumericId = 1;

    int tripId = nextTripId++;
    currentTripId = tripId;
    rejectedDriverIds.clear();  // Clear rejected drivers for new trip request
    
    QByteArray pickupBytes = locationId.toUtf8();
    QByteArray dropBytes = dropoffNodeId.toUtf8();

    if (!dispatchEngine->requestTrip(tripId, riderNumericId, pickupBytes.constData(), dropBytes.constData()))
    {
        QMessageBox::warning(this, "Trip error", "Unable to create trip.");
        return;
    }

    int driverId = dispatchEngine->assignNearestDriver(tripId);
    if (driverId < 0)
    {
        // Store the pending request details for auto-retry
        pendingPickupNodeId = locationId;
        pendingDropoffNodeId = dropoffNodeId;
        retryCount = 0;
        
        // Start auto-retry - don't show error, just retry
        tripStatusLabel->setText("Searching for drivers...");
        retryTimer->start(1500);  // Retry every 1.5 seconds
        return;
    }

    // Show driver confirmation dialog instead of directly assigning
    showDriverConfirmationDialog(tripId, driverId, false);
}

void RiderWindow::startTripProgress(int tripId)
{
    if (tripTimer->isActive())
        tripTimer->stop();
    tripTimer->disconnect();

    connect(tripTimer, &QTimer::timeout, this, [this, tripId]() {
        advanceTripProgress(tripId);
    });
    tripTimer->start(300);
}

void RiderWindow::advanceTripProgress(int tripId)
{
    if (!dispatchEngine)
        return;
    Trip *trip = dispatchEngine->getTrip(tripId);
    if (!trip)
    {
        tripTimer->stop();
        return;
    }

    bool more = dispatchEngine->advanceTripMovement(tripId);
    TripState state = trip->getState();

    // Update user's current location during trip
    QString oldLocation = locationId;
    locationId = QString::fromUtf8(trip->getRiderCurrentNodeId());
    updateCurrentLocationUI();
    
    // Record rider location change snapshot
    if (oldLocation != locationId && dispatchEngine && dispatchEngine->getRollbackManager()) {
        bool ok = false;
        int riderNumericId = riderId.toInt(&ok);
        if (!ok) riderNumericId = 1;
        
        dispatchEngine->getRollbackManager()->recordSnapshot(
            3, tripId, trip->getDriverId(), state, true,
            nullptr, riderNumericId, locationId.toUtf8().constData(), true, riderId.toUtf8().constData()
        );
    }
    
    emit locationUpdated(riderId, locationId);

    if (!more && state == ONGOING)
    {
        // Trip reached destination
        dispatchEngine->completeTrip(tripId);
        trip = dispatchEngine->getTrip(tripId);
        state = trip ? trip->getState() : COMPLETED;
        
        // Update rider's final location to dropoff
        if (trip)
        {
            QString oldLocation = locationId;
            locationId = QString::fromUtf8(trip->getDropoffNodeId());
            updateCurrentLocationUI();
            
            // Record rider location change snapshot
            if (oldLocation != locationId && dispatchEngine && dispatchEngine->getRollbackManager()) {
                bool ok = false;
                int riderNumericId = riderId.toInt(&ok);
                if (!ok) riderNumericId = 1;
                
                dispatchEngine->getRollbackManager()->recordSnapshot(
                    3, tripId, trip->getDriverId(), state, true,
                    nullptr, riderNumericId, locationId.toUtf8().constData(), true, riderId.toUtf8().constData()
                );
            }
            
            emit locationUpdated(riderId, locationId);
        }
    }

    QString extra;
    switch (state)
    {
        case REQUESTED: extra = "Requested"; break;
        case ASSIGNED: extra = "Driver assigned"; break;
        case PICKUP_IN_PROGRESS: extra = "En route to pickup"; break;
        case ONGOING: extra = "On trip"; break;
        case COMPLETED: extra = "Completed"; break;
        case CANCELLED: extra = "Cancelled"; break;
    }

    updateTripStatusUI(tripId, extra);
    
    // Show/hide cancel button based on trip state
    if (cancelRideButton) {
        cancelRideButton->setVisible(state == PICKUP_IN_PROGRESS);
    }

    if (state == COMPLETED || state == CANCELLED)
    {
        tripTimer->stop();
        
        // Hide cancel button
        if (cancelRideButton) {
            cancelRideButton->setVisible(false);
        }
        
        // Save trip to history
        if (trip)
        {
            QString pickup = QString::fromUtf8(trip->getPickupNodeId());
            QString dropoff = QString::fromUtf8(trip->getDropoffNodeId());
            QString status = (state == COMPLETED) ? "COMPLETED" : "CANCELLED";
            double fare = trip->calculateTotalFare();
            double distance = trip->getRideDistance();
            int driverId = trip->getDriverId();
            
            addTripToHistory(tripId, pickup, dropoff, status, fare, distance, driverId);
            refreshHistoryPage();
        }
        
        // Update destination placeholder to reflect trip completion
        if (state == COMPLETED)
        {
            QMessageBox::information(this, "Trip Complete", "Your ride has been completed!");
            destPlaceholder->setText("Where do you wanna go?");
            destPlaceholder->setStyleSheet(
                "color: rgba(46,125,50,0.5);"
                "font-size: 15px;"
                "font-weight: 500;"
                "background: transparent;"
                "border: none;"
            );
            dropoffNodeId = "";
            rejectedDriverIds.clear();  // Reset for next ride
        }
        else if (state == CANCELLED)
        {
            // Also reset for cancelled trips
            dropoffNodeId = "";
            rejectedDriverIds.clear();
        }
    }
}

void RiderWindow::updateCurrentLocationUI()
{
    if (currentLocationLabel)
    {
        if (locationId.isEmpty())
        {
            currentLocationLabel->setText("Location not set");
            currentLocationLabel->setStyleSheet(
                "color: #999;"
                "font-size: 14px;"
                "font-weight: 500;"
                "background: rgba(200,200,200,0.1);"
                "border-radius: 8px;"
                "padding: 10px 12px;"
                "border: 1px solid rgba(150,150,150,0.2);"
            );
        }
        else
        {
            currentLocationLabel->setText(locationId);
            currentLocationLabel->setStyleSheet(
                "color: #2E7D32;"
                "font-size: 16px;"
                "font-weight: 600;"
                "background: rgba(255,255,255,0.5);"
                "border-radius: 8px;"
                "padding: 10px 12px;"
                "border: 1px solid rgba(76,175,80,0.2);"
            );
        }
    }
}
void RiderWindow::updateTripStatusUI(int tripId, const QString &extra)
{
    if (!dispatchEngine || !tripStatusLabel)
        return;
    Trip *trip = dispatchEngine->getTrip(tripId);
    if (!trip)
        return;

    QString stateText = QString::fromUtf8(trip->stateToString(trip->getState()));
    QString info = QString("Trip #%1 • %2").arg(tripId).arg(stateText);
    if (!extra.isEmpty())
        info += QString(" • %1").arg(extra);
    tripStatusLabel->setText(info);

    Driver *driver = dispatchEngine->getDriver(trip->getDriverId());
    if (driver)
    {
        driverStatusLabel->setText(QString("Driver #%1 at %2").arg(driver->getDriverId()).arg(driver->getCurrentNodeId()));
    }
}

void RiderWindow::onMapButtonClick()
{
    showMapDialog();
}

void RiderWindow::showDriverConfirmationDialog(int tripId, int driverId, bool isRetry)
{
    if (!dispatchEngine)
        return;
    
    Trip *trip = dispatchEngine->getTrip(tripId);
    if (!trip)
        return;
    
    Driver *driver = dispatchEngine->getDriver(driverId);
    if (!driver)
        return;
    
    // Get driver details
    QString driverLocation = QString::fromUtf8(driver->getCurrentNodeId());
    QString driverZone = QString::fromUtf8(driver->getZone());
    
    // Calculate distance from driver to pickup location (sum of edge weights)
    const PathResult &driverToPickupPath = trip->getDriverToPickupPath();
    double distanceToPickup = driverToPickupPath.totalDistance > 0 ? driverToPickupPath.totalDistance : 0.0;
    
    // Calculate total trip distance and fare using Trip class functions
    double totalDistance = trip->getRideDistance();
    double baseFare = trip->calculateBaseFare();
    double zoneSurcharge = trip->calculateZoneSurcharge();
    double totalFare = trip->calculateTotalFare();
    
    // Create confirmation dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Confirm Driver");
    dialog.setModal(true);
    dialog.setMinimumWidth(450);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    QLabel *titleLabel = new QLabel(isRetry ? "Alternative Driver Found" : "Driver Found");
    titleLabel->setStyleSheet(
        "color: #1B5E20;"
        "font-size: 18px;"
        "font-weight: 700;"
    );
    layout->addWidget(titleLabel);
    
    // Driver details card
    QWidget *detailsCard = new QWidget();
    detailsCard->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "  stop:0 rgba(76,175,80,0.1), stop:1 rgba(46,125,50,0.05));"
        "border: 1px solid rgba(76,175,80,0.25);"
        "border-radius: 10px;"
        "padding: 15px;"
    );
    
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsCard);
    detailsLayout->setSpacing(10);
    
    // Driver ID
    QLabel *driverIdLabel = new QLabel();
    driverIdLabel->setText(QString("<b>Driver ID:</b> #%1").arg(driverId));
    driverIdLabel->setStyleSheet("color: #1B5E20; font-size: 14px;");
    detailsLayout->addWidget(driverIdLabel);
    
    // Driver Location
    QLabel *driverLocLabel = new QLabel();
    driverLocLabel->setText(QString("<b>Current Location:</b> %1 (Zone: %2)").arg(driverLocation, driverZone));
    driverLocLabel->setStyleSheet("color: #1B5E20; font-size: 14px;");
    driverLocLabel->setWordWrap(true);
    detailsLayout->addWidget(driverLocLabel);
    
    // Distance to pickup (already in meters from PathResult.totalDistance)
    QLabel *distanceLabel = new QLabel();
    distanceLabel->setText(QString("<b>Distance to Your Pickup:</b> %1 meters").arg((int)distanceToPickup));
    distanceLabel->setStyleSheet("color: #2E7D32; font-size: 14px; font-weight: 600;");
    detailsLayout->addWidget(distanceLabel);
    
    // Total trip distance
    QLabel *tripDistLabel = new QLabel();
    tripDistLabel->setText(QString("<b>Total Trip Distance:</b> %1 meters").arg((int)totalDistance));
    tripDistLabel->setStyleSheet("color: #2E7D32; font-size: 14px; font-weight: 600;");
    detailsLayout->addWidget(tripDistLabel);
    
    // Fare breakdown
    QLabel *baseFareLabel = new QLabel();
    baseFareLabel->setText(QString("<b>Base Fare:</b> Rs. %1").arg(baseFare, 0, 'f', 2));
    baseFareLabel->setStyleSheet("color: #2E7D32; font-size: 13px;");
    detailsLayout->addWidget(baseFareLabel);
    
    if (zoneSurcharge > 0.0) {
        QLabel *surchargeLabel = new QLabel();
        surchargeLabel->setText(QString("<b>Zone Surcharge:</b> Rs. %1").arg(zoneSurcharge, 0, 'f', 2));
        surchargeLabel->setStyleSheet("color: #FF9800; font-size: 13px; font-weight: 600;");
        detailsLayout->addWidget(surchargeLabel);
    }
    
    // Total fare
    QLabel *fareLabel = new QLabel();
    fareLabel->setText(QString("<b>Total Estimated Fare:</b> Rs. %1").arg(totalFare, 0, 'f', 2));
    fareLabel->setStyleSheet(
        "color: white;"
        "background: #4CAF50;"
        "font-size: 15px;"
        "font-weight: 700;"
        "padding: 10px;"
        "border-radius: 8px;"
    );
    detailsLayout->addWidget(fareLabel);
    
    layout->addWidget(detailsCard);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    QPushButton *confirmBtn = new QPushButton("Confirm Driver");
    confirmBtn->setStyleSheet(
        "QPushButton {"
        "  background: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px 20px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background: #66BB6A; }"
        "QPushButton:pressed { background: #388E3C; }"
    );
    
    QPushButton *findOtherBtn = new QPushButton("Find Another Driver");
    findOtherBtn->setStyleSheet(
        "QPushButton {"
        "  background: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px 20px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background: #42A5F5; }"
        "QPushButton:pressed { background: #1976D2; }"
    );
    
    QPushButton *cancelBtn = new QPushButton("Cancel Ride");
    cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background: #F44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px 20px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background: #EF5350; }"
        "QPushButton:pressed { background: #D32F2F; }"
    );
    
    buttonLayout->addWidget(confirmBtn);
    buttonLayout->addWidget(findOtherBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);
    
    connect(confirmBtn, &QPushButton::clicked, this, [this, tripId, driverId, &dialog]() {
        handleDriverConfirmation(tripId, driverId, true);
        dialog.accept();
    });
    
    connect(findOtherBtn, &QPushButton::clicked, this, [this, tripId, driverId, &dialog]() {
        dialog.reject();
        rejectedDriverIds.append(driverId);
        
        Trip *trip = dispatchEngine->getTrip(tripId);
        if (!trip) {
            return;
        }
        
        // Free the current driver before finding another
        Driver *currentDriver = dispatchEngine->getDriver(driverId);
        if (currentDriver) {
            currentDriver->setAvailable(true);
            currentDriver->setAssignedTripId(-1);
        }
        
        // Find the nearest available driver, excluding previously rejected ones
        int newDriverId = -1;
        double minDistance = 1e9;
        const char *pickupNode = trip->getPickupNodeId();
        
        // Loop through all drivers to find the nearest available one
        for (int i = 1; i <= 20; i++) {
            if (!rejectedDriverIds.contains(i)) {
                Driver *candidate = dispatchEngine->getDriver(i);
                if (candidate && candidate->isAvailable()) {
                    // Calculate Euclidean distance as heuristic (actual path calculated on assignment)
                    double dist = cityGraph->getDistance(candidate->getCurrentNodeId(), pickupNode);
                    if (dist >= 0 && dist < minDistance) {
                        minDistance = dist;
                        newDriverId = i;
                    }
                }
            }
        }
        
        if (newDriverId > 0) {
            // Reset trip state and reassign to new driver to recalculate paths
            trip->setState(REQUESTED);
            dispatchEngine->assignTrip(tripId, newDriverId);
            showDriverConfirmationDialog(tripId, newDriverId, true);
        } else {
            QMessageBox::warning(this, "No Alternative Drivers", 
                "No other available drivers found. Would you like to try again?");
            // Re-assign the rejected driver since no alternative found
            currentDriver->setAvailable(false);
            currentDriver->setAssignedTripId(tripId);
        }
    });
    
    connect(cancelBtn, &QPushButton::clicked, this, [this, tripId, &dialog]() {
        dialog.reject();
        
        // Get trip info before cancelling
        Trip *trip = dispatchEngine->getTrip(tripId);
        if (trip) {
            QString pickup = QString::fromUtf8(trip->getPickupNodeId());
            QString dropoff = QString::fromUtf8(trip->getDropoffNodeId());
            double fare = trip->calculateTotalFare();
            double distance = trip->getRideDistance();
            int driverId = trip->getDriverId();
            
            // Cancel the trip
            dispatchEngine->cancelTrip(tripId);
            
            // Save to history
            addTripToHistory(tripId, pickup, dropoff, "CANCELLED", fare, distance, driverId);
            refreshHistoryPage();
        } else {
            dispatchEngine->cancelTrip(tripId);
        }
        
        destPlaceholder->setText("Where do you wanna go?");
        destPlaceholder->setStyleSheet(
            "color: rgba(46,125,50,0.6);"
            "font-size: 15px;"
            "font-weight: 500;"
            "background: transparent;"
            "border: none;"
        );
        tripStatusLabel->setText("Ride request cancelled");
        driverStatusLabel->setText("");
        dropoffNodeId = "";
        rejectedDriverIds.clear();
    });
    
    dialog.exec();
}

void RiderWindow::handleDriverConfirmation(int tripId, int driverId, bool accepted)
{
    if (!dispatchEngine)
        return;
    
    if (!accepted) {
        dispatchEngine->cancelTrip(tripId);
        rejectedDriverIds.clear();
        dropoffNodeId = "";
        return;
    }
    
    // Assign the driver and start trip
    updateTripStatusUI(tripId, "Driver assigned");
    driverStatusLabel->setText(QString("Driver #%1 en route").arg(driverId));
    
    dispatchEngine->startPickupMovement(tripId);
    startTripProgress(tripId);
    // Keep rejectedDriverIds until trip completes - don't clear here
}

void RiderWindow::addTripToHistory(int tripId, const QString &pickup, const QString &dropoff,
                                    const QString &status, double fare, double distance, int driverId)
{
    TripHistoryRecord record;
    record.tripId = tripId;
    record.pickupNode = pickup.toStdString();
    record.dropoffNode = dropoff.toStdString();
    record.status = status.toStdString();
    record.fare = fare;
    record.distance = distance;
    record.driverId = driverId;
    
    // Get current timestamp
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    record.timestamp = buffer;
    
    // Update local cache for this window
    tripHistory.push_back(record);
    // Update session-wide store
    RiderWindow::s_sessionHistory[this->riderId].push_back(record);
    
    // Also record a snapshot for Rollback view to display history
    if (dispatchEngine && dispatchEngine->getRollbackManager()) {
        bool ok = false;
        int riderNumericId = riderId.toInt(&ok);
        if (!ok) riderNumericId = 1;
        dispatchEngine->getRollbackManager()->recordHistorySnapshot(
            tripId,
            riderNumericId,
            driverId,
            pickup.toUtf8().constData(),
            dropoff.toUtf8().constData(),
            status.toUtf8().constData(),
            fare,
            distance,
            riderId.toUtf8().constData()
        );
    }

    qDebug() << "Added trip to history:" << tripId << status << "- Stored in memory only";
}

void RiderWindow::refreshHistoryPage()
{
    // Recreate the history page to reflect new trip history
    QWidget *oldHistoryPage = contentStack->widget(1);
    QWidget *newHistoryPage = createHistoryPage();
    
    contentStack->removeWidget(oldHistoryPage);
    contentStack->insertWidget(1, newHistoryPage);
    
    delete oldHistoryPage;
    
    // Report from session store to reflect true session count
    qDebug() << "History page refreshed with" << RiderWindow::s_sessionHistory[this->riderId].size() << "trips";
}

QString RiderWindow::getHistoryFilePath() const
{
    // Store history files in a "rider_data" directory in the application folder
    QString dataDir = QCoreApplication::applicationDirPath() + "/rider_data";
    
    // Create directory if it doesn't exist
    QDir dir;
    if (!dir.exists(dataDir)) {
        dir.mkpath(dataDir);
    }
    
    // Return file path for this rider's history
    return dataDir + "/rider_" + riderId + "_history.json";
}

void RiderWindow::saveHistoryToFile()
{
    // History is now memory-only - no persistence to files
    // All data will be cleared when program ends
    qDebug() << "History stored in memory only (not persisted to file)";
}

void RiderWindow::loadHistoryFromFile()
{
    // History is now memory-only and not loaded from files
    // Each program session starts with empty history
    qDebug() << "History mode: Memory-only (no persistence from previous sessions)";
}

void RiderWindow::clearHistoryFile()
{
    QString historyFilePath = getHistoryFilePath();
    QFile historyFile(historyFilePath);
    if (historyFile.exists())
    {
        if (historyFile.remove())
        {
            qDebug() << "History file deleted for rider" << riderId;
        }
        else
        {
            qWarning() << "Failed to delete history file:" << historyFilePath;
        }
    }
}

void RiderWindow::retryRequestRide()
{
    if (!dispatchEngine)
    {
        retryTimer->stop();
        return;
    }
    
    retryCount++;
    qDebug() << "Auto-retrying driver request attempt" << retryCount << "of" << maxRetries;
    
    // Try to request and assign a new trip
    int tripId = nextTripId++;
    currentTripId = tripId;
    
    bool ok = false;
    int riderNumericId = riderId.toInt(&ok);
    if (!ok) riderNumericId = 1;
    
    QByteArray pickupBytes = pendingPickupNodeId.toUtf8();
    QByteArray dropBytes = pendingDropoffNodeId.toUtf8();
    
    if (!dispatchEngine->requestTrip(tripId, riderNumericId, pickupBytes.constData(), dropBytes.constData()))
    {
        // Failed to create trip, retry again
        if (retryCount >= maxRetries)
        {
            retryTimer->stop();
            tripStatusLabel->setText("Failed to find drivers. Please try again.");
            qWarning() << "Max retries exceeded for driver assignment";
        }
        return;
    }
    
    // Try to find a driver
    int driverId = dispatchEngine->assignNearestDriver(tripId);
    if (driverId < 0)
    {
        // No driver found yet, continue retrying
        if (retryCount >= maxRetries)
        {
            retryTimer->stop();
            dispatchEngine->cancelTrip(tripId);
            tripStatusLabel->setText("No drivers available. Please try again later.");
            qWarning() << "Max retries exceeded for driver assignment after" << maxRetries << "attempts";
        }
        return;
    }
    
    // Success! Found a driver
    retryTimer->stop();
    qDebug() << "Driver found on attempt" << retryCount << "- Driver ID:" << driverId;
    
    // Show the confirmation dialog
    showDriverConfirmationDialog(tripId, driverId, false);
}

// Static method to remove a history entry (for rollback functionality)
bool RiderWindow::removeHistoryEntry(const QString &riderCode, int tripId)
{
    if (!s_sessionHistory.contains(riderCode)) {
        return false;
    }
    
    QVector<TripHistoryRecord> &history = s_sessionHistory[riderCode];
    for (int i = 0; i < history.size(); ++i) {
        if (history[i].tripId == tripId) {
            history.removeAt(i);
            qDebug() << "Removed history entry for rider" << riderCode << "trip" << tripId;
            return true;
        }
    }
    
    return false;
}