// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModMainEnergyLandscape
#include ":::Util:PlotUtil"
#include ":::Util:IoUtil"

Static Function pTextBox(rotation,label_x,label_y,fontsize,label_string)
	Variable rotation,label_x,label_y,fontsize
	String label_string
	String SizedLabelString
	sprintf SizedLabelString,"\\Z%d%s",fontSize,label_string
	TextBox /O=(rotation)/Y=(label_y)/X=(label_x)/E=2/B=1/F=0/A=MT(SizedLabelString)
End Function

Static Function ScaleBar(Name,x1,x2,y1,y2,LabelString,label_x,label_y,rotation)
	String Name,LabelString
	Variable x1,x2,y1,y2,label_x,label_y,rotation
	Make /O/N=2  $(Name + " X"),$(Name + " Y")
	Wave X_Wave =$(Name + " X")
	Wave Y_Wave = $(Name + " Y")
	X_Wave = {x1,x2}
	Y_Wave = {y1,y2}
	ModPlotUtil#Plot(Y_Wave,mX=X_Wave,color="k",linewidth=4)
	Variable fontsize=20
	pTextBox(rotation,label_x,label_y,fontsize,labelString)
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
	ModIoUtil#LoadFile(InDir + "Unfold.txt")
	ModIoUtil#LoadFile(InDir + "Landscape.txt")
	Wave Folded= $("wave1")
	Wave Unfolded= $("wave0")
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
	String Sub1 = ModPlotUtil#Subplot(1,2,1)
	ModPlotUtil#Plot(FoldedY,mX=FoldedX,color="r",soften=1,alpha=0.3)
	ModPlotUtil#Plot(UnfoldedY,mX=UnfoldedX,color="b",soften=1,alpha=0.3)
	Make /O/T labels = {"Unfolding","Refolding"}
	Variable yOffset =-10
	Variable yOffsetPct = 60
	Variable xOffset = -2.1
	Variable XOffPct = -5
	ScaleBar("XFEC",xOffset+59.9,xOffset+62,yOffset+20,yOffset+20,"2nm",XOffPct+-28,YOffsetPct+30,0)
	ScaleBar("YFEC",xOffset+60,xOffset+60,yOffset+20,yOffset+25,"5pN",XOffPct+-40,YOffsetPct+5,90)
	ModPlotUtil#AxisOff()
	ModPlotUtil#TightAxes(points_left=30,points_bottom=15)
	// Add arrows. First, the outgoing
	Variable  ArrowSharp=0.0,Arrow=1,ArrowLen=10,linethick=3,arrowfat=1
	SetDrawEnv arrow=Arrow,arrowlen=ArrowLen,linethick=linethick,arrowfat=arrowfat,save
	SetDrawEnv arrowsharp= ArrowSharp,linefgc=(65535,37029,37029), save
	DrawLine 0.1,0.2,0.3,0.0
	// Next , the incoming 
	SetDrawEnv arrow=Arrow,arrowlen=ArrowLen,linethick=linethick,arrowfat=arrowfat,save
	SetDrawEnv arrowsharp= ArrowSharp,linefgc=(49151,53155,65535), save
	DrawLine 0.8,0.15,0.6,0.3
	Variable Rotation=0,fontsize=20,factor_caption=2
	// Make a slightly bigger caption...
	pTextBox(Rotation,-45,5,fontsize*factor_caption,"A")
	String EnergyPlot=ModPlotUtil#Subplot(1,2,2)
	ModPlotUtil#Plot(LandscapeY,mX=LandscapeX,color="g",alpha=0.3)
	ModPlotUtil#YLim(0,5,graphname=EnergyPlot)
	XOffset = -1.5
	XOffPct = 0
	YOffset = -2.6
	YOffsetPct = 53
	ScaleBar("XEnergy",XOffset+57.9,XOffset+60,YOffset+2.2,YOffset+2.2,"2nm",XOffPct+-30,YOffsetPct+35,0)
	ScaleBar("YEnergy",XOffset+58,XOffset+58,YOffset+2.2,YOffset+3.2,"1\f02k\[0\BB\]0T",XOffPct+-43,YOffsetPct+17,90)
	ModPlotUtil#AxisOff()
	ModPlotUtil#TightAxes(points_left=40,points_bottom=35)
	// put in the arrows
	pTextBox(Rotation,-25,20,fontsize,"Folded")
	pTextBox(Rotation,34,20,fontsize,"Unfolded")
	pTextBox(Rotation,-45,5,fontsize*factor_caption,"B")
	ModPlotUtil#SaveFig(savename=(InDir + "Copy"),saveAsPxp=1,closeFig=0,figname=fig)
	ModPlotUtil#SaveFig(savename=(InDir + "Copy"),saveAsPxp=0,closeFig=0,figname=fig)

End Function