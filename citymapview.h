#ifndef CITYMAPVIEW_H
#define CITYMAPVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QSet>
#include <QHash>
#include <QPointF>
#include <QColor>
#include <QPoint>
#include <QVector>

#include "core/city.h"

// Custom graphics items that can be hidden/shown based on zoom
class StreetNameItem : public QGraphicsTextItem
{
public:
    StreetNameItem(const QString &text, QGraphicsItem *parent = nullptr)
        : QGraphicsTextItem(text, parent) {}
    int type() const override { return UserType + 1; }
};

class LocationBlockItem : public QGraphicsEllipseItem
{
public:
    LocationBlockItem(const QRectF &rect, const QString &name, const QString &type, QGraphicsItem *parent = nullptr)
        : QGraphicsEllipseItem(rect, parent), locationName(name), locationType(type) {}
    int type() const override { return UserType + 2; }
    QString locationName;
    QString locationType;
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
private:
    QGraphicsTextItem *tooltipItem = nullptr;
};

class LocationLabelItem : public QGraphicsTextItem
{
public:
    LocationLabelItem(const QString &text, QGraphicsItem *parent = nullptr)
        : QGraphicsTextItem(text, parent) {}
    int type() const override { return UserType + 3; }
};

class LocationMarkerItem : public QGraphicsRectItem
{
public:
    LocationMarkerItem(double x, double y, const QString &id, const QString &name, const QString &type, QGraphicsItem *parent = nullptr)
        : QGraphicsRectItem(x - 4, y - 4, 8, 8, parent), locationId(id), locationName(name), locationType(type),
          isHovered(false), labelItem(nullptr)
    {
        setAcceptHoverEvents(true);
    }
    
    ~LocationMarkerItem()
    {
        // Label is a child item, Qt will automatically delete it
        labelItem = nullptr;
    }
    
    int type() const override { return UserType + 4; }
    QString locationId;
    QString locationName;
    QString locationType;
    
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    
private:
    bool isHovered;
    QGraphicsTextItem *labelItem;
};

class CityMapView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CityMapView(City *city, QWidget *parent = nullptr);
    void setUserLocation(const QString &locationId);
    void enterSelectionMode();
    void exitSelectionMode();

signals:
    void locationPicked(const QString &locationId, const QString &locationName);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    City *city;
    QGraphicsScene *scene;
    double scaleFactor;
    double minX, maxX, minY, maxY;
    QPoint lastPanPos;
    bool isPanning;
    double currentZoomLevel;
    double streetWidth;
    QString currentDistrict;
    QHash<QString, QString> poiGlyphMap;
    QSet<QString> drawnEdges;
    QString userLocationId;
    QGraphicsPathItem *userLocationMarker;
    bool selectionMode;
    QCursor pinCursor;
    
    struct BoundingBox {
        double minX, maxX, minY, maxY;
        QPointF center;
        QString name;
    };
    
    QHash<QString, BoundingBox> zoneBounds;           // zone -> bounds
    QHash<QString, BoundingBox> colonyBounds;         // "zone_colony" -> bounds
    QHash<QString, BoundingBox> streetBounds;         // "zone_colony_street" -> bounds
    
    QList<QGraphicsTextItem*> zoneLabels;
    QList<QGraphicsTextItem*> colonyLabels;
    QList<QGraphicsTextItem*> streetLabels;
    QList<QGraphicsTextItem*> locationEmojis;
    QList<LocationMarkerItem*> locationMarkers;  // Track all location markers

    // Street data structure: key = streetKey, value = list of nodes
    struct StreetSegment {
        QVector<QPointF> points;
        QString streetName;
        QString zone;
    };
    QHash<QString, StreetSegment> streets;

    void buildScene();
    void drawAllEdges();
    void drawLocationEmojis();
    void updateVisibilityBasedOnZoom();
    void updateLocationMarkerHoverState();
    void calculateZoneAndColonyBounds();
    void updateLabelsBasedOnView();
    void drawStreetNamesOnEdges(const QRectF &viewRect);
    void clearAllLabels();
    int countVisibleZones() const;
    int countVisibleColonies() const;
    QRectF getViewportRect() const;
    void drawTextAlongPath(const QPainterPath &path, const QString &text);
    void drawDistrictLabel();
    void initializePOIGlyphs();
    QString getPOIGlyph(const QString &locationType) const;
    
    QPointF mapToScreen(double x, double y) const;
    QString getStreetKey(Node *node) const;
    QColor getZoneColor(const QString &zone) const;
    QColor getLocationColor(const QString &locationType) const;
    QString getLocationIcon(const QString &locationType) const;
};

#endif // CITYMAPVIEW_H
