#pragma once
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QPen>
#include <QObject>

class ChairItem : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    ChairItem(QGraphicsItem* parent = nullptr);

    void setGuest(const QString& name);
    void removeGuest();
    bool isOccupied() const;
    QString getGuestName() const;

signals:
    void guestRemoved(QString name);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QString guestName;
    bool occupied;
};