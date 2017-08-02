// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModHairpin
#include ":::ReportUtil:ReportUtil"
#include ":::Util:PlotUtil"


Static Function Main()
	// Description goes here
	//
	// Args:
	//		Arg 1:
	//		
	// Returns:
	//
	//
	String wave_base = "root:ForceCurves:SubFolders:X2017_6_13:Image2343"
	wave m_y = $(wave_base + "Force")
	String m_x_name = (wave_base + "Time")
	Wave m_separation_wave = $(wave_base + "Sep")
	//m_separation_wave[] = (m_separation_wave[p]) * 1e9
	Duplicate /O m_y $m_x_name
	Wave m_x = $(m_x_name)
	m_x[] = deltax(m_x)*p
	// plotting parametrs..
	Variable n_filter = ceil(DimSize(m_x,0) * 2e-4)
	Variable x_low = 8.4
	Variable x_high = 8.6
	Variable offset_y = 55e-12
	ModPlotUtil#clf()
	ModPlotUtil#figure(hide=0)
	ModPlotUtil#subplot(2,1,1)

	Variable split_idx = ModReportUtil#FEC_Plot(m_x,m_y,NFilterPoints=n_filter,YFactor=1e12,OffsetY=-offset_y)
	ModPlotUtil#ylabel("Force (pN)")
	ModPlotUtil#xlim(x_low,x_high)	
	ModPlotUtil#subplot(2,1,2)
	Variable offset_x = WaveMin(m_separation_wave)
	ModReportUtil#FEC_Plot(m_x,m_separation_wave,NFilterPoints=n_filter,YFactor=1e9,split_idx=split_idx,SignX=1,SignY=1,OffsetY=offset_x)
	ModPlotUtil#xlim(x_low,x_high)
	ModPlotUtil#ylabel("Separation (nm)")
	ModPlotUtil#xlabel("Time (s)")		
End Function