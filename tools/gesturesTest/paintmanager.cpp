#include "paintmanager.h"

PaintManager::PaintManager(QGraphicsView *gestureView) : QObject(gestureView)
{
	mGestureScene = new QGraphicsScene(gestureView);
	gestureView->setScene(mGestureScene);
	mTimer = new QTimer(gestureView);
	QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(draw()));
	mTimer->start(250);
}

void PaintManager::drawPath(QPainter * painter, QList<QPoint> const & path)
{
	QPoint previousPoint(-1, -1);
	if (path.isEmpty())
		return;
	foreach (QPoint currentPoint, path)
	{
		if (previousPoint.x() >= 0 && previousPoint.y() >= 0)
			painter->drawLine(previousPoint, currentPoint);
		else
			painter->drawPoint(currentPoint);
		previousPoint = currentPoint;
	}

}

void PaintManager::drawGesture(QList<QPoint> const & polygon)
{
	mCurrentPointNumber = 0;
	mGestureScene->clear();
	mPath = QVector<QPoint>::fromList(polygon);
}

void PaintManager::draw()
{
	if (mPath.isEmpty())
		return;
	int verticeIndex = (mCurrentPointNumber / pointsAtSegment) % mPath.size();
	int segmentNumber = mCurrentPointNumber % pointsAtSegment + 1;
	if (verticeIndex == mPath.size() - 1)
	{
		mGestureScene->clear();
		mCurrentPointNumber = 0;
		return;
	}
	QPoint lastPaintedPoint = mPath.at(verticeIndex);
	QPoint nextPoint = mPath.at(verticeIndex + 1);
	QPoint currentPoint(coord(lastPaintedPoint.x(), nextPoint.x(), segmentNumber),
						coord(lastPaintedPoint.y(), nextPoint.y(), segmentNumber));
	mGestureScene->addLine(QLine(lastPaintedPoint, currentPoint), QPen(Qt::black));
	mCurrentPointNumber++;
}

int PaintManager::coord(int previous, int next, int part)
{
	return previous + (next - previous) * part / pointsAtSegment;
}

PaintManager::~PaintManager()
{
	delete mTimer;
}
