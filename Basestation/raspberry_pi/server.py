import numpy as np
import argparse
import cv2
from PIL import Image
from io import BytesIO
from flask import render_template, send_from_directory, send_file, Flask, request
import threading
import time

app = Flask(__name__)
global persistentImage
global images
persistentImage = np.zeros(5)
images = []


# Init Argparser
parser = argparse.ArgumentParser(description='Optional app description')
# Required positional argument
parser.add_argument('modelpath', type=str,
                    help='The filepath to the tflite Model')

# Required positional argument
parser.add_argument('--timeout', type=int, default=120, required=False,
                    help='The time answer to send to Roverstation')

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

@app.route("/image", methods=['POST'])
def process():
    global images
    data = request.get_data()
    print(f"Received Image with: {len(data)} bytes")
    image = np.array(Image.open(BytesIO(data)))
    images.append(image)
    
    #do all image processing and return json response
    return str(args.timeout)


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
    while True:
        time.sleep(1)
        if (len(images) > 0):
            print(f"Detecting images...Images in buffer: {len(images)}")
            image = images.pop()
            orig = image.copy()
            predboxes = objectDetector.detect(image,iou=0.45, score=0.5)
            persistentImage = objectDetector.draw(orig, predboxes, args.show)
            print("Detection completed")
            
        

def main():
    try:
        x = threading.Thread(target=worker)
        x.start()
        app.run(host="0.0.0.0", port="5001")
    except Exception as e:
        print(e)
        
        
        
        
        

if __name__ == '__main__':
    main()