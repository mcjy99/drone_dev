from flask import Flask, Response
from picamera2 import Picamera2
from PIL import Image
import io
import time

app = Flask(__name__)

def gen_frames(camera):
    while True:
        frame = camera.capture_array()  # Capture frame from the camera

        # Convert the NumPy array to a PIL image
        pil_image = Image.fromarray(frame)
        
         # Rotate the image 180 degrees
        pil_image = pil_image.rotate(180)

        # Convert RGBA to RGB if necessary
        if pil_image.mode == 'RGBA':
            pil_image = pil_image.convert('RGB')
        
        # Convert the PIL image to JPEG format
        buffer = io.BytesIO()
        pil_image.save(buffer, format='JPEG')
        frame = buffer.getvalue()
        
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')


@app.route('/')
def index():
    """Video streaming home page."""
    return '<html><body><h1>Raspberry Pi Camera Stream</h1><img src="/video_feed"></body></html>'

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(camera),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    camera = Picamera2()
    camera.configure(camera.create_preview_configuration())
    camera.start()
    time.sleep(2)  # Allow camera to warm up
    app.run(host='0.0.0.0', port=5000)
