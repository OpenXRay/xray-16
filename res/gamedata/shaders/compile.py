import os
import subprocess
import re
import binascii
import struct
from multiprocessing.pool import ThreadPool

# Set the path to the fxc compiler
fxc = os.environ['DXSDK_DIR'] + 'Utilities\\bin\\x86\\fxc.exe'
featureLevel = '3_0'
workers = 8

# Set the directory you want to start from
shaders = 'r2'
objects = 'r2/objects/r2'

# List all macro identifiers, excluding SMAP
options = [
    'FP16_FILTER',
    'FP16_BLEND',
    'USE_HWSMAP',
    'USE_HWSMAP_PCF',
    'USE_FETCH4',
    'USE_SJITTER',
    'USE_BRANCHING',
    'USE_VTF',
    'USE_TSHADOWS',
    'USE_MBLUR',
    'USE_SUNFILTER',
    'USE_R2_STATIC_SUN',
    'FORCE_GLOSS',
    'SKIN_COLOR',
    'USE_SSAO_BLUR',
    'USE_HBAO',
    'SSAO_OPT_DATA',
    'SKIN_NONE',
    'SKIN_0',
    'SKIN_1',
    'SKIN_2',
    'SKIN_3',
    'SKIN_4',
    'USE_SOFT_WATER',
    'USE_SOFT_PARTICLES',
    'USE_DOF',
    'SUN_SHAFTS_QUALITY',
    'SSAO_QUALITY',
    'SUN_QUALITY',
    'ALLOW_STEEPPARALLAX'
]

skinOptions = [
    'SKIN_NONE',
    'SKIN_0',
    'SKIN_1',
    'SKIN_2',
    'SKIN_3',
    'SKIN_4'
]

forceOptions = [
    'USE_HWSMAP'
]

disableOptions = [
    'SKIN_COLOR'
]

qualityOptions = {
    'SUN_SHAFTS_QUALITY': 3,
    'SSAO_QUALITY': 3,
    'SUN_QUALITY': 2
}

entrex = re.compile('main_(vs|ps)_[0-9]_[0-9]')
inclex = re.compile('#include\s+"(.*)"')
commex = re.compile("//.*?\n")

def addCRC(filename):
    with open(filename) as file:
        file = open(filename,'r+b')
        buf = file.read()
        crc = (binascii.crc32(buf) & 0xFFFFFFFF)
        file.seek(0)
        file.write(struct.pack("<I", crc))
        file.write(buf)

def findOptions(shader):
    list = []
    with open(shader) as f:
        buffer = f.read()
        buffer = re.sub(commex, "", buffer) # remove commented lines
        for option in options:
            if option in buffer:
                list.append(option)
        includes = inclex.findall(buffer)  # find all include files
        for include in includes:
            list += findOptions(os.path.join(os.path.dirname(shader), include))
    return list

def findEntry(shader):
    with open(shader) as f:
        buffer = f.read()
        buffer = re.sub(commex, "", buffer) # remove commented lines
        match = entrex.search(buffer)  # find the entry point
        if match:
            return match.group(0)
        if 'main' in buffer:
            return 'main'
        includes = inclex.findall(buffer)  # find all include files
        for include in includes:
            if include != 'common.h':
                return findEntry(os.path.join(os.path.dirname(shader), include))
    return False

def generateIds(list, id, i):
    if i >= len(options):
        return [ id ]
    if options[i] in list:
        if options[i] in forceOptions:
            return generateIds(list, id + '1', i + 1)
        if options[i] in disableOptions:
            return generateIds(list, id + '0', i + 1)
        if options[i] in skinOptions:
            ids = []
            for (index, option) in enumerate(skinOptions):
                skin = '0' * len(skinOptions)
                skin = skin[:index] + '1' + skin[index+1:]
                ids += generateIds(list, id + skin, i + len(skinOptions))
            return ids
        if options[i] in qualityOptions:
            ids = []
            for index in range(0, qualityOptions[options[i]]):
                ids += generateIds(list, id + str(index), i + 1)
            return ids
        return generateIds(list, id + '0', i + 1) + generateIds(list, id + '1', i + 1)
    else:
        return generateIds(list, id + '_', i + 1)

def compileShader(name, stage, flags):
    path = os.path.join(shaders, '%s.%s' % (name, stage))
    entry = findEntry(path)
    if not entry:
        print('No entry point found, skipping shader...')
        return

    profile = '%s_%s' % (stage, featureLevel)
    if len(entry) > len('main'):
        profile = entry[5:]
    
    object = os.path.join(objects, '%s.%s' % (name, stage))
    if os.path.isfile(object):
        print('Already compiled, skipping object %s' % flags)
        return
    
    args = [fxc, '/nologo', '/LD', '/Zpr', '/E%s' % entry, '/T%s' % profile]
    
    args.append('/D%s=%s' % ('SMAP_size', flags[0:4]))
    for idx, option in enumerate(options, 4):
        if flags[idx] != '_' and int(flags[idx]) != 0:
            args.append('/D%s=%s' % (option, flags[idx]))
            if option in skinOptions and option != 'SKIN_NONE':
                object = os.path.join(objects, '%s_%s.%s' % (name, option[-1], stage))
    args.append('/Fo%s' % os.path.join(object, flags))
    args.append(path)
    subprocess.check_call(args)
    addCRC(os.path.join(object, flags))
    
pool = ThreadPool(processes=workers)

jobs = []
for file in os.listdir(shaders):
    shader = os.path.splitext(file)[0]
    stage = os.path.splitext(file)[1][1:]
    if stage == 'vs' or stage == 'ps':
        print('Found shader: %s' % shader)
        flags = findOptions(os.path.join(shaders, file))
        outdir = os.path.join(objects, file)
        if not os.path.isdir(outdir):
            os.makedirs(outdir)
        if 'SKIN_NONE' in flags:
            for skin in skinOptions[1:]:
                outdir = os.path.join(objects, '%s_%s.%s' % (shader, skin[-1], stage))
                if (not os.path.isdir(outdir)):
                    os.makedirs(outdir)
        print(flags)
        for id in generateIds(flags, "2048", 0):
            job = pool.apply_async(compileShader, (shader, stage, id))
            jobs.append(job)
    for job in jobs:
        job.get()

pool.close()
pool.join()