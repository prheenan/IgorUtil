// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModMenuNames


// Below are functions returning important control names
Static Function /S MasterScanSize()
	return "ScanSizeSetVar_0"
End Function

Static Function /S MasterScanRate()
	return "ScanRateSetVar_0"
End Function

Static Function /S MasterOffsetX()
	return "XOffsetSetVar_0"
End Function

Static Function /S MasterOffsetY()
	return "YOffsetSetVar_0"
End Function

Static Function /S MasterScanPixels()
	return "PointsLinesSetVar_0"
End Function
	
// Below are functions returning important panel names
Static Function /S MasterPanel()
	return "MasterPanel"
End Function

