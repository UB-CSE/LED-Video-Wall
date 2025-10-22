import yaml
import hashlib
import subprocess, os, signal
import atexit
import signal
from flask import Flask, request, jsonify, send_from_directory

# initialize an empty dictionary to store image coordinates, (x, y)
imageCoords = {}

app = Flask(__name__)

server_process = None
CONFIG_DIR = "../../server"
config_File = None 


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

    try:
        with open("/tmp/led-cmd", "w") as fp:
            fp.write(f"move {imgId} {x} {y}\n")

    except FileNotFoundError:
        print("ERROR")


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

            try:
                with open("/tmp/led-cmd", "w") as fp:
                    fp.write(f"move {element['id']} {element['location'][0]} {element['location'][1]}\n")

            except FileNotFoundError:
                print("ERROR")
            
        return "Success: config file has been updated"  # Responds with success message
    except FileNotFoundError:
        return jsonify({"[ERROR]: Configuration file, {config_file}, not found"}), 404


@app.route("/api/upload-file", methods=["POST"])
def upload_file():
    file = request.files["file"]
    contents = file.stream.read()
    file.stream.seek(0)
    hashString = hashlib.sha256(contents).hexdigest()       # Takes a hash over the contents of the file
    extension = file.filename.rsplit('.', 1)[1]                # Finds the extension
    filename = hashString + '.' + extension
    filepath = os.path.join("./static/images", filename)    #Combines into filepath
    file.save(filepath)                                     #Saves to disk
    return jsonify({'filename': filename})


@app.route("/api/images/<filename>", methods=["GET"])
def get_image(filename):
    return send_from_directory("./static/images", filename)


@app.route("/api/start-server", methods=['POST'])
def start_server():
    global server_process

    if server_process is not None:
        return jsonify({"error": "Server is already running"}), 400

    # Get config file from frontend JSON body
    user_path = request.json.get("config_file")
    if not user_path:
        return jsonify({"error": "No configuration file specified"}), 400

    # Convert to absolute path
    abs_path = os.path.abspath(user_path)
    if not os.path.exists(abs_path):
        print(f"[ERROR]: File not found -> {abs_path}")
        return jsonify({"error": f"Configuration file not found: {abs_path}"}), 404

    server_config_File = abs_path  

    try:
        # Important: run led-wall-server in same directory as the executable
        exe_dir = os.path.abspath("../../server")
        
        subprocess.run(
            ["make"], cwd=exe_dir, capture_output=True, text=True
        )
        
        server_process = subprocess.Popen(
            ["./led-wall-server", server_config_File],
            cwd=exe_dir,
            preexec_fn = os.setsid
        )
        print(f"[INFO]: Server started with config: {server_config_File}")
        return jsonify({"status": "Server starting", "config_file": server_config_File}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to start server -> {e}")
        return jsonify({"error": f"Server couldn't be started: {str(e)}"}), 500



@app.route("/api/stop-server", methods=['POST'])
def stop_server():
    global server_process
    if server_process is None:
        print("[ERROR]: Server not currently running")
        return jsonify({"error": "Server not currently running"}), 400

    try:
        os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
        server_process = None
        print("[INFO]: Server stopped successfully")
        return jsonify({"status": "Server stopped"})
    except Exception as e:
        print(f"[ERROR]: Server couldn't be stopped -> {e}")
        return jsonify({"error": "Server couldn't be stopped"}), 500
    
def clean_server():
    global server_process
    if server_process is not None:
        try:
            os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
            server_process = None
        except ProcessLookupError:
            pass

atexit.register(clean_server)
signal.signal(signal.SIGINT, clean_server)
signal.signal(signal.SIGTERM, clean_server)
    

@app.route("/api/list-configs", methods=['GET'])
def list_configs():
    try:
        # List all .yaml files in CONFIG_DIR
        files = [f for f in os.listdir(CONFIG_DIR) if f.endswith(".yaml") and f.lower() != "matrix.yaml"]
        # Return full relative paths so frontend can send them to /start_server
        files_with_path = [os.path.join(CONFIG_DIR, f) for f in files]
        print(files_with_path)
        return jsonify({"configs": files_with_path})
    except Exception as e:
        print(f"[ERROR]: Failed to list config files -> {e}")
        return jsonify({"error": "Could not list config files"}), 500
    
@app.route("/api/update-config", methods=["POST"])
def update_config():
    global config_File
    data = request.get_json()
    selected = data.get("config_file")

    if not selected:
        return jsonify({"error": "No configuration file provided"}), 400

    abs_path = os.path.abspath(selected)
    if not os.path.exists(abs_path):
        return jsonify({"error": f"Configuration file not found: {abs_path}"}), 404

    config_File = abs_path
    print(f"[INFO]: Configuration file selected -> {config_File}")
    return jsonify({"status": "Config selected", "config_file": config_File}), 200




if __name__ == "_main_":
    app.run(host="0.0.0.0")
