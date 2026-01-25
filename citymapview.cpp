#include "citymapview.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsSceneHoverEvent>
#include <QDebug>
#include <QPainterPath>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QtMath>
#include <QDebug>
#include <QCursor>

// Implementation of LocationMarkerItem hover events
void LocationMarkerItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    isHovered = true;
    
    // Make border darker/thicker on hover
    setPen(QPen(QColor(100, 100, 100), 2));
    
    // Create and show location name label as child item (safer)
    if (!labelItem)
    {
        // Create label as a child of this item - Qt manages memory automatically
        labelItem = new QGraphicsTextItem(locationName, this);
        QFont nameFont("Segoe UI", 7);
        nameFont.setBold(true);
        labelItem->setFont(nameFont);
        labelItem->setDefaultTextColor(QColor(100, 100, 100));  // Dark grey
        
        // Position relative to this item (parent coordinates)
        labelItem->setPos(10, -15);
        labelItem->setZValue(2);  // Relative to parent
    }
    
    labelItem->show();
    
    QGraphicsRectItem::hoverEnterEvent(event);
}

void LocationMarkerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    isHovered = false;
    
    // Restore normal border
    setPen(QPen(QColor(50, 50, 50), 1));
    
    // Just hide the label instead of deleting it
    if (labelItem)
    {
        labelItem->hide();
    }
    
    QGraphicsRectItem::hoverLeaveEvent(event);
}

CityMapView::CityMapView(City *city, QWidget *parent)
    : QGraphicsView(parent), city(city), scene(new QGraphicsScene(this)), 
      scaleFactor(1.0), minX(0), maxX(0), minY(0), maxY(0), isPanning(false), currentZoomLevel(1.0), streetWidth(12.0),
      userLocationId(""), userLocationMarker(nullptr), selectionMode(false)
{
    setScene(scene);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setDragMode(QGraphicsView::NoDrag);
    setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    initializePOIGlyphs();
    
    // Build custom pin cursor for selection mode
    QPixmap pinPix(32, 48);
    pinPix.fill(Qt::transparent);
    {
        QPainter p(&pinPix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.moveTo(16, 46);
        path.quadTo(0, 20, 16, 4);
        path.quadTo(32, 20, 16, 46);
        p.fillPath(path, QColor(128, 0, 128));
        p.setPen(QPen(QColor(90, 0, 90), 2));
        p.drawPath(path);
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(16, 18), 5, 5);
    }
    pinCursor = QCursor(pinPix, 16, 40); // hotspot near tip
    
    buildScene();
}

void CityMapView::setUserLocation(const QString &locationId)
{
    userLocationId = locationId;
    
    // Remove existing marker if any
    if (userLocationMarker)
    {
        scene->removeItem(userLocationMarker);
        delete userLocationMarker;
        userLocationMarker = nullptr;
    }
    
    if (userLocationId.isEmpty() || !city)
        return;
    
    // Find the node with this location ID
    Node *userNode = city->getNode(userLocationId.toUtf8().constData());
    if (!userNode)
        return;
    
    // Create purple location pin marker at user location
    QPointF pos = mapToScreen(userNode->x, userNode->y);
    
    // Create custom pin/pointer shape using QPainterPath
    QPainterPath pinPath;
    
    // Pin dimensions
    double pinWidth = 24;
    double pinHeight = 36;
    double centerX = pos.x();
    double centerY = pos.y();
    
    // Create the pin shape (like a teardrop/location marker)
    // Start at the bottom point
    pinPath.moveTo(centerX, centerY);
    
    // Left curve from point to top
    pinPath.quadTo(centerX - pinWidth/2, centerY - pinHeight * 0.6,
                   centerX - pinWidth/2, centerY - pinHeight * 0.7);
    
    // Top circular part (left side)
    pinPath.arcTo(centerX - pinWidth/2, centerY - pinHeight, 
                  pinWidth, pinWidth, 
                  180, -180);
    
    // Right curve from top to point
    pinPath.quadTo(centerX + pinWidth/2, centerY - pinHeight * 0.6,
                   centerX, centerY);
    
    pinPath.closeSubpath();
    
    // Create graphics item with the pin shape
    userLocationMarker = new QGraphicsPathItem(pinPath);
    userLocationMarker->setBrush(QBrush(QColor(230, 30, 30)));  // Red pin
    userLocationMarker->setPen(QPen(QColor(180, 20, 20), 2));  // Darker red border
    userLocationMarker->setZValue(20);  // Above everything else
    scene->addItem(userLocationMarker);
    
    // Add white circle in the center of the pin (like the hole in location pins)
    QGraphicsEllipseItem *innerCircle = new QGraphicsEllipseItem(
        centerX - 6, centerY - pinHeight + 8, 12, 12, userLocationMarker);
    innerCircle->setBrush(QBrush(QColor(255, 255, 255)));
    innerCircle->setPen(Qt::NoPen);
    
    // Center the view on user location
    centerOn(pos);
}

