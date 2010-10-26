/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QDebug>
#include <QDropEvent>
#include <QGraphicsLinearLayout>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>

#include "chat_panel.h"

#define ICON_WIDTH 48
#define BUTTON_WIDTH 48
#define BORDER_RADIUS 8

class ChatPanelPrivate
{
public:
    void updateTitle();

    QVBoxLayout *header;
    QHBoxLayout *hbox;
    QHBoxLayout *widgets;
    QPushButton *attachButton;
    QPushButton *closeButton;
    QLabel *helpLabel;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QString windowExtra;
    QString windowStatus;
    QList< QPair<QString, int> > notificationQueue;

    ChatPanel *q;
};

void ChatPanelPrivate::updateTitle()
{
    nameLabel->setText(QString("<b>%1</b> %2<br/>%3").arg(q->windowTitle(),
        windowStatus, windowExtra));
}

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent),
    d(new ChatPanelPrivate)
{
    bool check;
    d->q = this;

    d->attachButton = new QPushButton;
    d->attachButton->setFlat(true);
    d->attachButton->setMaximumWidth(32);
    d->attachButton->setIcon(QIcon(":/add.png"));
    d->attachButton->hide();
    check = connect(d->attachButton, SIGNAL(clicked()),
                    this, SIGNAL(attachPanel()));
    Q_ASSERT(check);

    d->closeButton = new QPushButton;
    d->closeButton->setFlat(true);
    d->closeButton->setMaximumWidth(32);
    d->closeButton->setIcon(QIcon(":/close.png"));
    check = connect(d->closeButton, SIGNAL(clicked()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    d->iconLabel = new QLabel;
    d->nameLabel = new QLabel;

    d->hbox = new QHBoxLayout;
    d->hbox->addSpacing(16);
    d->hbox->addWidget(d->nameLabel);
    d->hbox->addStretch();
    d->hbox->addWidget(d->iconLabel);
    d->hbox->addWidget(d->attachButton);
    d->hbox->addWidget(d->closeButton);

    // assemble header
    d->header = new QVBoxLayout;
    d->header->setMargin(0);
    d->header->setSpacing(10);
    d->header->addLayout(d->hbox);

    d->helpLabel = new QLabel;
    d->helpLabel->setWordWrap(true);
    d->helpLabel->setOpenExternalLinks(true);
    d->helpLabel->hide();
    d->header->addWidget(d->helpLabel);

    d->widgets = new QHBoxLayout;
    d->widgets->addStretch();
    d->header->addLayout(d->widgets);

    setMinimumWidth(300);
}

ChatPanel::~ChatPanel()
{
    delete d;
}

void ChatPanel::addWidget(ChatPanelWidget *widget)
{
    Q_UNUSED(widget);
}

/** Return the type of entry to add to the roster.
 */
ChatRosterItem::Type ChatPanel::objectType() const
{
    return ChatRosterItem::Other;
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowExtra(const QString &extra)
{
    d->windowExtra = extra;
    d->updateTitle();
}

/** Sets the window's help text.
 *
 * @param help
 */
void ChatPanel::setWindowHelp(const QString &help)
{
    d->helpLabel->setText(help);
#ifndef WILINK_EMBEDDED
    if (help.isEmpty())
        d->helpLabel->hide();
    else
        d->helpLabel->show();
#endif
}

/** When the window icon is set, update the header icon.
 *
 * @param icon
 */
void ChatPanel::setWindowIcon(const QIcon &icon)
{
    QWidget::setWindowIcon(icon);
    const QSize actualSize = icon.actualSize(QSize(64, 64));
    d->iconLabel->setPixmap(icon.pixmap(actualSize));
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowStatus(const QString &status)
{
    d->windowStatus = status;
    d->updateTitle();
}

/** When the window title is set, update the header text.
 *
 * @param title
 */
void ChatPanel::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
    d->updateTitle();
}

/** Return a layout object for the panel header.
 */
QLayout* ChatPanel::headerLayout()
{
    return d->header;
}

void ChatPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ParentChange)
    {
        if (parent())
        {
            layout()->setMargin(0);
            d->attachButton->hide();
            d->closeButton->show();
        } else {
            layout()->setMargin(6);
            d->attachButton->show();
            d->closeButton->hide();
        }
    }
    QWidget::changeEvent(event);
}

void ChatPanel::closeEvent(QCloseEvent *event)
{
    emit hidePanel();
    QWidget::closeEvent(event);
}

bool ChatPanel::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *event = static_cast<QDragEnterEvent*>(e);
        event->acceptProposedAction();
        return true;
    }
    else if (e->type() == QEvent::DragLeave)
    {
        return true;
    }
    else if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop)
    {
        QDropEvent *event = static_cast<QDropEvent*>(e);
        event->ignore();
        emit dropPanel(event);
        return true;
    }
    return false;
}

