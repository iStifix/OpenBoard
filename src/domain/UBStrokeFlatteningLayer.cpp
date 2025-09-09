/*
 * UBStrokeFlatteningLayer
 */

#include "UBStrokeFlatteningLayer.h"

#include <QPainter>
#include <QGuiApplication>
#include <QScreen>

#include "UBGraphicsScene.h"
#include "UBItem.h"
#include "UBGraphicsStrokesGroup.h"
#include "UBGraphicsPolygonItem.h"
#include "core/UB.h"

static inline bool envIsFalse(const char* name)
{
    const QByteArray v = qgetenv(name);
    return (v == "0" || v.compare("false", Qt::CaseInsensitive) == 0);
}

UBStrokeFlatteningLayer::UBStrokeFlatteningLayer(UBGraphicsScene* scene, int tileSize, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_tileSize(tileSize)
    , m_enabled(true)
{
    if (!scene)
        m_enabled = false;

    // allow disabling via env
    if (envIsFalse("OPENBOARD_STROKE_FLATTEN"))
        m_enabled = false;

    bool ok = false;
    int ts = qEnvironmentVariableIntValue("OPENBOARD_STROKE_TILE_SIZE", &ok);
    if (ok && ts > 128)
        m_tileSize = ts;
}

QPoint UBStrokeFlatteningLayer::tileIndexFor(const QPointF& p, int tileSize)
{
    const int ix = static_cast<int>(std::floor(p.x() / tileSize));
    const int iy = static_cast<int>(std::floor(p.y() / tileSize));
    return QPoint(ix, iy);
}

QRectF UBStrokeFlatteningLayer::tileRect(const QPoint& ti, int tileSize)
{
    return QRectF(ti.x() * tileSize, ti.y() * tileSize, tileSize, tileSize);
}

UBStrokeTileItem* UBStrokeFlatteningLayer::ensureTile(const QPoint& ti)
{
    UBStrokeTileItem* tile = m_tiles.value(ti, nullptr);
    if (tile)
        return tile;

    QPixmap pm(m_tileSize, m_tileSize);
    pm.fill(Qt::transparent);

    tile = new UBStrokeTileItem(pm);
    // Place tile at its scene offset (top-left), no transform
    const QRectF r = tileRect(ti, m_tileSize);
    tile->setOffset(r.topLeft());
    tile->setTransformationMode(Qt::FastTransformation);
    tile->setData(UBGraphicsItemData::itemLayerType, QVariant(itemLayerType::DrawingItem));
    tile->setAcceptedMouseButtons(Qt::NoButton);
    tile->setFlag(QGraphicsItem::ItemIsSelectable, false);
    tile->setFlag(QGraphicsItem::ItemIsMovable, false);

    m_scene->addItem(tile);
    m_tiles.insert(ti, tile);
    return tile;
}

void UBStrokeFlatteningLayer::drawPolygonItemToTiles(UBGraphicsPolygonItem* item)
{
    if (!item || !m_enabled)
        return;

    // Determine intersecting tiles using sceneBoundingRect
    const QRectF sbr = item->sceneBoundingRect();
    const QPoint topLeft = tileIndexFor(sbr.topLeft(), m_tileSize);
    const QPoint bottomRight = tileIndexFor(sbr.bottomRight(), m_tileSize);

    for (int ty = topLeft.y(); ty <= bottomRight.y(); ++ty) {
        for (int tx = topLeft.x(); tx <= bottomRight.x(); ++tx) {
            const QPoint ti(tx, ty);
            UBStrokeTileItem* tile = ensureTile(ti);
            QPixmap pm = tile->pixmap();
            QPainter p(&pm);

            // Setup antialiasing based on env
            static const bool aa = !envIsFalse("OPENBOARD_STROKE_AA");
            if (aa)
                p.setRenderHint(QPainter::Antialiasing, true);

            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            // Transform to scene coordinates where (0,0) is tile top-left
            const QRectF tr = tileRect(ti, m_tileSize);
            p.translate(-tr.topLeft());
            p.setTransform(item->sceneTransform(), true);

            QPainterPath path;
            path.addPolygon(item->polygon());
            p.setPen(Qt::NoPen);
            p.setBrush(item->brush());
            p.drawPath(path);
            p.end();

            tile->setPixmap(pm);
            tile->update();
            // Raise touched tile to the top of drawing stack to preserve
            // expected layering of the most recent stroke across tile borders.
            UBGraphicsItem::assignZValue(tile, tile->zValue() + 1);
        }
    }
}

void UBStrokeFlatteningLayer::flattenGroup(UBGraphicsStrokesGroup* group)
{
    if (!m_enabled || !group)
        return;

    // Draw each child polygon to relevant tiles
    const auto children = group->childItems();
    for (QGraphicsItem* ch : children) {
        auto* pi = qgraphicsitem_cast<UBGraphicsPolygonItem*>(ch);
        if (!pi)
            continue;
        drawPolygonItemToTiles(pi);
    }

    // Hide the original group to avoid redraw overhead; keep it for undo/redo
    group->setVisible(false);
}

void UBStrokeFlatteningLayer::erase(const QPainterPath& eraserPath)
{
    if (!m_enabled)
        return;

    const QRectF br = eraserPath.boundingRect();
    const QPoint topLeft = tileIndexFor(br.topLeft(), m_tileSize);
    const QPoint bottomRight = tileIndexFor(br.bottomRight(), m_tileSize);

    for (int ty = topLeft.y(); ty <= bottomRight.y(); ++ty) {
        for (int tx = topLeft.x(); tx <= bottomRight.x(); ++tx) {
            const QPoint ti(tx, ty);
            UBStrokeTileItem* tile = m_tiles.value(ti, nullptr);
            if (!tile)
                continue;

            QPixmap pm = tile->pixmap();
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setCompositionMode(QPainter::CompositionMode_Clear);

            const QRectF tr = tileRect(ti, m_tileSize);
            QPainterPath localPath = eraserPath;
            QTransform t; t.translate(-tr.left(), -tr.top());
            localPath = t.map(localPath);
            p.fillPath(localPath, Qt::transparent);
            p.end();

            tile->setPixmap(pm);
            tile->update();
        }
    }
}

void UBStrokeFlatteningLayer::rebuildFromScene()
{
    if (!m_enabled || !m_scene)
        return;

    // Clear existing tiles
    for (auto it = m_tiles.begin(); it != m_tiles.end(); ++it) {
        QPixmap pm = it.value()->pixmap();
        pm.fill(Qt::transparent);
        it.value()->setPixmap(pm);
        it.value()->update();
    }

    // Draw all stroke polygon items currently present in the scene
    const auto allItems = m_scene->items();
    for (QGraphicsItem* gi : allItems) {
        auto* pi = qgraphicsitem_cast<UBGraphicsPolygonItem*>(gi);
        if (!pi)
            continue;
        drawPolygonItemToTiles(pi);
    }
}
