import json
import os
from datetime import datetime, timedelta

filePath = 'time.txt'
intervals = []
times = []

def init():
    global intervals
    intervals = getDefault()
    save()


def getDefault():
    intervals = []
    for i in range(24):
        intervals.append(10)
    return intervals
        
def save():
    global intervals
    with open(filePath, 'w') as f:
        json.dump((intervals, lastMeasurement), f)
        
def load():
    global intervals
    with open(filePath, 'r') as f:
        intervals = json.load(f)

def parseIntervals():
    global times
    today = datetime.now()
    currenttime = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    times = []
    while currenttime.day == today.day:
        while(intervals[currenttime.hour] == 0):
            if currenttime.day != today.day:
                break
            currenttime = (currenttime + timedelta(hours=1)).replace(minute=0, second=0, microsecond=0)
            
        if currenttime.day != today.day:
                times.append(times[0] + timedelta(days=1))
                break
        times.append(currenttime)
        currenttime = currenttime + timedelta(minutes=intervals[currenttime.hour])

def getNextTime(current):
    for time in times:
        if (current < time):
            return time

if not os.path.exists(filePath):
    init()
    parseIntervals()
else:
    load()
    parseIntervals()