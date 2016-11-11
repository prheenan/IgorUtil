// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModMainEnergyLandscape
#include ":::Util:PlotUtil"
#include ":::Util:IoUtil"

Static Function ScaleBar(Name,x1,x2,y1,y2,LabelString,labelx,labely,rotation)
	String Name,LabelString
	Variable x1,x2,y1,y2,labelx,labely,rotation
	Make /O/N=2  $(Name + " X"),$(Name + " Y")
	Wave X_Wave =$(Name + " X")
	Wave Y_Wave = $(Name + " Y")
	X_Wave = {x1,x2}
	Y_Wave = {y1,y2}
	ModPlotUtil#Plot(Y_Wave,mX=X_Wave,color="k",linewidth=4)
	String SizedLabelString
	Variable fontSize = 20
	sprintf SizedLabelString,"\\Z%d%s",fontSize,LabelString
	TextBox /O=(rotation)/Y=(labelY)/X=(labelx)/E=2/B=1/F=0/A=MT(SizedLabelString)
End Function

Static Function Main()
	// Description goes here
	//
	// Args:
	//		Arg 1:
	//		
	// Returns:
	//
	//
	KillWaves /A/Z
	ModPlotUtil#clf()
	String Base ="Macintosh HD:Users:patrickheenan:src_prh:IgorUtil:IgorCode:Figures:"
	String InDir = Base + "2016_11_nsf:Data:"
	ModIoUtil#LoadFile(InDir + "Fold.txt")
	ModIoUtil#LoadFile(InDir + "UnFold.txt")
	ModIoUtil#LoadFile(InDir + "Landscape.txt")
	Wave Folded= $("wave0")
	Wave Unfolded= $("wave1")
	Wave Landscape = $("wave2")
	// Get all the components...ugh
	Make /O/N=(DimSize(Folded,0)) FoldedX,FoldedY 
	FoldedX  = Folded[p][0]
	FoldedY  = Folded[p][1]
	Make /O/N=(DimSize(Unfolded,0)) UnfoldedX,UnFoldedY 
	UnFoldedX  = UnFolded[p][0]
	UnFoldedY  = UnFolded[p][1]
	Make /O/N=(DimSize(Landscape,0)) LandscapeX,LandscapeY 
	LandscapeX  = Landscape[p][0]
	LandscapeY  = Landscape[p][1]
	// plot everything
	String Fig = ModPlotUtil#Figure(hide=0,heightIn=4,widthIn=8)
	ModPlotUtil#Subplot(1,2,1)
	ModPlotUtil#Plot(FoldedY,mX=FoldedX,color="r",soften=1,alpha=0.3)
	ModPlotUtil#Plot(UnfoldedY,mX=UnfoldedX,color="b",soften=1,alpha=0.3)
	ScaleBar("XFEC",61,63,25,25,"2nm",-28,0,0)
	ScaleBar("YFEC",60,60,20,25,"5pN",-46,20,90)
	ModPlotUtil#AxisOff()
	ModPlotUtil#TightAxes()
	String EnergyPlot=ModPlotUtil#Subplot(1,2,2)
	ModPlotUtil#Plot(LandscapeY,mX=LandscapeX,color="g",alpha=0.3)
	ModPlotUtil#YLim(0,5,graphname=EnergyPlot)
	ScaleBar("XEnergy",59,61,3.2,3.2,"2nm",-27,3,0)
	ScaleBar("YEnergy",58,58,2.2,3.2,"1kT",-46,20,90)
	ModPlotUtil#AxisOff()
	ModPlotUtil#TightAxes()
End Function