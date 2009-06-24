/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __DRAWOBS_H__
#define __DRAWOBS_H__
/*
 * draw3
 * SZARP
 
 *
 * $Id$
 */

#include <wx/datetime.h>
#include <list>

class Draw;

/**Object implemeting following interface and attached to a @see Draw 
 * is notified about various event occuring in @see Draw object*/
class DrawObserver {
	public:
	/**Attaches to given draw
	 * @param draw @see Draw to attach*/
	virtual void Attach(Draw *draw);
	/**Detaches from @see Draw draw
	 * @param Draw draw to detach from*/
	virtual void Detach(Draw *draw);
	/**Informs object that displayed start time changed*/
	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_date) {};
	/**Informss object that new data appeared at given index position*/
	virtual void NewData(Draw* draw, int idx) {};
	/**Informss object that staitical values has changed.*/
	virtual void StatsChanged(Draw *draw) {};
	/**Informs object that current probe for draw has changed*/
	virtual void CurrentProbeChanged(Draw *draw, int pi, int ni, int d) {};
	/**Informs object that @see Draw has been selected*/
	virtual void DrawSelected(Draw *draw) {};
	/**Informs object that @see Draw has been selected*/
	virtual void DrawDeselected(Draw *draw) {};
	/**Informs object that current @see Draw has changed it's draw info*/
	virtual void DrawInfoChanged(Draw *draw) {};
	/**Informs object that current @see Draw has changed it's draw info*/
	virtual void FilterChanged(Draw *draw) {};
	/**Informs object that current @see Draw has changed it's period type*/
	virtual void PeriodChanged(Draw *draw, PeriodType period) {};
	/**Informs object that current @see Draw has changed it's enabled attribute*/
	virtual void EnableChanged(Draw *draw) {};

	virtual ~DrawObserver() = 0;
};

#endif 
