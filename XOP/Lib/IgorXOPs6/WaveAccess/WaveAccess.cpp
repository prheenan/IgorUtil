/*	WaveAccess.c

	Igor Pro 3.0 extended wave data storage from 1 dimension to 4 dimensions.
	This sample XOP illustrates how to access waves of dimension 1 through 4.
	
	The function WAGetWaveInfo illustrates the use of the following calls:
		WaveName, WaveType, WaveUnits, WaveScaling
		MDGetWaveDimensions, MDGetWaveScaling, MDGetWaveUnits, MDGetDimensionLabels
		
	It will  with Igor Pro 2.0 or later.

	Invoke it from Igor's command line like this:
		Make/N=(5,4,3) wave3D		// wave with 5 rows, 4 columns and 3 layers
		Print WAGetWaveInfo(wave3D)
	
	The functions WAFill3DWaveDirectMethod, WAFill3DWavePointMethod and
	WAFill3DWaveStorageMethod each fill a 3D wave with values, using different
	wave access methods. They all require Igor Pro 3.0 or later.
	
	You can invoke these functions from Igor Pro 3.0 or later as follows:
		Edit wave3D
		WAFill3DWaveDirectMethod(wave3D)
	or	WAFill3DWavePointMethod(wave3D)
	or	WAFill3DWaveStorageMethod(wave3D)
	
	The function fills a 3 dimensional wave with values such that:
		w[p][q][r] = p + 1e3*q + 1e6*r
	where p is the row number, q is the column number and r is the layer number.
	This is the equivalent of executing the following in Igor Pro 3.0 or later:
		wave3D = p + 1e3*q + 1e6*r
	
	The function WAModifyTextWave shows how to read and write the contents of
	a text wave. Invoke it from Igor Pro 3.0 or later like this:
		Make/T/N=(4,4) textWave2D = "(" + num2str(p) + "," + num2str(q) + ")"
		Edit textWave2D
		WAModifyTextWave(textWave2D, "Row/col=", ".")
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "WaveAccess.h"

// Global Variables
static int gCallSpinProcess = 1;		// Set to 1 to all user abort (cmd dot) and background processing.


static int
AddCStringToHandle(						// Concatenates C string to handle.
	const char *theStr,
	Handle theHand)
{
	return PtrAndHand(theStr, theHand, strlen(theStr));
}

#pragma pack(2)		// All structures passed to Igor are two-byte aligned
struct WAGetWaveInfoParams {
	waveHndl w;
	Handle strH;
};
typedef struct WAGetWaveInfoParams WAGetWaveInfoParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
WAGetWaveInfo(WAGetWaveInfoParams* p)	// See the top of the file for instructions on how to invoke this function from Igor Pro 3.0 or later.
{
	char buf[256];
	char waveName[MAX_OBJ_NAME+1];
	int waveType;
	int dimension, numDimensions;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	char dimensionUnits[MAX_DIMENSIONS][MAX_UNIT_CHARS+1];
	IndexInt element;
	char dataUnits[MAX_UNIT_CHARS+1];
	double dataFullScaleMax, dataFullScaleMin;
	double sfA[MAX_DIMENSIONS];
	double sfB[MAX_DIMENSIONS];
	char dimLabel[MAX_DIM_LABEL_CHARS+1];
	int result;
	
	if (p->w==NIL) {
		p->strH = NIL;						// Tell Igor that function return value is undefined.
		return NON_EXISTENT_WAVE;			// Make sure wave exists.
	}
	
	p->strH = NewHandle(0L);
	if (p->strH == NIL)
		return NOMEM;

	// Get wave name.
	WaveName(p->w, waveName);

	// Get wave data type.
	waveType = WaveType(p->w);

	// Get number of used dimensions in wave.
	if (result = MDGetWaveDimensions(p->w, &numDimensions, dimensionSizes))
		return result;

	/*	Get wave scaling for all used dimensions.
		The scaled index value for point p of dimension d is computed as:
			scaled index = p*sfA[d] + sfB[d];
	*/
	for(dimension=0; dimension<numDimensions; dimension++) {
		if (result = MDGetWaveScaling(p->w, dimension, &sfA[dimension], &sfB[dimension]))
			return result;
	}
	
	// Get units for all dimensions.
	for(dimension=0; dimension<numDimensions; dimension++) {
		if (result = MDGetWaveUnits(p->w, dimension, &dimensionUnits[dimension][0]))
			return result;
	}
	
	/*	Get the data nominal full scale values for the wave.
		-1 means get full scale values instead of dimension scaling.
	*/
	if (result = MDGetWaveScaling(p->w, -1, &dataFullScaleMax, &dataFullScaleMin))
		return result;
		
	/*	Get the data units for the wave.
		-1 means data units instead of dimension units.
	*/
	if (result = MDGetWaveUnits(p->w, -1, dataUnits))
		return result;

	// Now, store all of the info in the handle to return to Igor.
	
	sprintf(buf, "Wave name: \'%s\'; type: %d; dimensions: %d", waveName, waveType, numDimensions);
	if (result = AddCStringToHandle(buf, p->strH))
		return result;
	
	// Add the data units and nominal full scale values.
	sprintf(buf, "; data units=\"%s\"; data full scale=%g,%g", dataUnits, dataFullScaleMin, dataFullScaleMax);
	if (result = AddCStringToHandle(buf, p->strH))
		return result;
	
	// Add information for each dimension.
	for(dimension=0; dimension<numDimensions; dimension++) {
		if (result = AddCStringToHandle(CR_STR, p->strH))			// Add CR.
			return result;
		sprintf(buf, "\tDimension number: %d, size=%lld, sfA=%g, sfB=%g, dimensionUnits=\"%s\""CR_STR,
				dimension, (SInt64)dimensionSizes[dimension], sfA[dimension], sfB[dimension], dimensionUnits[dimension]);
		if (result = AddCStringToHandle(buf, p->strH))
			return result;
		
		//	Get dimension label for each element of this dimension.
		if (result = AddCStringToHandle("\t\tLabels: ", p->strH))
			return result;
		for(element=-1; element<dimensionSizes[dimension]; element++) {		// Loop starts from -1 because -1 returns
			if (element >= 5) {												// the label for the entire dimension.
				if (result = AddCStringToHandle("(and so on)", p->strH))
					return result;
				break;
			}
			if (result = MDGetDimensionLabel(p->w, dimension, element, dimLabel))
				return result;
			sprintf(buf, "\'%s\'", dimLabel);
			if (element < dimensionSizes[dimension]-1)
				strcat(buf, ", ");
			if (result = AddCStringToHandle(buf, p->strH))
				return result;
		}
	}
	
	return(0);							// XFUNC error code.
}


