import numpy as np
import argparse
import cv2
from PIL import Image
from io import BytesIO
from flask import render_template, send_from_directory, send_file, Flask, request

app = Flask(__name__)
global persistentImage
persistentImage = np.zeros(5)



# Init Argparser
parser = argparse.ArgumentParser(description='Optional app description')
# Required positional argument
parser.add_argument('modelpath', type=str,
                    help='The filepath to the tflite Model')
# Switch
parser.add_argument('--show', action='store_true',
                    help='When active the detected images will be shown on the Display (not available during headless operation)')

args = parser.parse_args()

# Init Detector
import objectDetector
objectDetector.init(args.modelpath)

@app.route("/image", methods=['POST'])
def process():
    global persistentImage
    data = request.get_data()
    print(f"Received Image with: {len(data)}bytes")
    image = np.array(Image.open(BytesIO(data)))
    orig = image.copy()

    
    predboxes = objectDetector.detect(image,iou=0.45, score=0.5)
    if (args.show):
        persistentImage = objectDetector.draw(orig, predboxes)
    
    
    #do all image processing and return json response
    return '120'


@app.route("/image", methods=['GET'])
def retrieve():
    return serve_pil_image(persistentImage)

def serve_pil_image(pil_img):
    img_io = BytesIO()
    pil_img.save(img_io, 'JPEG', quality=100)
    img_io.seek(0)
    return send_file(img_io, mimetype='image/jpeg')


if __name__ == '__main__':
    try:
        app.run(host="0.0.0.0", port="5001")
    except Exception as e:
        print(e)