void CityMapView::enterSelectionMode()
{
    selectionMode = true;
    setCursor(pinCursor);
    // Ensure markers accept hover during selection
    for (LocationMarkerItem *marker : locationMarkers)
    {
        marker->setAcceptHoverEvents(true);
    }
}

void CityMapView::exitSelectionMode()
{
    selectionMode = false;
    unsetCursor();
}

void CityMapView::buildScene()
{
    if (!city)
        return;

    Node *node = city->getFirstNode();
    if (!node)
        return;

    // Calculate bounds
    minX = node->x;
    maxX = node->x;
    minY = node->y;
    maxY = node->y;

    for (Node *walk = node; walk != nullptr; walk = walk->next)
    {
        minX = qMin(minX, walk->x);
        maxX = qMax(maxX, walk->x);
        minY = qMin(minY, walk->y);
        maxY = qMax(maxY, walk->y);
    }

    double width = maxX - minX;
    double height = maxY - minY;
    if (width <= 0.0) width = 1.0;
    if (height <= 0.0) height = 1.0;

    const double padding = 40.0;
    double viewW = viewport()->width() > 0 ? viewport()->width() : 1000.0;
    double viewH = viewport()->height() > 0 ? viewport()->height() : 700.0;

    scaleFactor = qMin((viewW - padding * 2.0) / width, (viewH - padding * 2.0) / height);
    if (scaleFactor <= 0.0) scaleFactor = 0.5;

    scene->clear();
    streets.clear();
    drawnEdges.clear();

    // Draw all edges directly from the graph
    drawAllEdges();
    
    // Calculate zone/colony/street bounds for labels
    calculateZoneAndColonyBounds();
    
    // Draw location emojis
    drawLocationEmojis();

    scene->setSceneRect(0.0, 0.0, width * scaleFactor + padding * 2.0, height * scaleFactor + padding * 2.0);

    // Render edges only
    drawDistrictLabel();
    updateVisibilityBasedOnZoom();
    updateLocationMarkerHoverState();

    qDebug() << "CityMapView: Built scene with edges from city graph";
}

