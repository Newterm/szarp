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

#ifndef EKSTRAKTOR_IDS_H
#define EKSTRAKTOR_IDS_H

#define EKSTRAKTOR_TITLEBAR _("SZARP Extractor")

#define DEFAULT_POSITION_X 100
#define DEFAULT_POSITION_Y 100
#define DEFAULT_HEIGHT 700
#define DEFAULT_WIDTH 500

enum EkstraktorMainWindowIDs { 
	ID_ChooseParameters = wxID_HIGHEST + 50,
	ID_ReadParamListFromFile, 
	ID_WriteParamListToFile, 
	ID_WriteResults, 
	ID_WriteAvailableParamsToDisk, 
	ID_PrintParamList, 
	ID_PrintPreview, 
	ID_Quit, 
	ID_Fonts,
	ID_Help,
	ID_About,
	ID_ContextHelp,

	ID_DotSeparator,
	ID_CommaSeparator,

	ID_Min10Period,
	ID_HourPeriod,
	ID_8HourPeriod,
	ID_DayPeriod,
	ID_WeekPeriod,
	ID_MonthPeriod,

	ID_SeparatorChange,
	ID_Minimize, 
	ID_TimeStep,

	ID_ChangeStartDate,
	ID_ChangeStopDate,

	ID_AddParametersBt, 
	ID_DeleteParametersBt, 
	ID_WriteResultsBt, 
	ID_WriteResultsDisketteBt,

	ID_ParSelectDlg,
	ID_TypeName,
	ID_SearchUp,
	ID_SearchDown,
	ID_ClickList,
	ID_Ok,
	ID_Cancel,
	ID_Help2
};

#define SZB_BUFFER_NUM 500

#endif
