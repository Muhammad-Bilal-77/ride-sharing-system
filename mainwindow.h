#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include <QString>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "riderwindow.h"
#include "core/city.h"
#include "core/dispatchengine.h"
#include "core/rollbackmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // Trip restoration info
    struct RestoredTripInfo {
        int tripId;
        QString pickup;
        QString dropoff;
        double fare;
        double distance;
        int driverId;
    };

private:
    Ui::MainWindow *ui;
    QStackedWidget *stackedWidget;
    QWidget *homePage;
    QWidget *rollbackManagerPage;
    QWidget *analyticsPage;
    QVBoxLayout *snapshotContainerLayout;
    QVBoxLayout *riderAnalyticsLayout;
    QVBoxLayout *driverAnalyticsLayout;
    QLabel *analyticsSummaryLabel;
    // Premium tiles
    QLabel *tileTripsLabel;
    QLabel *tileCompletedLabel;
    QLabel *tileCancelledLabel;
    QLabel *tileDistanceLabel;
    QLabel *tileSpendLabel;
    QPushButton *chooseUserButton;
    QPushButton *rollbackManagerButton;
    QPushButton *analyticsButton;
    QLabel *imageLabel;
    RiderWindow *riderWindow;
    
    // Shared city graph and dispatch engine for all riders
    City *sharedCity;
    DispatchEngine *sharedDispatchEngine;
    bool cityLoaded;
    bool driversInitialized;

    struct Rider {
        QString riderId;
        QString zone;
        QString colony;
        QString locationName;
        QString locationId;
    };
    QVector<Rider> riders;
    QHash<QString, QString> riderLocations; // Track current location of each rider
    QHash<QString, RestoredTripInfo> restoredTrips; // Track trips restored by rollback: riderCode -> trip info

    void loadRiders();
    QString resolveDataFile(const QString &fileName) const;
    void showRiderMenu();
    bool ensureCityLoaded();
    void ensureDispatchEngine();
    void initializeDrivers();
    void createRollbackManagerPage();
    void createAnalyticsPage();
    void showRollbackManagerPage();
    void showAnalyticsPage();
    void goBackToHome();
    void refreshSnapshotDisplay();
    void refreshAnalytics();

public slots:
    void updateRiderLocation(const QString &riderId, const QString &newLocation);
    
public:
    RestoredTripInfo getRestoredTripForRider(const QString &riderCode);
    void clearRestoredTrip(const QString &riderCode);
    void setRestoredTrip(const QString &riderCode, int tripId, const QString &pickup, 
                        const QString &dropoff, double fare, double distance, int driverId);
};
#endif // MAINWINDOW_H