void CityMapView::drawAllEdges()
{
    for (Node *fromNode = city->getFirstNode(); fromNode != nullptr; fromNode = fromNode->next)
    {
        QString fromType = QString::fromUtf8(fromNode->locationType).toLower();
        EdgeNode *edge = city->getNeighbors(fromNode->id);
        
        while (edge)
        {
            Node *toNode = city->getNode(edge->toNodeId);
            if (!toNode)
            {
                edge = edge->next;
                continue;
            }

            QString toType = QString::fromUtf8(toNode->locationType).toLower();
            
            // Skip edges that connect street nodes to location nodes
            bool fromIsStreet = fromType.contains("street") || fromType.contains("route") || fromType.contains("highway");
            bool fromIsLocation = !fromIsStreet && fromType != "no zone";
            bool toIsStreet = toType.contains("street") || toType.contains("route") || toType.contains("highway");
            bool toIsLocation = !toIsStreet && toType != "no zone";
            
            // Skip location-to-street edges
            if ((fromIsStreet && toIsLocation) || (fromIsLocation && toIsStreet))
            {
                edge = edge->next;
                continue;
            }

            // Create unique edge key to avoid duplicates (undirected)
            QString fromId = QString::fromUtf8(fromNode->id);
            QString toId = QString::fromUtf8(edge->toNodeId);
            QString edgeKey = fromId < toId ? fromId + "|" + toId : toId + "|" + fromId;
            
            if (drawnEdges.contains(edgeKey))
            {
                edge = edge->next;
                continue;
            }
            drawnEdges.insert(edgeKey);

            // Draw edge as a street
            QPointF p1 = mapToScreen(fromNode->x, fromNode->y);
            QPointF p2 = mapToScreen(toNode->x, toNode->y);

            // Draw road border (light gray #E0E0E0)
            QPen borderPen(QColor(224, 224, 224));
            borderPen.setWidthF(streetWidth + 1.0);
            borderPen.setCapStyle(Qt::RoundCap);
            borderPen.setJoinStyle(Qt::RoundJoin);
            scene->addLine(QLineF(p1, p2), borderPen)->setZValue(0);

            // Draw road (white #FFFFFF)
            QPen streetPen(QColor(255, 255, 255));
            streetPen.setWidthF(streetWidth);
            streetPen.setCapStyle(Qt::RoundCap);
            streetPen.setJoinStyle(Qt::RoundJoin);
            scene->addLine(QLineF(p1, p2), streetPen)->setZValue(1);

            // Store district from first edge
            if (currentDistrict.isEmpty())
            {
                currentDistrict = QString::fromUtf8(fromNode->zone);
            }

            edge = edge->next;
        }
    }
    
    // Now draw all location nodes as colored rectangles with dark grey labels
    qDebug() << "Drawing location nodes...";
    
    int locCount = 0;
    locationMarkers.clear();
    
    // Draw all locations as rectangles with names always visible
    for (Node *node = city->getFirstNode(); node != nullptr; node = node->next)
    {
        QString nodeType = QString::fromUtf8(node->locationType).trimmed().toLower();
        QString locationName = QString::fromUtf8(node->locationName).trimmed();
        QColor color;
        
        if (nodeType == "home" || nodeType == "house")
            color = QColor(100, 150, 255);  // Blue
        else if (nodeType == "mall")
            color = QColor(255, 165, 0);    // Orange
        else if (nodeType == "hospital")
            color = QColor(255, 100, 100); // Red
        else if (nodeType == "school")
            color = QColor(100, 200, 100); // Green
        else
            continue;
        
        QPointF pos = mapToScreen(node->x, node->y);
        
        // Create custom location marker item with hover support
        QString nodeId = QString::fromUtf8(node->id);
        LocationMarkerItem *locItem = new LocationMarkerItem(pos.x(), pos.y(), nodeId, locationName, nodeType);
        locItem->setBrush(QBrush(color));
        locItem->setPen(QPen(QColor(50, 50, 50), 1));
        locItem->setZValue(12);
        locItem->setAcceptHoverEvents(false);  // Initially disabled
        scene->addItem(locItem);
        
        locationMarkers.append(locItem);
        locCount++;
    }
    
    qDebug() << "Location nodes drawn:" << locCount;
}

void CityMapView::updateVisibilityBasedOnZoom()
{
    updateLabelsBasedOnView();
}

void CityMapView::updateLocationMarkerHoverState()
{
    // Enable hover only when zoomed to zone/colony level (when showing street labels)
    int visibleColonies = countVisibleColonies();
    bool enableHover = (visibleColonies >= 1);
    
    for (LocationMarkerItem *marker : locationMarkers)
    {
        marker->setAcceptHoverEvents(enableHover);
    }
}

