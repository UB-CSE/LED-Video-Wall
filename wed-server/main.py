
from flask import Flask, request, jsonify
#initialize an empty dictionary to store image coordinates, (x, y)
imageCoords = {}

app = Flask(__name__)

@app.route('/get_Data', methods = ['GET', 'POST'])
def get_Data():
    data = request.get_json() #parses the data as JSON. JSON expected: {"x": 104, "y": 283, "id": "jpeg1"}
    x = data.get("x") 
    y = data.get("y")
    imgId = data.get("id")
    imageCoords[imgId] = {"x":x, "y":y} #stores coordinates coordinates given by the JSON message 

    if (x is None or y is None or imgId is None):
        print("[ERROR]: Invalid message received")
        return jsonify({"[ERROR]: Invalid message received"}), 400



    print("Data:")
    print(f"Image Coordinates: ({x}, {y})")
    print("Image ID: "+ imgId)
    return "Main Communication"

if __name__ == "_main_":
    app.run(host="0.0.0.0")