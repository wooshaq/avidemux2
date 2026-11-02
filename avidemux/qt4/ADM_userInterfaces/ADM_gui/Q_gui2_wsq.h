/***************************************************************************
    Avidemux WsQ UI additions (theme, wrapping navigation bar, panel rail)
 ***************************************************************************/
#ifndef Q_gui2_wsq_h
#define Q_gui2_wsq_h

#include <QLayout>
#include <QList>
#include <QObject>
#include <QPair>
#include <QRect>
#include <QAction>
#include <QDockWidget>

#define ADM_QT_THEME_WSQ 3

/**
    \class WsqFlowLayout
    \brief left-to-right layout that wraps items to further rows when narrow
*/
class WsqFlowLayout : public QLayout
{
  public:
    explicit WsqFlowLayout(QWidget *parent, int hSpacing, int vSpacing);
    ~WsqFlowLayout();

    void addItem(QLayoutItem *item) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

  private:
    int doLayout(const QRect &rect, bool testOnly) const;
    QList<QLayoutItem *> itemList;
    int hSpace;
    int vSpace;
};

/**
    \class WsqHelper
    \brief event filter keeping the navigation dock height and rail actions in sync
*/
class WsqHelper : public QObject
{
  public:
    explicit WsqHelper(QObject *parent) : QObject(parent)
    {
    }
    bool eventFilter(QObject *watched, QEvent *event) override;
    void updateNavHeight(void);

    QDockWidget *navDock = nullptr;
    QWidget *navContents = nullptr;
    QList<QPair<QObject *, QAction *>> mirrors;

  private:
    bool pending = false;
};

#endif
