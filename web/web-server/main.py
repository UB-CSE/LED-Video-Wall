import yaml
import hashlib
import subprocess, os, signal
import atexit
import signal
import magic
from flask import Flask, request, jsonify, send_from_directory
from werkzeug.utils import secure_filename

from led_video_wall import LedVideoWall

# initialize an empty dictionary to store image coordinates, (x, y)
imageCoords = {}

app = Flask(__name__)

server_processes = {}
CONFIG_DIR = "../../server"
FONT_DIR = "../../server/ttf"
config_Files = {}
currently_running_files = {}
rtmp_ports = {}

VIDEO_DIR = os.path.join(CONFIG_DIR, "videos")
THUMBNAIL_DIR = os.path.join(VIDEO_DIR, "thumbnails")
MAX_VIDEO_SIZE = 50 * 1024 * 1024


@app.route("/api/<int:ledvw_port>/send-location", methods=["POST"])
def send_location(ledvw_port):
    if ledvw_port not in server_processes:
        return jsonify({"error": f"[ERROR]: No server running on port {ledvw_port}"}), 400

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
        return jsonify({"error": "[ERROR]: Invalid message received"}), 400

    try:
        LedVideoWall.move(ledvw_port, imgId, x, y)

    except FileNotFoundError:
        print("ERROR")

    print("Data:")
    print(f"Image Coordinates: ({x}, {y})")
    print("Image ID: " + imgId)
    return "Main Communication"


@app.route("/api/<int:ledvw_port>/get-yaml-config", methods=["GET"])
def get_yaml_config(ledvw_port):
    if ledvw_port not in config_Files:
        return jsonify({"error": f"[ERROR]: No configuration file in use by server on port {ledvw_port}"}), 400

    try:
        with open(config_Files[ledvw_port], "r") as file:
            config_Data = yaml.safe_load(file)

        if "settings" not in config_Data or "elements" not in config_Data:
            print("[ERROR]: Invalid configuration format")
            return jsonify({"error": "[ERROR]: Invalid configuration format"}), 400

        return jsonify(
            config_Data
        )  # Sends the client a JSON file that follows the YAML configuration
    except FileNotFoundError:
        return jsonify({"error": f"[ERROR]: Configuration file, {config_Files[ledvw_port]}, not found"}), 404


@app.route("/api/<int:ledvw_port>/set-yaml-config", methods=["POST"])
def set_yaml_config(ledvw_port):
    if ledvw_port not in config_Files:
        return jsonify({"error": f"[ERROR]: No configuration file in use by server on port {ledvw_port}"}), 400

    try:
        with open(config_Files[ledvw_port], "w") as file:
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
            print(config["elements"])
            for i in range(len(config["elements"])):
                for name in config["elements"]:
                    element = config["elements"][name]
                    if element["id"] == i + 1:
                        if element["type"] == "image":
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
                            + "\n    scale: "
                            + str(element["scale"])
                            )
                        elif element["type"] == "text":
                            yaml_string = (
                            yaml_string
                            + '\n  "'
                            + name
                            + '":\n    id: '
                            + str(element["id"])
                            + '\n    type: "'
                            + element["type"]
                            + '"'
                            + '\n    content: "'
                            + element["content"]
                            + '"'
                            + "\n    size: "
                            + str(element["size"])
                            + "\n    color: "
                            + '"'
                            + str(element["color"])
                            + '"'
                            + "\n    font_path: "
                            + '"'
                            + element["font_path"]
                            + '"'
                            + "\n    location: ["
                            + str(element["location"][0])
                            + ","
                            + str(element["location"][1])
                            + "]")
            file.write(yaml_string)

        return "Success: config file has been updated"  # Responds with success message
    except FileNotFoundError:
        return jsonify({"error": f"[ERROR]: Configuration file, {config_Files[ledvw_port]}, not found"}), 404


@app.route("/api/upload-file", methods=["POST"])
def upload_file():
    file = request.files["file"]
    contents = file.stream.read()
    mime_type = magic.from_buffer(contents, mime=True)
    if not mime_type.startswith("image"):
        return "[ERROR]: Incorrect file type: {mime_type}", 415
    file.stream.seek(0)
    hashString = hashlib.sha256(contents).hexdigest()       # Takes a hash over the contents of the file
    extension = file.filename.rsplit('.', 1)[1]                # Finds the extension
    filename = hashString + '.' + extension
    filepath = os.path.join("../../server/images", filename)    #Combines into filepath
    file.save(filepath)                                     #Saves to disk
    return jsonify({'filename': filename})


