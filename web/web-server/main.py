import yaml
import json

from flask import Flask, request, jsonify

# initialize an empty dictionary to store image coordinates, (x, y)
imageCoords = {}

app = Flask(__name__)

config_File = "../../server/input-image-test.yaml"  # will change so that config_File could be any yaml file saved in the repo, not just input-conway


@app.route("/api/send-location", methods=["POST"])
def send_location():
    data = (
        request.get_json()
    )  # parses the data as JSON. JSON expected: {"x": 104, "y": 283, "id": "jpeg1"}
    x = data.get("x")
    y = data.get("y")
    imgId = data.get("id")
    imageCoords[imgId] = {
        "x": x,
        "y": y,
    }  # stores coordinates coordinates given by the JSON message

    if x is None or y is None or imgId is None:
        print("[ERROR]: Invalid message received")
        return jsonify({"[ERROR]: Invalid message received"}), 400

    print("Data:")
    print(f"Image Coordinates: ({x}, {y})")
    print("Image ID: " + imgId)
    return "Main Communication"


@app.route("/api/get-yaml-config", methods=["GET"])
def get_yaml_config():
    try:
        with open(config_File, "r") as file:
            config_Data = yaml.safe_load(file)

        if "settings" not in config_Data or "elements" not in config_Data:
            print("[ERROR]: Invalid configuration format")
            jsonify({"[ERROR]: Invalid configuration format"})

        return jsonify(
            config_Data
        )  # Sends the client a JSON file that follows the YAML configuration
    except FileNotFoundError:
        return jsonify({"[ERROR]: Configuration file, {config_file}, not found"}), 404


@app.route("/api/set-yaml-config", methods=["POST"])
def set_yaml_config():
    try:
        with open(config_File, "w") as file:
            config = (
                request.get_json()
            )  # parses the data as JSON. JSON expected: {'settings': {'gamma': number},
            #                                     'elements': {'elem1': 'id': number,
            #                                                           'type': string,
            #                                                           'filepath': string,
            #                                                           'location': number[]},
            #                                                  'elem2'...}}
            # Converts the JSON to yaml manually in order to fit the expected yaml format
            yaml_string = (
                "settings:\n  gamma: "
                + str(config["settings"]["gamma"])
                + "\nelements:"
            )
            for name in config["elements"]:
                element = config["elements"][name]
                yaml_string = (
                    yaml_string
                    + '\n  "'
                    + name
                    + '":\n    id: '
                    + str(element["id"])
                    + '\n    type: "'
                    + element["type"]
                    + '"'
                    + '\n    filepath: "'
                    + element["filepath"]
                    + '"'
                    + "\n    location: ["
                    + str(element["location"][0])
                    + ","
                    + str(element["location"][1])
                    + "]"
                )
            file.write(yaml_string)
        return "Success: config file has been updated"  # Responds with success message
    except FileNotFoundError:
        return jsonify({"[ERROR]: Configuration file, {config_file}, not found"}), 404


if __name__ == "_main_":
    app.run(host="0.0.0.0")