void CityMapView::drawTextAlongPath(const QPainterPath &path, const QString &text)
{
    if (path.length() < 30)
        return;

    qreal pathLength = path.length();
    qreal startOffset = pathLength * 0.2;
    
    QFont font("Arial", 5);
    font.setBold(true);
    
    QGraphicsTextItem *textItem = new QGraphicsTextItem(text);
    textItem->setFont(font);
    textItem->setDefaultTextColor(QColor(64, 64, 64));
    
    qreal textPercent = path.percentAtLength(startOffset);
    QPointF textPos = path.pointAtPercent(textPercent);
    qreal angle = path.angleAtPercent(textPercent);
    
    textItem->setPos(textPos);
    textItem->setRotation(-angle);
    textItem->setZValue(3);
    scene->addItem(textItem);
}

QPointF CityMapView::mapToScreen(double x, double y) const
{
    const double padding = 40.0;
    double sx = (x - minX) * scaleFactor + padding;
    double sy = (maxY - y) * scaleFactor + padding;
    return QPointF(sx, sy);
}

QString CityMapView::getStreetKey(Node *node) const
{
    return QString("%1_%2_%3")
        .arg(QString::fromUtf8(node->zone))
        .arg(QString::fromUtf8(node->colony))
        .arg(node->streetNo);
}

QColor CityMapView::getZoneColor(const QString &zone) const
{
    QString zoneKey = QString(zone).toLower();
    zoneKey.remove("zone").remove(" ");
    
    if (zoneKey == "1") return QColor(76, 175, 80);   // Green
    if (zoneKey == "2") return QColor(33, 150, 243);  // Blue
    if (zoneKey == "3") return QColor(255, 152, 0);   // Orange
    if (zoneKey == "4") return QColor(156, 39, 176);  // Purple
    return QColor(120, 120, 120);
}

QColor CityMapView::getLocationColor(const QString &locationType) const
{
    QString type = locationType.toLower();
    if (type.contains("home") || type.contains("house")) return QColor(100, 181, 246);
    if (type.contains("hospital")) return QColor(239, 83, 80);
    if (type.contains("mall")) return QColor(255, 193, 7);
    if (type.contains("school")) return QColor(102, 187, 106);
    return QColor(158, 158, 158);
}

QString CityMapView::getLocationIcon(const QString &locationType) const
{
    QString type = locationType.toLower();
    if (type.contains("home") || type.contains("house")) return "🏠";
    if (type.contains("hospital")) return "🏥";
    if (type.contains("mall")) return "🏬";
    if (type.contains("school")) return "🏫";
    if (type.contains("park")) return "🌳";
    if (type.contains("restaurant")) return "🍴";
    return "📍";
}

void CityMapView::drawLocationEmojis()
{
    // Locations are now drawn directly in drawAllEdges()
}

void CityMapView::wheelEvent(QWheelEvent *event)
{
    const double zoomInFactor = 1.15;
    const double zoomOutFactor = 1.0 / zoomInFactor;

    if (event->angleDelta().y() > 0)
    {
        scale(zoomInFactor, zoomInFactor);
        currentZoomLevel *= zoomInFactor;
    }
    else
    {
        scale(zoomOutFactor, zoomOutFactor);
        currentZoomLevel *= zoomOutFactor;
    }

    updateVisibilityBasedOnZoom();
    updateLocationMarkerHoverState();
    event->accept();
}

