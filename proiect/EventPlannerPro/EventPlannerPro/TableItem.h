#pragma once

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QList>
#include <QGraphicsTextItem>
#include "ChairItem.h"

class TableItem : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT

public:
    TableItem(int id, int nrScaune, qreal x, qreal y, qreal w, qreal h);
    int getId() const { return tableId; }
    int getMaxChairs() const { return maxChairs; }

    QStringList getGuestsData() const;
    void setGuestsData(const QStringList& guests);
    void removeGuestByName(const QString& name);

    static int GlobalMinDist;

signals:
    void tableDeleted(TableItem* table);
    void guestSeated(QString name, int tableId);
    void guestUnseated(QString name);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private slots:
    void onChairGuestRemoved(QString name);

private:
    int tableId;
    int maxChairs;
    QList<ChairItem*> chairs;
    QGraphicsTextItem* textId;
    QPointF originalPos;

    void arrangeChairs();
    void checkEmptyTable();
};