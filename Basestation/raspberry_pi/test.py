import objectDetector
import numpy as np
import cv2
from flask import render_template, send_from_directory, Flask, request

app = Flask(__name__)

objectDetector.init('checkpoints\\yolov4-tiny-416.tflite')


@app.route("/image", methods=['POST'])
def process():
    image = request.get_data()
    image = np.fromstring(image, np.uint8)
    orig = cv2.imdecode(image, cv2.IMREAD_COLOR) 
    image = cv2.imdecode(image, cv2.IMREAD_COLOR) 
    
    predboxes = objectDetector.detect(image,iou=0.45, score=0.5)
    objectDetector.draw(orig, predboxes)
    
    
    ##do all image processing and return json response
    return 'image: success'






if __name__ == '__main__':
    try:
        app.run()
    except Exception as e:
        print(e)