void CityMapView::mousePressEvent(QMouseEvent *event)
{
    if (selectionMode && event->button() == Qt::LeftButton)
    {
        QGraphicsItem *item = itemAt(event->pos());
        auto *marker = dynamic_cast<LocationMarkerItem*>(item);
        if (marker)
        {
            emit locationPicked(marker->locationId, marker->locationName);
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton)
    {
        isPanning = true;
        lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void CityMapView::mouseMoveEvent(QMouseEvent *event)
{
    if (isPanning)
    {
        QPoint delta = event->pos() - lastPanPos;
        lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CityMapView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CityMapView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateLabelsBasedOnView();
}

void CityMapView::drawDistrictLabel()
{
    // District label removed - will show zone labels instead
}

void CityMapView::calculateZoneAndColonyBounds()
{
    zoneBounds.clear();
    colonyBounds.clear();
    streetBounds.clear();
    
    // Initialize bounds structures
    QHash<QString, QList<QPointF>> zonePoints;
    QHash<QString, QList<QPointF>> colonyPoints;
    QHash<QString, QList<QPointF>> streetPoints;
    
    for (Node *n = city->getFirstNode(); n != nullptr; n = n->next)
    {
        QString zone = QString::fromUtf8(n->zone).trimmed();
        QString colony = QString::fromUtf8(n->colony).trimmed();
        int streetNo = n->streetNo;
        
        if (zone.isEmpty() || zone == "No Zone")
            continue;
            
        QPointF pos = mapToScreen(n->x, n->y);
        
        // Track zone points
        zonePoints[zone].append(pos);
        
        // Track colony points
        if (!colony.isEmpty())
        {
            QString colonyKey = zone + "_" + colony;
            colonyPoints[colonyKey].append(pos);
        }
        
        // Track street points
        if (streetNo > 0)
        {
            QString streetKey = QString("%1_%2_%3").arg(zone).arg(colony).arg(streetNo);
            streetPoints[streetKey].append(pos);
        }
    }
    
    // Calculate zone bounds
    for (auto it = zonePoints.constBegin(); it != zonePoints.constEnd(); ++it)
    {
        const QList<QPointF> &points = it.value();
        if (points.isEmpty()) continue;
        
        BoundingBox box;
        box.minX = box.maxX = points.first().x();
        box.minY = box.maxY = points.first().y();
        box.name = it.key();
        
        for (const QPointF &p : points)
        {
            box.minX = qMin(box.minX, p.x());
            box.maxX = qMax(box.maxX, p.x());
            box.minY = qMin(box.minY, p.y());
            box.maxY = qMax(box.maxY, p.y());
        }
        
        box.center = QPointF((box.minX + box.maxX) / 2, (box.minY + box.maxY) / 2);
        zoneBounds[it.key()] = box;
    }
    
    // Calculate colony bounds
    for (auto it = colonyPoints.constBegin(); it != colonyPoints.constEnd(); ++it)
    {
        const QList<QPointF> &points = it.value();
        if (points.isEmpty()) continue;
        
        BoundingBox box;
        box.minX = box.maxX = points.first().x();
        box.minY = box.maxY = points.first().y();
        
        // Extract colony name from key
        QString key = it.key();
        int underscorePos = key.indexOf('_');
        box.name = underscorePos >= 0 ? key.mid(underscorePos + 1) : key;
        
        for (const QPointF &p : points)
        {
            box.minX = qMin(box.minX, p.x());
            box.maxX = qMax(box.maxX, p.x());
            box.minY = qMin(box.minY, p.y());
            box.maxY = qMax(box.maxY, p.y());
        }
        
        box.center = QPointF((box.minX + box.maxX) / 2, (box.minY + box.maxY) / 2);
        colonyBounds[it.key()] = box;
    }
    
    // Calculate street bounds
    for (auto it = streetPoints.constBegin(); it != streetPoints.constEnd(); ++it)
    {
        const QList<QPointF> &points = it.value();
        if (points.isEmpty()) continue;
        
        BoundingBox box;
        box.minX = box.maxX = points.first().x();
        box.minY = box.maxY = points.first().y();
        
        // Extract street number from key
        QString key = it.key();
        QStringList parts = key.split('_');
        box.name = parts.size() >= 3 ? "St " + parts[2] : key;
        
        for (const QPointF &p : points)
        {
            box.minX = qMin(box.minX, p.x());
            box.maxX = qMax(box.maxX, p.x());
            box.minY = qMin(box.minY, p.y());
            box.maxY = qMax(box.maxY, p.y());
        }
        
        box.center = QPointF((box.minX + box.maxX) / 2, (box.minY + box.maxY) / 2);
        streetBounds[it.key()] = box;
    }
}

void CityMapView::initializePOIGlyphs()
{
    poiGlyphMap["hospital"] = "H";
    poiGlyphMap["restaurant"] = "R";
    poiGlyphMap["mall"] = "S";
    poiGlyphMap["school"] = "S";
    poiGlyphMap["home"] = "H";
    poiGlyphMap["park"] = "P";
}

QString CityMapView::getPOIGlyph(const QString &locationType) const
{
    QString type = locationType.toLower();
    for (auto it = poiGlyphMap.constBegin(); it != poiGlyphMap.constEnd(); ++it)
    {
        if (type.contains(it.key()))
            return it.value();
    }
    return "•";
}

// LocationBlockItem hover implementation
void LocationBlockItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (!locationName.isEmpty())
    {
        tooltipItem = new QGraphicsTextItem(locationName, this);
        QFont font("Arial", 7);
        font.setBold(true);
        tooltipItem->setFont(font);
        tooltipItem->setDefaultTextColor(QColor(40, 40, 40));
        tooltipItem->setPos(15, -8);
        tooltipItem->setZValue(25);
        
        // Add background rectangle for tooltip
        QRectF textRect = tooltipItem->boundingRect();
        QGraphicsRectItem *bg = new QGraphicsRectItem(textRect, tooltipItem);
        bg->setBrush(QBrush(QColor(255, 255, 255, 230)));
        bg->setPen(QPen(QColor(200, 200, 200), 0.5));
        bg->setZValue(-1);
    }
    // Highlight with purple on hover
    setBrush(QBrush(QColor(123, 97, 255)));
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void LocationBlockItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (tooltipItem)
    {
        delete tooltipItem;
        tooltipItem = nullptr;
    }
    // Return to dark gray
    setBrush(QBrush(QColor(96, 96, 96)));
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

QRectF CityMapView::getViewportRect() const
{
    return mapToScene(viewport()->rect()).boundingRect();
}

int CityMapView::countVisibleZones() const
{
    QRectF viewRect = getViewportRect();
    int count = 0;
    
    for (auto it = zoneBounds.constBegin(); it != zoneBounds.constEnd(); ++it)
    {
        const BoundingBox &box = it.value();
        QRectF zoneRect(box.minX, box.minY, box.maxX - box.minX, box.maxY - box.minY);
        if (viewRect.intersects(zoneRect))
            count++;
    }
    
    return count;
}

int CityMapView::countVisibleColonies() const
{
    QRectF viewRect = getViewportRect();
    int count = 0;
    
    for (auto it = colonyBounds.constBegin(); it != colonyBounds.constEnd(); ++it)
    {
        const BoundingBox &box = it.value();
        QRectF colonyRect(box.minX, box.minY, box.maxX - box.minX, box.maxY - box.minY);
        if (viewRect.intersects(colonyRect))
            count++;
    }
    
    return count;
}

void CityMapView::clearAllLabels()
{
    for (QGraphicsTextItem *label : zoneLabels)
        delete label;
    for (QGraphicsTextItem *label : colonyLabels)
        delete label;
    for (QGraphicsTextItem *label : streetLabels)
        delete label;
    for (QGraphicsTextItem *label : locationEmojis)
        delete label;
        
    zoneLabels.clear();
    colonyLabels.clear();
    streetLabels.clear();
    locationEmojis.clear();
}

void CityMapView::updateLabelsBasedOnView()
{
    clearAllLabels();
    
    int visibleZones = countVisibleZones();
    int visibleColonies = countVisibleColonies();
    QRectF viewRect = getViewportRect();
    
    // Show zone labels when multiple zones visible (zoomed out)
    if (visibleZones >= 2)
    {
        for (auto it = zoneBounds.constBegin(); it != zoneBounds.constEnd(); ++it)
        {
            const BoundingBox &box = it.value();
            if (!viewRect.contains(box.center))
                continue;
                
            QGraphicsTextItem *label = new QGraphicsTextItem(box.name);
            QFont font("Arial", 18);
            font.setBold(true);
            label->setFont(font);
            label->setDefaultTextColor(QColor(80, 80, 80, 180));
            
            QRectF textRect = label->boundingRect();
            label->setPos(box.center.x() - textRect.width() / 2, box.center.y() - textRect.height() / 2);
            label->setZValue(15);
            scene->addItem(label);
            zoneLabels.append(label);
        }
    }
    // Show street names on edges when viewing colonies
    else if (visibleColonies >= 1)
    {
        // Draw street names on all streets in visible colonies
        drawStreetNamesOnEdges(viewRect);
    }
}

void CityMapView::drawStreetNamesOnEdges(const QRectF &viewRect)
{
    // Track which streets we've already labeled to avoid duplicates
    QSet<QString> labeledStreets;
    
    // First, collect all nodes for each street
    QHash<QString, QList<Node*>> streetNodes;
    
    for (Node *node = city->getFirstNode(); node != nullptr; node = node->next)
    {
        QString nodeType = QString::fromUtf8(node->locationType).trimmed();
        bool isStreet = nodeType.contains("street", Qt::CaseInsensitive) ||
                       nodeType.contains("route", Qt::CaseInsensitive) ||
                       nodeType.contains("highway", Qt::CaseInsensitive);
        
        if (!isStreet) continue;
        
        // Skip highway zone streets
        QString zone = QString::fromUtf8(node->zone).trimmed();
        if (zone.contains("highway", Qt::CaseInsensitive))
            continue;
        
        QString streetKey = QString("%1_%2_%3").arg(node->zone).arg(node->colony).arg(node->streetNo);
        streetNodes[streetKey].append(node);
    }
    
    // Now label each street
    for (auto it = streetNodes.constBegin(); it != streetNodes.constEnd(); ++it)
    {
        const QString &streetKey = it.key();
        const QList<Node*> &nodes = it.value();
        
        if (nodes.isEmpty()) continue;
        
        // Get the first node and the 10th node (or last if fewer than 10)
        Node *firstNode = nodes[0];
        Node *tenthNode = nodes.size() >= 10 ? nodes[9] : nodes[nodes.size() - 1];
        
        QPointF firstPos = mapToScreen(firstNode->x, firstNode->y);
        QPointF tenthPos = mapToScreen(tenthNode->x, tenthNode->y);
        
        // Check if either position is visible in viewport
        QRectF labelRect(qMin(firstPos.x(), tenthPos.x()), qMin(firstPos.y(), tenthPos.y()),
                        qAbs(tenthPos.x() - firstPos.x()), qAbs(tenthPos.y() - firstPos.y()));
        if (!viewRect.intersects(labelRect.adjusted(-100, -100, 100, 100)))
            continue;
        
        // Calculate center point between first and tenth node
        QPointF center = (firstPos + tenthPos) / 2.0;
        double dx = tenthPos.x() - firstPos.x();
        double dy = tenthPos.y() - firstPos.y();
        double angle = qAtan2(dy, dx) * 180.0 / M_PI;
        
        // Normalize angle to keep text readable (not upside down)
        if (angle > 90 || angle < -90)
            angle += 180;
        
        // Create street name
        QString streetName;
        QString colonyName = QString::fromUtf8(firstNode->colony).trimmed();
        
        if (firstNode->streetNo > 0)
        {
            if (!colonyName.isEmpty())
                streetName = QString("%1 - Street %2").arg(colonyName).arg(firstNode->streetNo);
            else
                streetName = QString("Street %1").arg(firstNode->streetNo);
        }
        else
            streetName = colonyName.isEmpty() ? QString::fromUtf8(firstNode->zone).trimmed() : colonyName;
        
        // Create the text label
        QGraphicsTextItem *label = new QGraphicsTextItem(streetName);
        QFont font("Segoe UI", 6);
        font.setBold(false);
        label->setFont(font);
        // Light grey color that complements white streets (#FFFFFF)
        label->setDefaultTextColor(QColor(180, 180, 180));
        
        // Position at center and rotate
        QRectF textRect = label->boundingRect();
        label->setPos(center.x() - textRect.width() / 2, center.y() - textRect.height() / 2);
        label->setTransformOriginPoint(textRect.width() / 2, textRect.height() / 2);
        label->setRotation(angle);
        label->setZValue(16);
        
        scene->addItem(label);
        streetLabels.append(label);
    }
}
