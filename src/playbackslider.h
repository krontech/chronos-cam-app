/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#ifndef PLAYBACKSLIDER
#define PLAYBACKSLIDER


#include <QSlider>

class PlaybackSlider : public QSlider
{
	Q_OBJECT
public:
	PlaybackSlider(QWidget * parent = 0);
	~PlaybackSlider();
	void setHighlightRegion(int start, int end);
	void appendRegionToList();
	void removeLastRegionFromList();
protected:
	void paintEvent(QPaintEvent *ev);

private:
	int highlightRegionStartFrame = 0;
	int highlightRegionEndFrame = 0;
	QList<QRect> previouslySavedRegions;
	QRect newSaveRegion;
	unsigned int currentColorIndex;
	QColor colorArray[9];
};


#endif // PlaybackSlider