void ChatPanel::filterDrops(QWidget *widget)
{
    widget->setAcceptDrops(true);
    widget->installEventFilter(this);
}

void ChatPanel::queueNotification(const QString &message, int options)
{
    d->notificationQueue << qMakePair(message, options);
    QTimer::singleShot(0, this, SLOT(sendNotifications()));
}

void ChatPanel::sendNotifications()
{
    while (!d->notificationQueue.isEmpty())
    {
        QPair<QString, int> entry = d->notificationQueue.takeFirst();
        emit notifyPanel(entry.first, entry.second);
    }
}

/** Creates a new ChatPanelBar instance.
 *
 * @param view
 */
ChatPanelBar::ChatPanelBar(QGraphicsView *view)
    : m_view(view)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(5, 5);
    effect->setBlurRadius(10);
    setGraphicsEffect(effect);
    m_view->viewport()->installEventFilter(this);

    m_delay = new QTimer(this);
    m_delay->setInterval(100);
    m_delay->setSingleShot(true);

    m_animation = new QPropertyAnimation(this, "size");
    m_animation->setEasingCurve(QEasingCurve::OutQuad);

    m_layout = new QGraphicsLinearLayout;
    m_layout->setContentsMargins(16, 8, 16, 8);
    setLayout(m_layout);
    setOpacity(0.9);

    bool check;
    check = connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                    this, SLOT(scrollChanged()));
    Q_ASSERT(check);

    check = connect(m_delay, SIGNAL(timeout()),
                    this, SLOT(trackView()));
    Q_ASSERT(check);
}

void ChatPanelBar::addWidget(ChatPanelWidget *widget)
{
    m_layout->addItem(widget);
    widget->appear();
}

bool ChatPanelBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize)
    {
        if (m_animation->state() == QAbstractAnimation::Running)
            trackView();
        else
            m_delay->start();
    }
    return false;
}

void ChatPanelBar::scrollChanged()
{
    if (m_layout->count())
        setPos(m_view->mapToScene(QPoint(0, 0)));
}

void ChatPanelBar::trackView()
{
    QSizeF newSize;
    newSize.setWidth(m_view->viewport()->width() - 1);
    newSize.setHeight(48);
    m_animation->setDuration(500);
    m_animation->setStartValue(size());
    m_animation->setEndValue(newSize);
    m_animation->start();
}

/** Creates a new ChatPanelWidget instance.
 *
 * @param parent
 */
