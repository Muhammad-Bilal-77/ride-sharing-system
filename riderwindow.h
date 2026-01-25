#ifndef RIDERWINDOW_H
#define RIDERWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QLabel>
#include <QMap>
#include <QStringList>
#include <QVector>
#include "core/city.h"
#include "core/dispatchengine.h"
#include "core/rider.h"

class RiderWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RiderWindow(const QString &riderId, const QString &locationId, QWidget *parent = nullptr);
    ~RiderWindow();
    
    void setSharedResources(City *city, DispatchEngine *engine);
    
    // Show restored trip section
    void showRestoredTripSection(const QString &pickup, const QString &dropoff, 
                                 double fare, double distance, int tripId);
    void hideRestoredTripSection();
    
    // Static methods for rollback functionality
    static bool removeHistoryEntry(const QString &riderCode, int tripId);
    static QMap<QString, QVector<TripHistoryRecord>>& getSessionHistory() { return s_sessionHistory; }

signals:
    void backRequested();
    void locationUpdated(const QString &riderId, const QString &newLocation);

private:
    // Session-wide history store shared across RiderWindow instances
    static QMap<QString, QVector<TripHistoryRecord>> s_sessionHistory;

    QString riderId;
    QString locationId;
    QString dropoffNodeId;
    QLabel *welcomeLabel;
    QLabel *profileImageLabel;
    QPushButton *backButton;
    QPushButton *bookRideButton;
    QPushButton *historyButton;
    QPushButton *mapButton;
    QPushButton *requestRideButton;
    QStackedWidget *contentStack;
    QLabel *destPlaceholder;
    QWidget *mapImage;
    City *cityGraph;
    bool cityLoaded;
    DispatchEngine *dispatchEngine;
    bool driversInitialized;
    int nextTripId;
    QTimer *tripTimer;
    QLabel *tripStatusLabel;
    QLabel *driverStatusLabel;
    QLabel *currentLocationLabel;
    QPushButton *cancelRideButton;  // Button to cancel ride during pickup
    
    // Map overlay elements
    QLabel *mapLocationOverlay;
    QWidget *mapLocationBox;
    QPushButton *mapHoverButton;
    
    bool usingSharedResources; // Track if using shared resources (don't delete them)
    
    // Auto-retry for driver requests
    QTimer *retryTimer;
    int retryCount;
    int maxRetries;
    QString pendingPickupNodeId;
    QString pendingDropoffNodeId;
    
    // Location data structures
    struct LocationInfo {
        QString id;
        QString name;
        QString type;
    };
    
    // Hierarchy: Zone -> Colony -> Street -> Locations/Nodes
    QMap<QString, QMap<QString, QMap<QString, QList<LocationInfo>>>> zoneData; // zone -> colony -> street -> locations
    QMap<QString, QMap<QString, QStringList>> streetNodes; // zone_colony -> street -> nodes
    QStringList highwayNodes;
    QStringList zoneConnectors;
    
    // Driver confirmation state
    int currentTripId;
    QList<int> rejectedDriverIds;  // Track rejected drivers in current request
    
    // Trip history for this rider
    QVector<TripHistoryRecord> tripHistory;
    
    // Restored trip information
    struct RestoredTrip {
        int tripId;
        QString pickup;
        QString dropoff;
        double fare;
        double distance;
    };
    RestoredTrip currentRestoredTrip;
    QWidget *restoredTripWidget;
    
    void setupUI();
    void createSidebar(QWidget *parent);
    QWidget* createBookRidePage();
    QWidget* createHistoryPage();
    QWidget* createRestoredTripWidget(const QString &pickup, const QString &dropoff, 
                                      double fare, double distance, int tripId);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void loadLocationData();
    void loadStreetNodes();
    void showZoneSelection();
    void showColonySelection(const QString &zone);
    void showStreetSelection(const QString &zone, const QString &colony);
    void showLocationSelection(const QString &zone, const QString &colony, const QString &street);
    void showHighwayNodes();
    void showZoneConnectors();
    bool ensureCityLoaded();
    bool ensureDispatchEngine();
    void initializeDrivers();
    QString resolveDataFile(const QString &fileName) const;
    void showMapDialog();
    void updateTripStatusUI(int tripId, const QString &extra = QString());
    void updateCurrentLocationUI();
    void startTripProgress(int tripId);
    void advanceTripProgress(int tripId);
    void showDriverConfirmationDialog(int tripId, int driverId, bool isRetry = false);
    void handleDriverConfirmation(int tripId, int driverId, bool accepted);
    void addTripToHistory(int tripId, const QString &pickup, const QString &dropoff,
                          const QString &status, double fare, double distance, int driverId);
    void refreshHistoryPage();
    void saveHistoryToFile();
    void loadHistoryFromFile();
    QString getHistoryFilePath() const;
    void clearHistoryFile();
    void retryRequestRide();  // Automatic retry for driver requests
    
private slots:
    void onDestinationClick();
    void onMapClick();
    void onMapButtonClick();
    void onPickFromMapClicked();
    void onRequestRideClicked();
};

#endif // RIDERWINDOW_H
