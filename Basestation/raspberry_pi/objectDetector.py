import tensorflow as tf
physical_devices = tf.config.experimental.list_physical_devices('GPU')
if len(physical_devices) > 0:
    tf.config.experimental.set_memory_growth(physical_devices[0], True)
import core.utils as utils
from core.yolov4 import filter_boxes
from tensorflow.python.saved_model import tag_constants
from PIL import Image
import cv2
import numpy as np
from tensorflow.compat.v1 import ConfigProto
from tensorflow.compat.v1 import InteractiveSession

global interpreter
global tiny
global numClasses

def detect(image, iou : float, score : float):
    global interpreter
    global tiny
    
    STRIDES, ANCHORS, NUM_CLASS, XYSCALE = utils.load_config(tiny)
    input_size = 416
    
    # Preprocessing
    original_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    image_data = cv2.resize(original_image, (input_size, input_size))
    image_data = image_data / 255.
    
    # Weird Formatting
    image_data = np.asarray([image_data]).astype(np.float32)
    
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    interpreter.set_tensor(input_details[0]['index'], image_data)
    interpreter.invoke()
    pred = [interpreter.get_tensor(output_details[i]['index']) for i in range(len(output_details))]
    boxes, pred_conf = filter_boxes(pred[0], pred[1], score_threshold=0.25, input_shape=tf.constant([input_size, input_size]))
    

    boxes, scores, classes, valid_detections = tf.image.combined_non_max_suppression(
        boxes=tf.reshape(boxes, (tf.shape(boxes)[0], -1, 1, 4)),
        scores=tf.reshape(
            pred_conf, (tf.shape(pred_conf)[0], -1, tf.shape(pred_conf)[-1])),
        max_output_size_per_class=50,
        max_total_size=50,
        iou_threshold=iou,
        score_threshold=score
    )
    
    return [boxes.numpy(), scores.numpy(), classes.numpy(), valid_detections.numpy()]
    
    
    
    
def init(weights_path : str, isTinyModel : bool):
    global interpreter
    global tiny
    # Loading config
    # config = ConfigProto()
    # config.gpu_options.allow_growth = True
    # session = InteractiveSession(config=config)
    tiny = isTinyModel   
    interpreter = tf.lite.Interpreter(model_path=weights_path)
    interpreter.allocate_tensors()
    

def draw(image, predboxes, show):
    
    #image = cv2.cvtColor(np.array(image), cv2.COLOR_BGR2RGB)
    image = utils.draw_bbox(image, predboxes)
    image = Image.fromarray(image.astype(np.uint8))
    result = image.copy()
    if (show):
        image.show()
    image = cv2.cvtColor(np.array(image), cv2.COLOR_BGR2RGB)
    cv2.imwrite('output.jpg', image)
    return result