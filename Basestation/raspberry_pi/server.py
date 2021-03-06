import numpy as np
import argparse
import cv2
from PIL import Image
from io import BytesIO
from flask import render_template, send_from_directory, send_file, Flask, request
import threading
import time
from datetime import datetime, timedelta
import wittyPy
import scheduling.timeManagement as tm
import os
import sys
import logging
from timeit import default_timer as timer

processStart = timer()
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.FileHandler("debug.log"),
        logging.StreamHandler()
    ]
)

roverContactOccured = False
nextRoverstart = datetime.now()

app = Flask(__name__)
persistentImage = np.zeros(5)
images = []
times = []

# Init Argparser
parser = argparse.ArgumentParser(description='Optional app description')
# Required positional argument
parser.add_argument('modelpath', type=str,
                    help='The filepath to the tflite Model')

parser.add_argument('--wittypipath', type=str, required=False,
                    help='The folderpath to the wittypi')

# Required positional argument
parser.add_argument('--timeout', type=int, default=120, required=False,
                    help='The time answer to send to Roverstation')

# Required positional argument
parser.add_argument('--sleepdelay', type=int, default=10, required=False,
                    help='How long to wait between images before going back to sleep')

# Required positional argument
parser.add_argument('--initdelay', type=int, default=30, required=False,
                    help='How long to wait for the first image before going to sleep')


# Required positional argument
parser.add_argument('--bootdelay', type=int, default=30, required=False,
                    help='Time between Basestation boot and rover boot')

# Required positional argument
parser.add_argument('--score', type=float, default=0.5, required=False,
                    help='The minimum score to count as detection')

# Switch
parser.add_argument('--show', action='store_true',
                    help='When active the detected images will be shown on the Display (not available during headless operation)')
# Switch
parser.add_argument('--tiny', action='store_true',
                    help='Model is yolo tiny')

args = parser.parse_args()

# Init Detector
import objectDetector
objectDetector.init(args.modelpath, args.tiny)
if args.wittypipath is not None:
    witty = wittyPy.WittyPi(args.wittypipath)
delay = args.initdelay

@app.route("/image", methods=['POST'])
def process():
    global images
    global nextRoverstart
    global roverContactOccured
    
    data = request.get_data()
    logging.info(f"Received Image with: {len(data)} bytes")
    image = np.array(Image.open(BytesIO(data)))
    images.append(image)
    
    if roverContactOccured:
        roverTime = nextRoverstart
    else:
        roverTime = tm.getNextTime(datetime.now()) + timedelta(seconds=args.bootdelay)
        nextRoverstart = roverTime
        
    now = datetime.now()
    timeToNext = roverTime - now
    roversleep = timeToNext.total_seconds()
    logging.info(f"Next time for Rover is in {roversleep}seconds at {roverTime}")
    
    roverContactOccured = True
    
    return str(int(roversleep))

@app.route("/time", methods=['POST'])
def roverTime():
    global times
    duration = int(request.data.decode())
    times.append(duration)
    logging.info(times)
    return ('', 204)
    
@app.route("/average", methods=['GET'])
def averageDuration():
    global times
    return str(sum(times) / len(times))

@app.route("/image", methods=['GET'])
def retrieve():
    return serve_pil_image(persistentImage)

def serve_pil_image(pil_img):
    img_io = BytesIO()
    pil_img.save(img_io, 'JPEG', quality=100)
    img_io.seek(0)
    return send_file(img_io, mimetype='image/jpeg')


def worker():
    global images
    global persistentImage
    global delay
    while True:
        time.sleep(1)
        if (len(images) > 0):
            delay = args.sleepdelay
            logging.info(f"Detecting images...Images in buffer: {len(images)}")
            image = images.pop()
            orig = image.copy()
            start = timer()
            predboxes = objectDetector.detect(image,iou=0.45, score=args.score)
            _, _, _, [num_boxes] = predboxes
            persistentImage = objectDetector.draw(orig, predboxes, args.show)
            end = timer()
            logging.info(f"We detected: {num_boxes} cars..Detection took {timedelta(seconds=end-start)} seconds")
        else:
            if (delay == 0 and args.wittypipath is not None): 
                if roverContactOccured:
                    startTime = nextRoverstart - timedelta(seconds=args.bootdelay)
                else:
                    startTime = tm.getNextTime(datetime.now()) - timedelta(seconds=args.bootdelay)
                    
                witty.set_startup('??', startTime.hour, startTime.minute, startTime.second)
                processEnd = timer()
                logging.info(f"Shutting down now. Waking up in {(startTime - datetime.now()).total_seconds()} seconds at {startTime}...Complete Operation took {timedelta(seconds=processEnd-processStart)} seconds")
                os.system("sudo shutdown -h now")
                
                
                
            logging.info(f"Shutting down in {delay} seconds")
            delay -= 1
            
        
            
        

def main():
    try:
        x = threading.Thread(target=worker)
        x.start()
        app.run(host="0.0.0.0", port="5001")
    except Exception as e:
        logging.error(e)
        
        
        
        
        

if __name__ == '__main__':
    main()