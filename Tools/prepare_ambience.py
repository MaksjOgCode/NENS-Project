"""Prepare phase-continuous ambient loops from the artist's unmodified sources.

Uses the libsndfile shipped with the selected Unreal installation and NumPy.
No network/downloads. Equal-power overlap removes the original end/start cut.
"""
import argparse
import ctypes as ct
import json
import math
import os
from pathlib import Path
import shutil
import wave
import numpy as np

class Info(ct.Structure):
    _fields_=[('frames',ct.c_int64),('samplerate',ct.c_int),('channels',ct.c_int),
              ('format',ct.c_int),('sections',ct.c_int),('seekable',ct.c_int)]

def prepare(source, destination, library, seconds=3.0):
    info=Info()
    handle=library.sf_wchar_open(str(source),0x10,ct.byref(info))
    if not handle: raise RuntimeError('Cannot decode '+str(source))
    try:
        samples=np.empty((info.frames,info.channels),dtype=np.float32)
        count=library.sf_readf_float(handle,samples.ctypes.data_as(ct.POINTER(ct.c_float)),info.frames)
        assert count==info.frames and info.samplerate>0 and 1<=info.channels<=2
    finally: library.sf_close(handle)
    assert np.isfinite(samples).all()
    dc=samples.mean(axis=0,dtype=np.float64)
    samples-=dc.astype(np.float32)
    n=min(round(seconds*info.samplerate),len(samples)//8)
    assert n>1024
    angle=np.linspace(0,math.pi/2,n,endpoint=True,dtype=np.float32)[:,None]
    overlap=samples[-n:]*np.cos(angle)+samples[:n]*np.sin(angle)
    loop=np.concatenate((samples[n:-n],overlap))
    rms=float(np.sqrt(np.mean(loop.astype(np.float64)**2)))
    peak=float(np.max(np.abs(loop)))
    # Gentle ambient level, bounded gain, ample headroom even in the overlap.
    gain=min(10**(-24/20)/max(rms,1e-8),10**(-1/20)/max(peak,1e-8),10**(6/20))
    loop*=gain
    encoded=np.rint(np.clip(loop,-1,1)*32767).astype('<i2')
    destination.parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(destination),'wb') as output:
        output.setnchannels(info.channels); output.setsampwidth(2); output.setframerate(info.samplerate)
        output.writeframes(encoded.tobytes())
    steps=np.abs(np.diff(encoded.astype(np.int32),axis=0))
    seam=np.abs(encoded[0].astype(np.int32)-encoded[-1].astype(np.int32))
    result=dict(source=str(source),output=str(destination),sample_rate=info.samplerate,channels=info.channels,
                source_seconds=info.frames/info.samplerate,loop_seconds=len(loop)/info.samplerate,
                overlap_seconds=n/info.samplerate,gain_db=20*math.log10(gain),
                rms_dbfs=20*math.log10(max(float(np.sqrt(np.mean(loop.astype(np.float64)**2))),1e-9)),
                peak_dbfs=20*math.log10(max(float(np.max(np.abs(loop))),1e-9)),
                seam_step_pcm=seam.tolist(),normal_step_99_percentile=float(np.percentile(steps,99)))
    assert result['peak_dbfs']<=-0.99 and np.max(np.abs(encoded.astype(np.int32)))<32767
    assert int(max(seam))<=max(256,result['normal_step_99_percentile']*3), 'Unexpected loop-boundary discontinuity'
    return result

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-dir',type=Path,required=True)
    parser.add_argument('--engine',type=Path,default=Path('C:/Program Files/Epic Games/UE_5.8'))
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    dll_dir=args.engine/'Engine/Binaries/ThirdParty/libsndfile/Win64'
    with os.add_dll_directory(str(dll_dir)):
        lib=ct.CDLL(str(dll_dir/'libsndfile-1.dll'))
        lib.sf_wchar_open.argtypes=[ct.c_wchar_p,ct.c_int,ct.POINTER(Info)]; lib.sf_wchar_open.restype=ct.c_void_p
        lib.sf_readf_float.argtypes=[ct.c_void_p,ct.POINTER(ct.c_float),ct.c_int64]; lib.sf_readf_float.restype=ct.c_int64
        lib.sf_close.argtypes=[ct.c_void_p]
        report=[]
        for original in ['S_GeneralGamePlay_Ambient.mp3','S_MainMenu_Ambient.wav']:
            source=root/'SourceArt/Audio/Originals'/original
            source.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(args.source_dir/original,source)
            report.append(prepare(source,root/'SourceArt/Audio/Loops'/(source.stem+'.wav'),lib))
    saved=root/'Saved/GamePresentation'; saved.mkdir(parents=True,exist_ok=True)
    (saved/'audio-preparation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
