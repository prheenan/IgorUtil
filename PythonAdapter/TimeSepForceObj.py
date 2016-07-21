# force floating point division. Can still use integer with //
from __future__ import division
# This file is used for importing the common utilities classes.
import numpy as np
import matplotlib.pyplot as plt

from WaveDataGroup import WaveDataGroup
from DataObj import DataObj as DataObj

class Bunch:
    """
    see 
http://stackoverflow.com/questions/2597278/python-load-variables-in-a-dict-into-namespace

    Used to keep track of the meta information
    """
    def __init__(self, adict):
        self.__dict__.update(adict)
    def __getitem__(self,key):
        return self.__dict__[key]

def DataObjByConcat(ConcatData,*args,**kwargs):
    """
    Initializes an data object from a concatenated wave object (e.g., 
    high resolution time,sep, and force)
    
    Args:
        ConcatData: concatenated WaveObj
    """
    Meta = Bunch(ConcatData.Note)
    time,sep,force = ConcatData.GetTimeSepForceAsCols()
    return DataObj(time,sep,force,Meta,*args,**kwargs)

class TimeSepForceObj:
    def __init__(self,mWaves=None):
        """
        Given a WaveDataGrop, gets an easier-to-use object, with low and 
        (possible) high resolution time sep and force
        
        Args:
            mWaves: WaveDataGroup. Should be able to get time,sep,and force 
            from it
        """
        if (mWaves is not None):
            self.LowResData = \
                DataObjByConcat(mWaves.CreateTimeSepForceWaveObject())
            # by default, assume we *dont* have high res data
            self.HiResData = None
            if (mWaves.HasHighBandwidth()):
                hiResConcat = mWaves.HighBandwidthCreateTimeSepForceWaveObject()
                self.HiResData = DataObjByConcat(hiResConcat)
    def HasSurfaceDwell(self):
        """
        Returns true if there is a surface dwell
        """
        # by default, stored as a float; 0 means no dwell, 1 means surface,
        # three means both, etc.
        DwellInt = int(self.Meta.DwellSetting) 
        return (DwellInt != 0) and (DwellInt % 2 == 1)
    @property
    def TriggerTime(self):
        return self.Meta.TriggerTime
    @property
    def SurfaceDwellTime(self):
        """
        Returns the dwell time (0 if none) as a float
        """
        if (self.HasSurfaceDwell()):
            return self.Meta.DwellTime
        else:
            return 0
    @property
    def Zsnsr(self):
        return self.LowResData.Zsnsr
    @property
    def Frequency(self):
        ToRet = float(self.Meta.NumPtsPerSec)
        return ToRet
    @property
    def Meta(self):
        """
        Returns the low-resolution meta
        """
        return self.LowResData.meta
    @property
    def Time(self):
        """
        return the low-resolution time
        """
        return self.LowResData.time
    @property
    def Separation(self):
        """
        Returns the (low resolution) separation
        """
        return self.LowResData.sep
    @property
    def Force(self):
        """
        Returns the (high resolution) separation
        """
        return self.LowResData.force
    @property
    def SpringConstant(self):
        return self.LowResData.meta.SpringConstant
    @property
    def Velocity(self):
        return self.LowResData.meta.Velocity
    @property
    def ApproachVelocity(self):
        return self.Meta.ApproachVelocity
    def CreatedFiltered(self,idxLowRes,idxHighRes):
        """
        Given indices for low and high resolution data, creates a new,
        Filtered data object (of type TimeSepForceObj)
        
        Args:
            idxLowRes: low resolution indices of interest. Should be a list;
            each element is a distinct 'window' we wan to look at

            idxHighRes: high resolution indices of interest. see idxLowRes
        """
        assert self.HiResData is not None
        # create an (empty) data object
        toRet = TimeSepForceObj()
        toRet.LowResData= self.LowResData.CreateDataSliced(idxLowRes)
        toRet.HiResData = self.HiResData.CreateDataSliced(idxHighRes)
        return toRet
    
