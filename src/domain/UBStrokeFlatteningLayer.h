/*
 * UBStrokeFlatteningLayer
 * A lightweight tiling layer that flattens vector stroke polygons
 * into cached pixmap tiles for fast GPU compositing under QOpenGLWidget.
 */

#pragma once

#include <QObject>
#include <QHash>
#include <QPoint>
#include <QPainterPath>
#include <QGraphicsPixmapItem>

class UBGraphicsScene;
class UBGraphicsStrokesGroup;
class UBGraphicsPolygonItem;

class UBStrokeTileItem : public QGraphicsPixmapItem
{
public:
    using QGraphicsPixmapItem::QGraphicsPixmapItem;
    enum { Type = UserType + 0x7a11 }; // arbitrary unique type id
    int type() const override { return Type; }
};

class UBStrokeFlatteningLayer : public QObject
{
    Q_OBJECT
public:
    explicit UBStrokeFlatteningLayer(UBGraphicsScene* scene, int tileSize = 1024, QObject* parent = nullptr);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool en) { m_enabled = en; }

    // Draw a stroke group (its child polygons) into tiles and hide the group
    void flattenGroup(class UBGraphicsStrokesGroup* group);

    // Apply eraser path to tiles (transparent punch-through)
    void erase(const QPainterPath& eraserPath);

    // Full rebuild from current scene stroke polygons (used after undo/redo)
    void rebuildFromScene();

private:
    UBGraphicsScene* m_scene;
    int m_tileSize;
    bool m_enabled;
    qreal m_z;

    // Keyed by tile index (ix,iy)
    QHash<QPoint, UBStrokeTileItem*> m_tiles;

    UBStrokeTileItem* ensureTile(const QPoint& ti);
    static QPoint tileIndexFor(const QPointF& p, int tileSize);
    static QRectF tileRect(const QPoint& ti, int tileSize);
    void drawPolygonItemToTiles(UBGraphicsPolygonItem* item);
};