/*	WAFill3DWaveDirectMethod()
	
	This example shows how to access the data in a multi-dimensional wave
	using the direct method.
	
	See the top of the file for instructions on how to invoke this function
	from Igor Pro 3.0 or later.
*/

#pragma pack(2)		// All structures passed to Igor are two-byte aligned
struct WAFill3DWaveDirectMethodParams {
	waveHndl w;
	double result;
};
typedef struct WAFill3DWaveDirectMethodParams WAFill3DWaveDirectMethodParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
WAFill3DWaveDirectMethod(WAFill3DWaveDirectMethodParams* p)
{
	waveHndl wavH;
	int waveType;
	int numDimensions;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	char* dataStartPtr;
	/*	Pointer terminology
		dp0 points to start of double data.
		dlp points to start of double data for a layer.
		dcp points to start of double data for a column.
		dp points to a particular point of double data.
	*/
	double *dp0, *dlp, *dcp, *dp;				// Pointers used for double data.
	float *fp0, *flp, *fcp, *fp;				// Pointers used for float data.
	SInt32 *lp0, *llp, *lcp, *lp;				// Pointers used for long data.
	short *sp0, *slp, *scp, *sp;				// Pointers used for short data.
	char *cp0, *clp, *ccp, *cp;					// Pointers used for char data.
	UInt32 *ulp0, *ullp, *ulcp, *ulp;			// Pointers used for unsigned long data.
	unsigned short *usp0, *uslp, *uscp, *usp;	// Pointers used for unsigned short data.
	unsigned char *ucp0, *uclp, *uccp, *ucp;	// Pointers used for unsigned char data.
	IndexInt dataOffset;
	CountInt numRows, numColumns, numLayers;
	IndexInt row, column, layer;
	CountInt pointsPerColumn, pointsPerLayer;
	int result;
	
	p->result = 0;				// The Igor function result is always zero.
	
	wavH = p->w;
	if (wavH == NIL)
		return NOWAV;

	waveType = WaveType(wavH);
	if (waveType & NT_CMPLX)
		return NO_COMPLEX_WAVE;
	if (waveType==TEXT_WAVE_TYPE)
		return NUMERIC_ACCESS_ON_TEXT_WAVE;
	
	if (result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes))
		return result;
	
	if (numDimensions != 3)
		return NEEDS_3D_WAVE;

	numRows = dimensionSizes[0];
	numColumns = dimensionSizes[1];
	numLayers = dimensionSizes[2];
	pointsPerColumn = numRows;
	pointsPerLayer = pointsPerColumn*numColumns;
	
	if (result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset))
		return result;
	
	dataStartPtr = (char*)(*wavH) + dataOffset;
	
	result = 0;
	switch (waveType) {
		case NT_FP64:
			dp0 = (double*)dataStartPtr;							// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				dlp = dp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					dcp = dlp + column*pointsPerColumn;				// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						dp = dcp + row;
						*dp = (double)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_FP32:
			fp0 = (float*)dataStartPtr;								// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				flp = fp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					fcp = flp + column*pointsPerColumn;				// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						fp = fcp + row;
						*fp = (float)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I32:
			lp0 = (SInt32*)dataStartPtr;							// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				llp = lp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					lcp = llp + column*pointsPerColumn;				// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						lp = lcp + row;
						*lp = (SInt32)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I16:
			sp0 = (short*)dataStartPtr;								// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				slp = sp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					scp = slp + column*pointsPerColumn;				// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						sp = scp + row;
						*sp = (short)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I8:
			cp0 = (char*)dataStartPtr;								// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				clp = cp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					ccp = clp + column*pointsPerColumn;				// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						cp = ccp + row;
						*cp = (char)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I32 | NT_UNSIGNED:
			ulp0 = (UInt32*)dataStartPtr;							// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				ullp = ulp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					ulcp = ullp + column*pointsPerColumn;			// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						ulp = ulcp + row;
						*ulp = (UInt32)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I16 | NT_UNSIGNED:
			usp0 = (unsigned short*)dataStartPtr;					// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				uslp = usp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					uscp = uslp + column*pointsPerColumn;			// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						usp = uscp + row;
						*usp = (unsigned short)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;

		case NT_I8 | NT_UNSIGNED:
			ucp0 = (unsigned char*)dataStartPtr;					// Pointer to the start of all wave data.
			for(layer=0; layer<numLayers; layer++) {
				uclp = ucp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
				for(column=0; column<numColumns; column++) {
					if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
						result = -1;								// User aborted.
						break;
					}
					uccp = uclp + column*pointsPerColumn;			// Pointer to start of data for this column.
					for(row=0; row<numRows; row++) {
						ucp = uccp + row;
						*ucp = (unsigned char)(row + 1000*column + 1000000*layer);
					}
				}
				if (result != 0)
					break;											// User abort.
			}
			break;
		
		default:	// Unknown data type - possible in a future version of Igor.
			return NT_FNOT_AVAIL;
			break;
	}
	
	WaveHandleModified(wavH);			// Inform Igor that we have changed the wave.
	
	return result;
}

