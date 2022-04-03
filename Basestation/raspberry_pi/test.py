import scheduling.timeManagement as tm
from datetime import datetime

v = tm.parseIntervals()

print(tm.getNextTime(current=datetime.now()))