ChatPanelWidget::ChatPanelWidget(QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_buttonEnabled(true)
{
    const QPalette palette = QApplication::palette();

    m_border = new QGraphicsPathItem(this);
    QLinearGradient gradient(0, 0, 0, 32);
    gradient.setColorAt(0, palette.color(QPalette::Light));
    gradient.setColorAt(0.6, palette.color(QPalette::Button));
    gradient.setColorAt(1, palette.color(QPalette::Mid));
    m_border->setBrush(gradient);
    m_border->setPen(QPen(palette.color(QPalette::Shadow), 1));

    m_buttonPath = new QGraphicsPathItem(this);
    m_buttonPath->setPen(QPen(palette.color(QPalette::Shadow), 1));
    m_buttonPixmap = new QGraphicsPixmapItem(this);

    m_icon = new QGraphicsPixmapItem(this);
}

/** Makes the widget appear.
 */
void ChatPanelWidget::appear()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

/** Returns the rectangle holding the widget's contents.
 */
QRectF ChatPanelWidget::contentsRect() const
{
    QRectF rect = geometry().adjusted(0, 1, -BUTTON_WIDTH - ICON_WIDTH, -2);
    rect.moveLeft(ICON_WIDTH);
    rect.moveTop(0);
    return rect;
}

/** Makes the widget disappear then deletes it.
 */
void ChatPanelWidget::disappear()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::InQuad);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->start();
    connect(animation, SIGNAL(finished()), this, SLOT(deleteLater()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ChatPanelWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_buttonEnabled &&
        m_buttonPath->path().contains(event->pos()))
    {
        const QPalette palette = QApplication::palette();
        QLinearGradient gradient(0, 0, 0, 32);
        gradient.setColorAt(0, palette.color(QPalette::Button));
        gradient.setColorAt(0.6, palette.color(QPalette::Mid));
        gradient.setColorAt(1, palette.color(QPalette::Dark));
        m_buttonPath->setBrush(gradient);
        m_buttonPixmap->setOffset(0, 2);
    }
    event->accept();
}

void ChatPanelWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    const QPalette palette = QApplication::palette();
    m_buttonPath->setBrush(Qt::NoBrush);
    m_buttonPixmap->setOffset(0, 0);
    if (m_buttonEnabled &&
        m_buttonPath->path().contains(event->pos()) &&
        m_buttonPath->path().contains(event->buttonDownPos(Qt::LeftButton)))
        emit buttonClicked();
    else if (contentsRect().contains(event->pos()) &&
             contentsRect().contains(event->buttonDownPos(Qt::LeftButton)))
        emit contentsClicked();
    event->accept();
}

/** Updates the widget geometry.
 */
void ChatPanelWidget::setGeometry(const QRectF &rect)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, rect.width() - 1, rect.height() - 1), BORDER_RADIUS, BORDER_RADIUS);
    m_border->setPath(path);

    QSizeF pixmapSize = m_icon->pixmap().size();
    m_icon->setPos((ICON_WIDTH - pixmapSize.width()) / 2,
        (rect.height() - pixmapSize.height()) / 2);

    QPainterPath buttonPath;
    buttonPath.addRoundedRect(QRectF(rect.width() - BUTTON_WIDTH, 0,
        BUTTON_WIDTH - 1, rect.height() - 1), BORDER_RADIUS, BORDER_RADIUS);
    m_buttonPath->setPath(buttonPath);

    pixmapSize = m_buttonPixmap->pixmap().size();
    m_buttonPixmap->setPos(
        rect.width() - (BUTTON_WIDTH + pixmapSize.width()) / 2,
        (rect.height() - pixmapSize.height()) / 2);
    QGraphicsWidget::setGeometry(rect);
}

/** Enable or disable the widget's button.
 *
 * @param enabled
 */
void ChatPanelWidget::setButtonEnabled(bool enabled)
{
    m_buttonEnabled = enabled;
}

/** Sets the widget's button pixmap.
 */
void ChatPanelWidget::setButtonPixmap(const QPixmap &pixmap)
{
    m_buttonPixmap->setPixmap(pixmap.scaledToWidth(BUTTON_WIDTH/2, Qt::SmoothTransformation));
}

/** Sets the widget's button tooltip.
 */
void ChatPanelWidget::setButtonToolTip(const QString &toolTip)
{
    m_buttonPath->setToolTip(toolTip);
}

/** Sets the widget's icon pixmap.
 */
void ChatPanelWidget::setIconPixmap(const QPixmap &pixmap)
{
    m_icon->setPixmap(pixmap);
}