/*	WAFill3DWavePointMethod()
	
	This example shows how to access the data in a multi-dimensional wave
	using a slower but very easy access method.

	See the top of the file for instructions on how to invoke this function
	from Igor Pro 3.0 or later.
	
	By using the MDSetNumericWavePointValue routine to store into the wave, instead of
	accessing the wave directly, we relieve ourselves of the need to worry about
	the data type of the wave, at the cost of running more slowly.
*/

#pragma pack(2)		// All structures passed to Igor are two-byte aligned
struct WAFill3DWavePointMethodParams {
	waveHndl w;
	double result;
};
typedef struct WAFill3DWavePointMethodParams WAFill3DWavePointMethodParams;
#pragma pack()		// Reset structure alignment to default.

static int
WAFill3DWavePointMethod(WAFill3DWavePointMethodParams* p)
{
	waveHndl wavH;
	int waveType;
	int numDimensions;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	IndexInt indices[MAX_DIMENSIONS];			// Used to pass the row, column and layer to MDSetNumericWavePointValue.
	double value[2];							// Contains, real/imaginary parts but we use the real only.
	CountInt numRows, numColumns, numLayers;
	IndexInt row, column, layer;
	int result;
	
	p->result = 0;				// The Igor function result is always zero.
	
	wavH = p->w;
	if (wavH == NIL)
		return NOWAV;

	waveType = WaveType(wavH);
	if (waveType & NT_CMPLX)
		return NO_COMPLEX_WAVE;
	if (waveType==TEXT_WAVE_TYPE)
		return NUMERIC_ACCESS_ON_TEXT_WAVE;
	
	if (result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes))
		return result;
	
	if (numDimensions != 3)
		return NEEDS_3D_WAVE;

	numRows = dimensionSizes[0];
	numColumns = dimensionSizes[1];
	numLayers = dimensionSizes[2];
	
	MemClear(indices, sizeof(indices));			// Unused indices must be zero.
	result = 0;
	for(layer=0; layer<numLayers; layer++) {
		indices[2] = layer;
		for(column=0; column<numColumns; column++) {
			if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
				result = -1;								// User aborted.
				break;
			}
			indices[1] = column;
			for(row=0; row<numRows; row++) {
				indices[0] = row;
				value[0] = (double)(row + 1000*column + 1000000*layer);
				if (result = MDSetNumericWavePointValue(wavH, indices, value)) {
					WaveHandleModified(wavH);			// Inform Igor that we have changed the wave.
					return result;
				}
			}
		}
		if (result != 0)
			break;
	}

	WaveHandleModified(wavH);			// Inform Igor that we have changed the wave.
	
	return result;
}


