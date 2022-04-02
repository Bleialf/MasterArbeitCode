import json
import os
from datetime import datetime, timedelta

filePath = 'time.txt'
times = []

def init():
    global times
    times = getDefault()
    save()


def getDefault():
    times = []
    for i in range(24):
        times.append(10)
    return times
        
def save():
    global times
    with open(filePath, 'w') as f:
        json.dump(times, f)
        
def load():
    global times
    with open(filePath, 'r') as f:
        times = json.load(f)
        
def getNextTime():
    now = datetime.now()
    index = now.hour
    skippedHours = 0
    if (times[index] == 0):
        while(times[index] == 0):
            index += 1
            skippedHours += 1
            if index == 24:
                index = 0
        return (now + timedelta(hours=skippedHours)).replace(minute=0, second=0, microsecond=0)
    return now + timedelta(minutes=times[index])

if not os.path.exists(filePath):
    init()
else:
    load()