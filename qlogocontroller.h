#ifndef QLOGOCONTROLLER_H
#define QLOGOCONTROLLER_H

#include "logocontroller.h"
#include "constants.h"
#include <QDataStream>
#include <QFile>
#include <QFont>
#include "inputqueue.h"
#include "datum.h"

class QLogoController : public LogoController
{
    InputQueue messageQueue;
    message_t getMessage();
    void waitForMessage(message_t expectedType);

    // Return values from getMessage()
    QString rawLine;
    QChar rawChar;

    // Text returned from editor winow
    QString editorText;

    double minPensize;
    double maxPensize;
    double penSize;

    double xbound;
    double ybound;

    QStringList allFontNames;
    QString textFontName;
    qreal textFontSize;

    QString labelFontName;
    qreal labelFontSize;

public:
    QLogoController(QObject *parent = 0);
    ~QLogoController();
    void systemStop();

    void initialize();

    void printToConsole(const QString &s);
    DatumP readRawlineWithPrompt(const QString &prompt);
    DatumP readchar();
    const QString editText(const QString startText);

    void setTurtlePos(const QMatrix4x4 &newTurtlePos);
    void setTurtleIsVisible(bool isVisible);
    void drawLine(const QVector3D &start, const QVector3D &end, const QColor &startColor, const QColor &endColor);
    void drawPolygon(const QList<QVector3D> &points, const QList<QColor> &colors);
    void clearScreen();
    void drawLabel(const QString &, const QVector3D &, const QColor &);
    void setCanvasBackgroundColor(QColor);

    void setBounds(double x, double y);
    double boundX() { return xbound; }
    double boundY() { return ybound; }

    bool isPenSizeValid(double candidate) { return ((candidate >= minPensize) && (candidate <= maxPensize)); }

    void setTextFontSize(double aSize);
    double getTextFontSize();
    const QString getTextFontName();
    void setTextFontName(const QString aFontName);
    const QStringList getAllFontNames() { return allFontNames; }

    void setLabelFontSize(double aSize);
    double getLabelFontSize();
    const QString getLabelFontName();
    void setLabelFontName(const QString &aName);

    void setPensize(double);
    void mwait(unsigned long msecs);
    void clearScreenText();
};

#endif // QLOGOCONTROLLER_H