/*	WAFill3DWaveStorageMethod()
	
	This example shows how to access the data in a multi-dimensional wave
	using the temp storage method. It is fast and easy but requires enough
	memory for a temporary double-precision copy of the wave data.

	See the top of the file for instructions on how to invoke this function
	from Igor Pro 3.0 or later.
*/

#pragma pack(2)		// All structures passed to Igor are two-byte aligned
struct WAFill3DWaveStorageMethodParams {
	waveHndl w;
	double result;
};
typedef struct WAFill3DWaveStorageMethodParams WAFill3DWaveStorageMethodParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
WAFill3DWaveStorageMethod(WAFill3DWaveStorageMethodParams* p)
{
	waveHndl wavH;
	int waveType;
	int numDimensions;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	/*	Pointer terminology
		dp0 points to start of char data.
		dlp points to start of char data for a layer.
		dcp points to start of char data for a column.
		dp points to a particular point of char data.
	*/
	double *dp0, *dlp, *dcp, *dp;			// Pointers used for double data.
	CountInt numRows, numColumns, numLayers;
	IndexInt row, column, layer;
	CountInt pointsPerColumn, pointsPerLayer;
	BCInt numBytes;
	double* dPtr;
	int result, result2;
	
	p->result = 0;							// The Igor function result is always zero.
	
	wavH = p->w;
	if (wavH == NIL)
		return NOWAV;

	waveType = WaveType(wavH);
	if (waveType & NT_CMPLX)
		return NO_COMPLEX_WAVE;
	if (waveType==TEXT_WAVE_TYPE)
		return NUMERIC_ACCESS_ON_TEXT_WAVE;
	
	if (result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes))
		return result;
	
	if (numDimensions != 3)
		return NEEDS_3D_WAVE;

	numRows = dimensionSizes[0];
	numColumns = dimensionSizes[1];
	numLayers = dimensionSizes[2];
	pointsPerColumn = numRows;
	pointsPerLayer = pointsPerColumn*numColumns;
	
	numBytes = WavePoints(wavH) * sizeof(double);			// Bytes needed for copy
	//	This example doesn't support complex waves.
	//	if (isComplex)
	//		numBytes *= 2;
	dPtr = (double*)NewPtr(numBytes);
	if (dPtr==NIL)
		return NOMEM;
	
	if (result = MDGetDPDataFromNumericWave(wavH, dPtr)) {	// Get a copy of the wave data.
		DisposePtr((Ptr)dPtr);
		return result;
	}
	
	result = 0;
	dp0 = dPtr;												// Pointer to the start of all wave data.
	for(layer=0; layer<numLayers; layer++) {
		dlp = dp0 + layer*pointsPerLayer;					// Pointer to start of data for this layer.
		for(column=0; column<numColumns; column++) {
			if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
				result = -1;								// User aborted.
				break;
			}
			dcp = dlp + column*pointsPerColumn;				// Pointer to start of data for this column.
			for(row=0; row<numRows; row++) {
				dp = dcp + row;
				*dp = (double)(row + 1000*column + 1000000*layer);
			}
		}
		if (result != 0)
			break;
	}
	
	if (result2 = MDStoreDPDataInNumericWave(wavH, dPtr)) {	// Store copy in the wave.
		DisposePtr((Ptr)dPtr);
		return result2;
	}
	
	DisposePtr((Ptr)dPtr);
	WaveHandleModified(wavH);			// Inform Igor that we have changed the wave.
	
	return result;
}

