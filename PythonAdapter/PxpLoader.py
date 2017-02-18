# force floating point division. Can still use integer with //
from __future__ import division
# This file is used for importing the common utilities classes.
import numpy as np
import matplotlib.pyplot as plt
import sys
import ProcessSingleWave
from GeneralUtil.python.LibUtil import IgorUtil as IgorUtil
from GeneralUtil.python import GenUtilities as pGenUtil

from pprint import pformat
from IgorUtil.PythonAdapter.igor.binarywave import load as loadibw
from IgorUtil.PythonAdapter.igor.packed import load as loadpxp
from IgorUtil.PythonAdapter.igor.record.wave import WaveRecord

import re
import collections


def LoadPxpFilesFromDirectory(directory):
    """
    Given a directory, load all the pxp files into a wave

    Args:
        directory: loads all directories 
    Returns:
        see LoadAllWavesFromPxp
    """
    allFiles = pGenUtil.getAllFiles(directory,ext=".pxp")
    d = []
    for f in allFiles:
        d.extend(LoadAllWavesFromPxp(f))
    return d

def IsValidFec(Record):
    """
    Args:
        Wave: igor WaveRecord type
    Returns:
        True if the waves is consistent with a FEC
    """
    return ProcessSingleWave.ValidName(Record.wave)

def valid_fec_allow_endings(Record):
    name = ProcessSingleWave.GetWaveName(Record.wave).lower()
    for ext in ProcessSingleWave.DATA_EXT:
        if (ext.lower() in name):
            return True
    return False

def IsValidImage(Record):
    """
    Returns true if this wave appears to be a valid image

    Args:
        See IsValidFEC
    """
    Wave = Record.wave
    # check if the name matches
    Name = ProcessSingleWave.GetWaveName(Wave)
    Numbers = 4
    pattern = re.compile("^[0-9]{4}$")
    if (not pattern.match(Name[-Numbers:])):
        return False
    # now we need to check the dimensionality of the wave
    WaveStruct =  ProcessSingleWave.GetWaveStruct(Wave)
    header = ProcessSingleWave.GetHeader(WaveStruct)
    dat = WaveStruct['wData']
    ToRet = len(dat.shape) == 3
    return ToRet

def LoadAllWavesFromPxp(filepath,ValidFunc=IsValidFec):
    """
    Given a file path to an igor pxp file, loads all waves associated with it

    Args:
        filepath: path to igor pxp file
        ValidFunc: takes in a record, returns true if we wants it. defaults to
        all FEC-valid ones
    Returns:
        list of WaveObj (see ParseSingleWave), containing data and metadata
    """
    # XXX use file system to filter?
    records,_ = loadpxp(filepath)
    mWaves = []
    for i,record in enumerate(records):
        # if this is a wave with a proper name, go for it
        if isinstance(record, WaveRecord):
            # determine if the wave is something we care about
            if (not ValidFunc(record)):
                continue
            # POST: have a valid name
            WaveObj = ProcessSingleWave.WaveObj(record=record.wave,
                                                SourceFile=filepath)
            mWaves.append(WaveObj)
    return mWaves


def GroupWavesByEnding(WaveObjs):
    """
    Given a list of waves and (optional) list of endings, groups the waves

    Args:
        WaveObjs: List of wave objects, from LoadAllWavesFromPxp
    
    Returns:
        dictionary of lists; each sublist is a 'grouping' of waves by extension
    """
    # get all the names of the wave objects
    rawNames = [o.Name() for o in WaveObjs]
    # assumed waves end with a number, followed by an ending
    # we need to figure out what the endings and numbers are
    digitEndingList = []
    # filter out to objects we are about
    goodNames = []
    goodObj = []
    for n,obj in zip(rawNames,WaveObjs):
        try:
            digitEndingList.append(ProcessSingleWave.IgorNameRegex(n))
            goodNames.append(n)
            goodObj.append(obj)
        except ValueError as e:
            # not a good wave, go ahead and remove it
            continue
    # first element gives the (assumed unique) ids
    preamble = [ele[0] for ele in digitEndingList]
    ids = [ele[1] for ele in digitEndingList]
    endings = [ele[2] for ele in digitEndingList]
    # following (but we want all, not just duplicates):
#stackoverflow.com/questions/5419204/index-of-duplicates-items-in-a-python-list
    counter=collections.Counter(ids) 
    idSet=[i for i in counter]
    # each key of 'result' will give a list of indies associated with that ID
    result={}
    for item in idSet:
        result[item]=[i for i,j in enumerate(ids) if j==item]
    # (1) each key in the result corresponds to a specific ID (with extensions)
    # (2) each value associated with a key is a list of indices
    # Go ahead and group the waves (remember the waves? that's what we care
    # about) into a 'master' dictionary, indexed by their names
    finalList ={}
    for key,val in result.items():
        tmp = {}
        # append each index to this list
        for idxWithSameId in val:
            objToAdd = goodObj[idxWithSameId]
            tmp[endings[idxWithSameId].lower()] = objToAdd
        finalList[preamble[idxWithSameId] + key] = tmp
    return finalList
    
def LoadPxp(inFile,**kwargs):
    """
    Convenience Wrapper. Given a pxp file, reads in all data waves and
    groups by common ID

    Args:
        Infile: file to input
        **kwargs: passed to LoadAllWavesFromPxp
    Returns:
        dictionary: see GroupWavesByEnding, same output
    """
    mWaves = LoadAllWavesFromPxp(inFile,**kwargs)
    return GroupWavesByEnding(mWaves)
