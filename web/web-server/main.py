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

    if server_process is not None:
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

        if app.debug:
            cmd = ["./led-wall-server", server_config_File]
        else:
            cmd = ["./led-wall-server", server_config_File, "--prod"]
        
        server_process = subprocess.Popen(
            cmd,
            cwd=exe_dir,
            preexec_fn = os.setsid
        )
        print(f"[INFO]: Server started with config: {server_config_File}")
        if not app.debug:
            print("[INFO]: Flask not in debug mode, '--prod' flag added")
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


def signal_handling(signum = None, frame = None):
    clean_server()
    os._exit(0)

signal.signal(signal.SIGINT, signal_handling)
signal.signal(signal.SIGTERM, signal_handling)

try:
    signal.signal(signal.SIGHUP, signal_handling)
except AttributeError:
    print("[ERROR]: SIGHUP not supported on this platform")

atexit.register(clean_server)
    

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


@app.route("/api/get-matrix-config", methods=["GET"])
def get_matrix_config():
    matrix_config_path = os.path.join(CONFIG_DIR, "config.yaml")
    if not os.path.exists(matrix_config_path):
        print("[ERROR]: LED matrix configuration not Found")
        return jsonify({"[ERROR]: LED matrix configuration not Found"}), 404
    
    try:
        with open(matrix_config_path, "r") as file:
            matrix_config_data = yaml.safe_load(file)

        return jsonify(matrix_config_data)
    except Exception as e:
        print("[ERROR]: Failed to read LED configuration")
        return jsonify({"[ERROR]: Failed to read LED configuration"})

# accepts JSON: {"layer_list": ["elem1", "elem2", ....]}   
@app.route("/api/reorder-layers", methods = ["POST"])
def reorder_layers():
    global config_File
    json_package = request.get_json()
    new_order = json_package.get("layer_list") #expects JSON to send a list of the new order of layers: ["elem1", "elem3", "elem2"]

    if not config_File:
        return jsonify({"[ERROR]: No configuration file selected"}), 400
    if not isinstance(new_order, list):
        return jsonify({"[ERROR]: There must be a list of element names"}), 400
    
    try:
        with open(config_File, "r") as f:
            data = yaml.safe_load(f) or {"settings": {}, "elements": {}}
        
        elements = data.get("elements", {})
        new_elements = {}
        for name in new_order:
            if name in elements:
                new_elements[name] = elements[name]
            else:
                print(f"[WARNING]: '{name}' not found in config")
        
        for name, value in elements.items():
            if name not in new_elements:
                new_elements[name] = value
        
        data["elements"] = new_elements

        with open(config_File, "w") as f:
            yaml.safe_dump(data, f, sort_keys=False)
        
        print(f"[INFO]: Layers reordered to {list(new_elements.keys())}")

        return jsonify({"[STATUS]: sucess, reordered to {list(new_elements.keys()"}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to reorder layers, {e}")
        return jsonify({"[ERROR]: Failed to reorder layers, {e}"}), 500

#accepts JSON: {"name": "elem1"}
@app.route("/api/delete-layer", methods = ["POST"])
def delete_layer():
    global config_File
    json_package = request.get_json()
    name = json_package.get("name") #delete a layer based on the name of the element assigned to that layer

    if not config_File:
        return jsonify({"[ERROR]: No configuration file selected"}), 400
    
    try:
        with open(config_File, "r") as f:
            data = yaml.safe_load(f) or {"settings": {}, "elements": {}}
        
        elements = data.get("elements", {})
        delete = None

        if name:
            if name in elements:
                delete = elements.pop(name)
            else:
                return jsonify({"error": f"Element named '{name}' not found"}), 404
            
        data["elements"] = elements
        with open(config_File, "w") as f:
            yaml.safe_dump(data, f, sort_keys=False)
        
        print(f"[INFO]: Deleted element '{name}' from config")
        return jsonify({"status": "deleted", "name": name, "removed": delete}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to delete layer -> {e}")
        return jsonify({"error": str(e)}), 500
    
#can accept JSON: {"filename": newconfig.yaml} <---- this is for if the user wants to give the file a custom name
@app.route("/api/new-config", methods = ["POST"])
def new_config():
    global CONFIG_DIR, config_File
    json_package = request.get_json() or {}
    filename = json_package.get("filename")

    os.makedirs(CONFIG_DIR, exist_ok=True)

    if not filename:  # if there is no filename, then a generic name is given  
        base = "new-config"
        i=1
        while True:
            canidate = f"{base}_{i}.yaml"
            path = os.path.join(CONFIG_DIR, canidate)
            if not os.path.exists(path):
                filename = canidate
                break
            i += 1

    target_path = os.path.abspath(os.path.join(CONFIG_DIR, filename))

    if os.path.exists(target_path):
        return jsonify({"[ERROR]: File already exists: {target_path}"}), 400
    
    template = {"settings": {"gamma": 1.0},
                "elements": {}
                }
    
    try: 
        with open(target_path, "w") as f:
            yaml.safe_dump(template, f, sort_keys=False)
        
        config_File = target_path
        print(f"[INFO]: Created new config -> {config_File}")
        return jsonify({"status": "created", "config_file": config_File}), 201
    except Exception as e:
        print(f"[ERROR]: Failed to create new config -> {e}")
        return jsonify({"error": str(e)}), 500
    
 #accepts JSON: {"new_name": "config_file.yaml"}   
@app.route("/api/save-config-as", methods = ["POST"])
def save_config_as():
    global CONFIG_DIR, config_File
    json_package = request.get_json()
    new_name = json_package.get("new_name")

    if not new_name:
        return jsonify({"[ERROR]: No filename provided"}), 400
    
    if not new_name.endswith(".yaml"):
        new_name += ".yaml"
    
    new_path = os.path.join(CONFIG_DIR, new_name)

    if not config_File or not os.path.exists(config_File):
        return jsonify({"[ERROR] No active configuration file to copy"}), 400
    
    try:
        with open(config_File, "r") as src:
            current_config = src.read()
        with open(new_path, "w") as dest:
            dest.write(current_config)
        
        print(f"[INFO]: Configuration copied to {new_path}")

        config_File = new_path
        return jsonify({ "status": "success",
            "new_config_file": new_path
        }), 200
    except Exception as e:
        print(f"[ERROR]: Failed to save config -> {e}")
        return jsonify({"error": f"Failed to save config: {str(e)}"}), 500






if __name__ == "_main_":
    app.run(host="0.0.0.0")