static int
DoAppendAndPrepend(Handle textH, Handle prependStringH, Handle appendStringH)
{
	BCInt textHLength;
	BCInt appendStringHLength;
	BCInt prependStringHLength;

	textHLength = GetHandleSize(textH);
	prependStringHLength = GetHandleSize(prependStringH);
	appendStringHLength = GetHandleSize(appendStringH);
	
	SetHandleSize(textH, textHLength + prependStringHLength + appendStringHLength);
	if (MemError())
		return NOMEM;
	memmove(*textH+prependStringHLength, *textH, textHLength);								// Make room for prependString.
	memcpy(*textH, *prependStringH, prependStringHLength);									// Prepend prependString.
	memcpy(*textH+textHLength+prependStringHLength, *appendStringH, appendStringHLength);	// Append appendString.
	return 0;
}

/*	WAModifyTextWave()
	
	This example shows how to access the data in a multi-dimensional text wave.

	See the top of the file for instructions on how to invoke this function
	from Igor Pro 3.0 or later.
*/

#pragma pack(2)		// All structures passed to Igor are two-byte aligned
struct WAModifyTextWaveParams {
		Handle appendStringH;			// String to be appended to each wave point.
		Handle prependStringH;			// String to be prepended to each wave point.
		waveHndl w;
		double result;
};
typedef struct WAModifyTextWaveParams WAModifyTextWaveParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
WAModifyTextWave(WAModifyTextWaveParams* p)
{
	waveHndl wavH;
	int waveType;
	int numDimensions;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	IndexInt indices[MAX_DIMENSIONS];				// Used to pass the row, column and layer to MDSetTextWavePointValue.
	CountInt numRows, numColumns, numLayers, numChunks;
	IndexInt row, column, layer, chunk;
	Handle textH;
	int result;
	
	result = 0;

	textH = NewHandle(0L);			// Handle used to pass text wave characters to Igor.
	if (textH == NIL) {
		result = NOMEM;
		goto done;
	}
	
	if (p->prependStringH == NIL) {
		result = USING_NULL_STRVAR;		// The user called the function with an uninitialized string variable.
		goto done;
	}
	
	if (p->appendStringH == NIL) {
		result = USING_NULL_STRVAR;		// The user called the function with an uninitialized string variable.
		goto done;
	}
	
	wavH = p->w;
	if (wavH == NIL) {
		result = NOWAV;					// The user called the function with a missing wave or uninitialized wave reference variable.
		goto done;
	}

	waveType = WaveType(wavH);
	if (waveType!=TEXT_WAVE_TYPE) {
		result = TEXT_ACCESS_ON_NUMERIC_WAVE;
		goto done;
	}
	
	if (result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes))
		goto done;

	numRows = dimensionSizes[0];
	numColumns = dimensionSizes[1];
	if (numColumns==0)
		numColumns = 1;
	numLayers = dimensionSizes[2];
	if (numLayers==0)
		numLayers = 1;
	numChunks = dimensionSizes[3];
	if (numChunks==0)
		numChunks = 1;
	
	MemClear(indices, sizeof(indices));			// Clear unused indices.
	result = 0;
	for(chunk=0; chunk<numChunks; chunk++) {
		indices[3] = chunk;
		for(layer=0; layer<numLayers; layer++) {
			indices[2] = layer;
			for(column=0; column<numColumns; column++) {
				if (gCallSpinProcess && SpinProcess()) {		// Spins cursor and allows background processing.
					result = -1;								// User aborted.
					break;
				}
				indices[1] = column;
				for(row=0; row<numRows; row++) {
					indices[0] = row;
					if (result = MDGetTextWavePointValue(wavH, indices, textH))
						goto done;
					if (result = DoAppendAndPrepend(textH, p->prependStringH, p->appendStringH))
						goto done;
					if (result = MDSetTextWavePointValue(wavH, indices, textH))
						goto done;
				}
			}
			if (result != 0)
				break;
		}
		if (result != 0)
			break;
	}
	