@app.route("/api/images/<filename>", methods=["GET"])
def get_image(filename):
    return send_from_directory("../../server/images", filename)

@app.route("/api/fonts/<filename>", methods=["GET"])
def get_font(filename):
    return send_from_directory("../../server/ttf", filename, mimetype='font/ttf')

@app.before_request
def limit_video_upload_size():
    if request.path == "/api/upload-video" and request.content_length:
        if request.content_length > MAX_VIDEO_SIZE:
            return jsonify({"error": "Video exceeds 50 MB upload limit"}), 413

@app.route("/api/upload-video", methods=["POST"])
def upload_video():
    if "file" not in request.files:
        return jsonify({"error": "No file provided"}), 400

    file = request.files["file"]
    if file.filename == "":
        return jsonify({"error": "No filename provided"}), 400

    filename = secure_filename(file.filename)
    base_name, ext = os.path.splitext(filename)

    if ext.lower() not in [".mp4", ".mov", ".avi", ".mkv", ".webm"]:
        return jsonify({"error": "Unsupported video format"}), 400

    video_path = os.path.join(VIDEO_DIR, filename)
    file.save(video_path)

    thumbnail_name = f"{base_name}.jpg"  # generate thumbnail name and
    thumbnail_path = os.path.join(THUMBNAIL_DIR, thumbnail_name)

    # extract first frame using FFmpeg
    try:
        subprocess.run([
            "ffmpeg",
            "-y",               # overwrite if exists
            "-i", video_path,   # input video
            "-ss", "00:00:00",  # start time (first frame)
            "-vframes", "1",    # grab one frame
            thumbnail_path
        ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        print(f"[ERROR]: FFmpeg failed -> {e}")
        return jsonify({"error": "[ERROR]: Failed to generate thumbnail"}), 500

    print(f"[INFO]: Uploaded video saved -> {video_path}")
    print(f"[INFO]: Thumbnail saved -> {thumbnail_path}")

    return jsonify({
        "status": "success",
        "video_filename": filename,
        "thumbnail_filename": thumbnail_name
    }), 201

@app.route("/api/videos/<filename>", methods = ["GET"])
def get_video(filename):
    return send_from_directory(VIDEO_DIR, filename)

@app.route("/api/video-thumbnails/<filename>", methods = ["GET"])
def get_video_thumbnail(filename):
    return send_from_directory(THUMBNAIL_DIR, filename)


@app.route("/api/<int:ledvw_port>/start-server", methods=['POST'])
def start_server(ledvw_port):
    global server_processes
    global rtmp_ports

    if ledvw_port in server_processes:
        return jsonify({"error": "Server is already running"}), 400

    # Get config file from frontend JSON body
    user_path = request.json.get("config_file")
    if not user_path:
        return jsonify({"error": "No configuration file specified"}), 400

    rtmp_port = request.json.get("rtmp_port")
    if not rtmp_port:
        return jsonify({"error": "No RTMP port specified"}), 400

    if rtmp_port in rtmp_ports.values():
        return jsonify({"error": f"RTMP port {rtmp_port} is already in use by another server"}), 400

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
            cmd = ["./led-wall-server", server_config_File, f"--ledvw-port={ledvw_port}", f"--rtmp-port={rtmp_port}"]
        else:
            cmd = ["./led-wall-server", server_config_File, "--prod", f"--ledvw-port={ledvw_port}", f"--rtmp-port={rtmp_port}", "--ozone-platform=headless", "--disable-gpu"] # Server doesn't have a GPU!

        server_processes[ledvw_port] = subprocess.Popen(
            cmd,
            cwd=exe_dir,
            preexec_fn = os.setsid
        )
        rtmp_ports[ledvw_port] = rtmp_port
        global currently_running_files
        currently_running_files[ledvw_port] = server_config_File
        print(f"[INFO]: Server started with config: {server_config_File}")
        if not app.debug:
            print("[INFO]: Flask not in debug mode, '--prod' flag added")
        return jsonify({"status": "Server starting", "config_file": server_config_File}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to start server -> {e}")
        return jsonify({"error": f"Server couldn't be started: {str(e)}"}), 500



@app.route("/api/<int:ledvw_port>/stop-server", methods=['POST'])
def stop_server(ledvw_port):
    global server_processes
    if ledvw_port not in server_processes:
        print(f"[ERROR]: Server on port {ledvw_port} not currently running")
        return jsonify({"error": f"Server on port {ledvw_port} not currently running"}), 400

    try:
        os.killpg(os.getpgid(server_processes[ledvw_port].pid), signal.SIGTERM)
        server_processes.pop(ledvw_port)
        rtmp_ports.pop(ledvw_port)
        currently_running_files.pop(ledvw_port)
        print(f"[INFO]: Server on port {ledvw_port} stopped successfully")
        global currently_running_file
        currently_running_file = ""
        return jsonify({"status": f"Server on port {ledvw_port} stopped"}), 200
    except Exception as e:
        print(f"[ERROR]: Server on port {ledvw_port} couldn't be stopped -> {e}")
        return jsonify({"error": f"Server on port {ledvw_port} couldn't be stopped"}), 500

def clean_server():
    global server_processes
    for server_process in server_processes.values():
        if server_process is not None:
            try:
                os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
            except ProcessLookupError:
                pass
    server_processes.clear()
    rtmp_ports.clear()
    currently_running_files.clear()


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
        # List all .yaml files in CONFIG_DIR that start with "input"
        files = [f for f in os.listdir(CONFIG_DIR) if f.endswith(".yaml") and f.lower() != "matrix.yaml"]

        # Return full relative paths so frontend can send them to /start_server
        files_with_path = [os.path.join(CONFIG_DIR, f) for f in files]
        print(files_with_path)
        return jsonify({"configs": files_with_path})
    except Exception as e:
        print(f"[ERROR]: Failed to list config files -> {e}")
        return jsonify({"error": "Could not list config files"}), 500

@app.route("/api/list-running-servers", methods=['GET'])
def list_running_servers():
    return ",".join([f"[Server] LEDVW {ledvw_port} + RTMP {rtmp_port}" for ledvw_port, rtmp_port in rtmp_ports.items()])

@app.route("/api/list-fonts", methods=['GET'])
def list_fonts():
    try:
        # List all .ttf files in FONT_DIR
        files = [f for f in os.listdir(FONT_DIR) if f.endswith(".ttf")]

        file_paths = [os.path.join("ttf", f) for f in files]

        return jsonify({"fonts": file_paths})
    except Exception as e:
        print(f"[ERROR]: Failed to list font files -> {e}")
        return jsonify({"error": "Could not list font files"}), 500

@app.route("/api/<int:ledvw_port>/update-config", methods=["POST"])
def update_config(ledvw_port):
    global config_Files
    data = request.get_json()
    selected = data.get("config_file")

    if not selected:
        return jsonify({"error": "No configuration file provided"}), 400

    abs_path = os.path.abspath(selected)
    if not os.path.exists(abs_path):
        return jsonify({"error": f"Configuration file not found: {abs_path}"}), 404

    config_Files[ledvw_port] = abs_path
    print(f"[INFO]: Configuration file selected -> {config_Files[ledvw_port]}")
    return jsonify({"status": "Config selected", "config_file": config_Files[ledvw_port]}), 200


@app.route("/api/get-matrix-config", methods=["GET"])
def get_matrix_config():
    matrix_config_path = os.path.join(CONFIG_DIR, "config.yaml")
    if not os.path.exists(matrix_config_path):
        print("[ERROR]: LED matrix configuration not Found")
        return jsonify({"error": "[ERROR]: LED matrix configuration not Found"}), 404

    try:
        with open(matrix_config_path, "r") as file:
            matrix_config_data = yaml.safe_load(file)

        return jsonify(matrix_config_data)
    except Exception as e:
        print("[ERROR]: Failed to read LED configuration")
        return jsonify({"error": "[ERROR]: Failed to read LED configuration"})

@app.route("/api/<int:ledvw_port>/get-current-config", methods=["GET"])
def get_current_config(ledvw_port):
    if ledvw_port not in currently_running_files:
        return ""
    return currently_running_files[ledvw_port]

# accepts JSON: {"layer_list": ["elem1", "elem2", ....]}
@app.route("/api/<int:ledvw_port>/reorder-layers", methods = ["POST"])
def reorder_layers(ledvw_port):
    global config_Files
    json_package = request.get_json()
    new_order = json_package.get("layer_list") #expects JSON to send a list of the new order of layers: ["elem1", "elem3", "elem2"]

    if ledvw_port not in config_Files:
        return jsonify({"error": f"[ERROR]: No configuration file selected for server on port {ledvw_port}"}), 400
    if not isinstance(new_order, list):
        return jsonify({"error": "[ERROR]: There must be a list of element names"}), 400

    try:
        with open(config_Files[ledvw_port], "r") as f:
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

        with open(config_Files[ledvw_port], "w") as f:
            yaml.safe_dump(data, f, sort_keys=False)

        print(f"[INFO]: Layers reordered to {list(new_elements.keys())} in config for server on port {ledvw_port}")

        return jsonify({"status": "success","reordered_to": list(new_elements.keys())}), 200

    except Exception as e:
        print(f"[ERROR]: Failed to reorder layers for server on port {ledvw_port} -> {e}")
        return jsonify({"error": f"[ERROR]: Failed to reorder layers for server on port {ledvw_port}, {e}"}), 500

#accepts JSON: {"name": "elem1"}
@app.route("/api/<int:ledvw_port>/delete-layer", methods = ["POST"])
def delete_layer(ledvw_port):
    global config_Files
    json_package = request.get_json()
    name = json_package.get("name") #delete a layer based on the name of the element assigned to that layer

    if ledvw_port not in config_Files:
        return jsonify({"error": "[ERROR]: No configuration file selected for server on port {ledvw_port}"}), 400

    try:
        with open(config_Files[ledvw_port], "r") as f:
            data = yaml.safe_load(f) or {"settings": {}, "elements": {}}

        elements = data.get("elements", {})
        delete = None

        if name:
            if name in elements:
                delete = elements.pop(name)
            else:
                return jsonify({"error": f"[ERROR]: Element named '{name}' not found"}), 404

        data["elements"] = elements
        with open(config_Files[ledvw_port], "w") as f:
            yaml.safe_dump(data, f, sort_keys=False)

        print(f"[INFO]: Deleted element '{name}' from config")
        return jsonify({"status": "deleted", "name": name, "removed": delete}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to delete layer -> {e}")
        return jsonify({"error": f"[ERROR]: {str(e)}"}), 500

#can accept JSON: {"filename": newconfig.yaml} <---- this is for if the user wants to give the file a custom name
@app.route("/api/<int:ledvw_port>/new-config", methods = ["POST"])
def new_config(ledvw_port):
    global CONFIG_DIR, config_Files
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
        return jsonify({"error": f"[ERROR]: File already exists: {target_path}"}), 400

    template = {"settings": {"gamma": 1.0},
                "elements": {}
                }

    try:
        with open(target_path, "w") as f:
            yaml.safe_dump(template, f, sort_keys=False)

        config_Files[ledvw_port] = target_path
        print(f"[INFO]: Created new config -> {config_Files[ledvw_port]}")
        return jsonify({"status": "created", "config_file": config_Files[ledvw_port]}), 201
    except Exception as e:
        print(f"[ERROR]: Failed to create new config -> {e}")
        return jsonify({"error": str(e)}), 500

 #accepts JSON: {"new_name": "config_file.yaml"}
@app.route("/api/<int:ledvw_port>/save-config-as", methods = ["POST"])
def save_config_as(ledvw_port):
    global CONFIG_DIR, config_Files
    json_package = request.get_json()
    new_name = json_package.get("new_name")

    if not new_name:
        return jsonify({"error": "[ERROR]: No filename provided"}), 400

    if not new_name.endswith(".yaml"):
        new_name += ".yaml"

    new_path = os.path.join(CONFIG_DIR, new_name)



    if ledvw_port not in config_Files or not os.path.exists(config_Files[ledvw_port]):
        return jsonify({"error": "[ERROR]: No active configuration file to copy"}), 400

    try:
        with open(config_Files[ledvw_port], "r") as src:
            current_config = src.read()
        with open(new_path, "w") as dest:
            dest.write(current_config)

        print(f"[INFO]: Configuration copied to {new_path}")

        config_Files[ledvw_port] = new_path
        return jsonify({ "status": "success",
            "new_config_file": new_path
        }), 200
    except Exception as e:
        print(f"[ERROR]: Failed to save config -> {e}")
        return jsonify({"error": f"[ERROR]: Failed to save config: {str(e)}"}), 500









if __name__ == "__main__":
    app.run(host="0.0.0.0")
