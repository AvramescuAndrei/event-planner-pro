#include "ChairItem.h"
#include <QToolTip>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>

ChairItem::ChairItem(QGraphicsItem* parent)
    : QObject(), QGraphicsEllipseItem(parent), occupied(false)
{
    setRect(0, 0, 20, 20);
    setBrush(QBrush(Qt::gray));
    setPen(QPen(Qt::black));

    setAcceptHoverEvents(true);
    setToolTip("GOL");
}

void ChairItem::setGuest(const QString& name)
{
    guestName = name;
    occupied = true;
    setBrush(QBrush(Qt::black));
    setToolTip(name);
}

void ChairItem::removeGuest()
{
    QString oldName = guestName;
    guestName.clear();
    occupied = false;
    setBrush(QBrush(Qt::gray));
    setToolTip("GOL");

    emit guestRemoved(oldName);
}

bool ChairItem::isOccupied() const { return occupied; }
QString ChairItem::getGuestName() const { return guestName; }

void ChairItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (occupied) setToolTip(guestName);
    else setToolTip("GOL");

    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void ChairItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if (occupied) {
        QMenu menu;

        QAction* infoAction = menu.addAction("Invitat: " + guestName);

        menu.addSeparator();

        QAction* removeAction = menu.addAction("Sterge invitat");

        QAction* selectedAction = menu.exec(event->screenPos());

        if (selectedAction == removeAction) {
            removeGuest();
        }
    }
}