done:
	if (wavH != NIL)
		WaveHandleModified(wavH);				// Inform Igor that we have changed the wave.
	if (textH != NIL)
		DisposeHandle(textH);
	if (p->prependStringH)
		DisposeHandle(p->prependStringH);		// We need to get rid of input parameters.
	if (p->appendStringH)
		DisposeHandle(p->appendStringH);		// We need to get rid of input parameters.
	p->result = 0;								// The Igor function result is always zero.
	
	return(result);
}

/*	RegisterFunction()
	
	Igor calls this at startup time to find the address of the
	XFUNCs added by this XOP. See XOP manual regarding "Direct XFUNCs".
*/
static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);		// Which function is Igor asking about?
	switch (funcIndex) {
		case 0:						// WAGetWaveInfo(wave)
			return (XOPIORecResult)WAGetWaveInfo;
			break;
		case 1:						// WAFill3DWaveDirectMethod(wave)
			return (XOPIORecResult)WAFill3DWaveDirectMethod;
			break;
		case 2:						// WAFill3DWavePointMethod(wave)
			return (XOPIORecResult)WAFill3DWavePointMethod;
			break;
		case 3:						// WAFill3DWaveStorageMethod(wave)
			return (XOPIORecResult)WAFill3DWaveStorageMethod;
			break;
		case 4:						// WAModifyTextWave(wave, prependString, appendString)
			return (XOPIORecResult)WAModifyTextWave;
			break;
	}
	return 0;
}

/*	DoFunction()
	
	Igor calls this when the user invokes one if the XOP's XFUNCs
	if we returned NIL for the XFUNC from RegisterFunction. In this
	XOP, we always use the direct XFUNC method, so Igor will never call
	this function. See XOP manual regarding "Direct XFUNCs".
*/
static int
DoFunction()
{
	int funcIndex;
	void *p;				// Pointer to structure containing function parameters and result.
	int err;

	funcIndex = (int)GetXOPItem(0);	// Which function is being invoked ?
	p = (void*)GetXOPItem(1);		// Get pointer to params/result.
	switch (funcIndex) {
		case 0:						// WAGetWaveInfo(wave)
			err = WAGetWaveInfo((WAGetWaveInfoParams*)p);
			break;
		case 1:						// WAFill3DWaveDirectMethod(wave)
			err = WAFill3DWaveDirectMethod((WAFill3DWaveDirectMethodParams*)p);
			break;
		case 2:						// WAFill3DWavePointMethod(wave)
			err = WAFill3DWavePointMethod((WAFill3DWavePointMethodParams*)p);
			break;
		case 3:						// WAFill3DWaveStorageMethod(wave)
			err = WAFill3DWaveStorageMethod((WAFill3DWaveStorageMethodParams*)p);
			break;
		case 4:						// WAModifyTextWave(wave, prependString, appendString)
			err = WAModifyTextWave((WAModifyTextWaveParams*)p);
			break;
	}
	return(err);
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all messages after the
	INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;

	switch (GetXOPMessage()) {
		case FUNCTION:						// Our external function being invoked ?
			result = DoFunction();
			break;

		case FUNCADDRS:
			result = RegisterFunction();
			break;
	}
	SetXOPResult(result);
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	
	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)		// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.
	
	if (igorVersion < 620) {			// Requires Igor Pro 6.20 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in WaveAccess.h and there are corresponding error strings in WaveAccess.r and WaveAccessWinCustom.rc.
		return EXIT_FAILURE;
	}

	SetXOPResult(0L);
	return EXIT_SUCCESS;
}
