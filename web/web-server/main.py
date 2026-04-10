import yaml
import hashlib
import subprocess, os, signal
import atexit
import signal
import copy
import magic
from flask import Flask, request, jsonify, send_from_directory
from werkzeug.utils import secure_filename

from led_video_wall import LedVideoWall

imageCoords = {}

app = Flask(__name__)

server_process = None
CONFIG_DIR = "../../server"
FONT_DIR = "../../server/ttf"
config_File = None
currently_running_file = ""

VIDEO_DIR = os.path.join(CONFIG_DIR, "videos")
THUMBNAIL_DIR = os.path.join(VIDEO_DIR, "thumbnails")
MAX_VIDEO_SIZE = 50 * 1024 * 1024


@app.route("/api/send-location", methods=["POST"])
def send_location():
    data = request.get_json()
    x = data.get("x")
    y = data.get("y")
    imgId = data.get("id")
    imageCoords[imgId] = {"x": x, "y": y}

    if x is None or y is None or imgId is None:
        print("[ERROR]: Invalid message received")
        return jsonify({"[ERROR]: Invalid message received"}), 400

    if server_process is not None:
        try:
            LedVideoWall.move(imgId, x, y)
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

        return jsonify(config_Data)
    except FileNotFoundError:
        return jsonify({"[ERROR]: Configuration file, {config_file}, not found"}), 404


@app.route("/api/set-yaml-config", methods=["POST"])
def set_yaml_config():
    try:
        with open(config_File, "w") as file:
            config = request.get_json()
            yaml_string = (
                "settings:\n  gamma: "
                + str(config["settings"]["gamma"])
                + "\nelements:"
            )
            print(config["elements"])
            sorted_elements = sorted(config["elements"].items(), key=lambda x: x[1]["id"])
            for name, element in sorted_elements:
                        if element["type"] == "image":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "' + element["type"] + '"'
                                + '\n    filepath: "' + element["filepath"] + '"'
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                                + "\n    scale: " + str(element["scale"])
                            )
                        elif element["type"] == "text":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "' + element["type"] + '"'
                                + '\n    content: "' + element["content"] + '"'
                                + "\n    size: " + str(element["size"])
                                + "\n    color: " + '"' + str(element["color"]) + '"'
                                + "\n    font_path: " + '"' + element["font_path"] + '"'
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                            )
                        elif element["type"] == "carousel":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "carousel"'
                                + "\n    filepaths:\n"
                                + "".join(f'      - "{fp}"\n' for fp in element.get("filepaths", []))
                                + "    framerate: " + str(element.get("framerate", 1))
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                            )
                        elif element["type"] == "video":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "video"'
                                + '\n    filepath: "' + element.get("filepath", "") + '"'
                                + "\n    framerate: " + str(element.get("framerate", 30))
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                            )
                        elif element["type"] == "webcam":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "webcam"'
                                + "\n    camera-number: " + str(element.get("camera_number", 0))
                                + "\n    framerate: " + str(element.get("framerate", 30))
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                            )
                        elif element["type"] == "rtmp":
                            yaml_string = (
                                yaml_string
                                + '\n  "' + name + '":\n    id: ' + str(element["id"])
                                + '\n    type: "rtmp"'
                                + '\n    stream-name: "' + element.get("stream_name", "") + '"'
                                + "\n    framerate: " + str(element.get("framerate", 30))
                                + "\n    location: [" + str(element["location"][0]) + "," + str(element["location"][1]) + "]"
                            )
                            if element.get("size") and len(element["size"]) == 2:
                                yaml_string += "\n    size: [" + str(element["size"][0]) + "," + str(element["size"][1]) + "]"
            print(f"[DEBUG] yaml_string = {yaml_string}")
            file.write(yaml_string)
        return "Success: config file has been updated"
    except Exception as e:
        print(f"[ERROR]: set_yaml_config failed -> {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500


@app.route("/api/upload-file", methods=["POST"])
def upload_file():
    file = request.files["file"]
    contents = file.stream.read()
    mime_type = magic.from_buffer(contents, mime=True)
    if not mime_type.startswith("image"):
        return "[ERROR]: Incorrect file type: {mime_type}", 415
    file.stream.seek(0)
    hashString = hashlib.sha256(contents).hexdigest()
    extension = file.filename.rsplit('.', 1)[1]
    filename = hashString + '.' + extension
    filepath = os.path.join("../../server/images", filename)
    file.save(filepath)
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

    thumbnail_name = f"{base_name}.jpg"
    thumbnail_path = os.path.join(THUMBNAIL_DIR, thumbnail_name)

    try:
        subprocess.run([
            "ffmpeg", "-y", "-i", video_path,
            "-ss", "00:00:00", "-vframes", "1", thumbnail_path
        ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        print(f"[ERROR]: FFmpeg failed -> {e}")
        return jsonify({"[ERROR]": "Failed to generate thumbnail"}), 500

    return jsonify({
        "status": "success",
        "video_filename": filename,
        "thumbnail_filename": thumbnail_name
    }), 201

@app.route("/api/videos/<filename>", methods=["GET"])
def get_video(filename):
    return send_from_directory(VIDEO_DIR, filename)

@app.route("/api/video-thumbnails/<filename>", methods=["GET"])
def get_video_thumbnail(filename):
    return send_from_directory(THUMBNAIL_DIR, filename)


@app.route("/api/start-server", methods=['POST'])
def start_server():
    global server_process

    if server_process is not None:
        return jsonify({"error": "Server is already running"}), 400

    user_path = request.json.get("config_file")
    if not user_path:
        return jsonify({"error": "No configuration file specified"}), 400

    abs_path = os.path.abspath(user_path)
    if not os.path.exists(abs_path):
        print(f"[ERROR]: File not found -> {abs_path}")
        return jsonify({"error": f"Configuration file not found: {abs_path}"}), 404

    server_config_File = abs_path

    try:
        exe_dir = os.path.abspath("../../server")
        subprocess.run(["make"], cwd=exe_dir, capture_output=True, text=True)

        if app.debug:
            cmd = ["./led-wall-server", server_config_File]
        else:
            cmd = ["./led-wall-server", server_config_File, "--prod"]

        server_process = subprocess.Popen(cmd, cwd=exe_dir, preexec_fn=os.setsid)
        global currently_running_file
        currently_running_file = config_File
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
        global currently_running_file
        currently_running_file = ""
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


def signal_handling(signum=None, frame=None):
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
        files = [f for f in os.listdir(CONFIG_DIR) if f.endswith(".yaml") and f.lower() != "matrix.yaml"]

        valid_files = []
        for f in files:
            try:
                with open(os.path.join(CONFIG_DIR, f), "r") as file:
                    is_valid = True
                    config_Data = yaml.safe_load(file)
                    if "settings" not in config_Data or "elements" not in config_Data:
                        is_valid = False
                    for name in config_Data.get("elements", {}):
                        element = config_Data["elements"][name]
                        if "type" not in element:
                            is_valid = False
                            break
                        if element["type"] == "image":
                            if "id" not in element or "filepath" not in element or "location" not in element or "scale" not in element:
                                is_valid = False
                        elif element["type"] == "text":
                            if "id" not in element or "content" not in element or "size" not in element or "color" not in element or "font_path" not in element or "location" not in element:
                                is_valid = False
                    for name in config_Data.get("elements", {}):
                        element = config_Data["elements"][name]
                        # WHEN ADDING NEW ELEMENT TYPES, UPDATE THIS LIST
                        if element["type"] not in ["image", "text", "carousel", "video", "webcam", "rtmp"]:
                            is_valid = False
                            break
                    if is_valid:
                        valid_files.append(f)
            except Exception as e:
                continue

        files_with_path = [os.path.join(CONFIG_DIR, f) for f in valid_files]
        print(files_with_path)
        return jsonify({"configs": files_with_path})
    except Exception as e:
        print(f"[ERROR]: Failed to list config files -> {e}")
        return jsonify({"error": "Could not list config files"}), 500


@app.route("/api/list-fonts", methods=['GET'])
def list_fonts():
    try:
        files = [f for f in os.listdir(FONT_DIR) if f.endswith(".ttf")]
        file_paths = [os.path.join("ttf", f) for f in files]
        return jsonify({"fonts": file_paths})
    except Exception as e:
        print(f"[ERROR]: Failed to list font files -> {e}")
        return jsonify({"error": "Could not list font files"}), 500

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

@app.route("/api/get-current-config", methods=["GET"])
def get_current_config():
    return currently_running_file

@app.route("/api/reorder-layers", methods=["POST"])
def reorder_layers():
    global config_File
    json_package = request.get_json()
    new_order = json_package.get("layer_list")

    if not config_File:
        return jsonify({"[ERROR]": "No configuration file selected"}), 400
    if not isinstance(new_order, list):
        return jsonify({"[ERROR]": "There must be a list of element names"}), 400

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
        return jsonify({"status": "success", "reordered_to": list(new_elements.keys())}), 200

    except Exception as e:
        print(f"[ERROR]: Failed to reorder layers, {e}")
        return jsonify({"[ERROR]: Failed to reorder layers, {e}"}), 500

@app.route("/api/delete-layer", methods=["POST"])
def delete_layer():
    global config_File
    json_package = request.get_json()
    name = json_package.get("name")

    if not config_File:
        return jsonify({"[ERROR]": "No configuration file selected"}), 400

    try:
        with open(config_File, "r") as f:
            data = yaml.safe_load(f) or {"settings": {}, "elements": {}}

        elements = data.get("elements", {})
        delete = None

        if name:
            if name in elements:
                delete = elements.pop(name)
            else:
                return jsonify({"[ERROR]": f"Element named '{name}' not found"}), 404

        data["elements"] = elements
        with open(config_File, "w") as f:
            yaml.safe_dump(data, f, sort_keys=False)

        print(f"[INFO]: Deleted element '{name}' from config")
        return jsonify({"status": "deleted", "name": name, "removed": delete}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to delete layer -> {e}")
        return jsonify({"[ERROR]": str(e)}), 500

@app.route("/api/new-config", methods=["POST"])
def new_config():
    global CONFIG_DIR, config_File
    json_package = request.get_json() or {}
    filename = json_package.get("filename")

    os.makedirs(CONFIG_DIR, exist_ok=True)

    if not filename:
        base = "new-config"
        i = 1
        while True:
            canidate = f"{base}_{i}.yaml"
            path = os.path.join(CONFIG_DIR, canidate)
            if not os.path.exists(path):
                filename = canidate
                break
            i += 1

    target_path = os.path.abspath(os.path.join(CONFIG_DIR, filename))

    if os.path.exists(target_path):
        return jsonify({"[ERROR]": f"File already exists: {target_path}"}), 400

    template = {"settings": {"gamma": 1.0}, "elements": {}}

    try:
        with open(target_path, "w") as f:
            yaml.safe_dump(template, f, sort_keys=False)

        config_File = target_path
        print(f"[INFO]: Created new config -> {config_File}")
        return jsonify({"status": "created", "config_file": config_File}), 201
    except Exception as e:
        print(f"[ERROR]: Failed to create new config -> {e}")
        return jsonify({"error": str(e)}), 500

@app.route("/api/save-config-as", methods=["POST"])
def save_config_as():
    global CONFIG_DIR, config_File
    json_package = request.get_json()
    new_name = json_package.get("new_name")
    print(config_File)

    if not new_name:
        return jsonify({"[ERROR]": "No filename provided"}), 400

    if not new_name.endswith(".yaml"):
        new_name += ".yaml"

    new_path = os.path.join(CONFIG_DIR, new_name)

    if not config_File or not os.path.exists(config_File):
        return jsonify({"[ERROR]": "No active configuration file to copy"}), 400

    try:
        with open(config_File, "r") as src:
            current_config = src.read()
        with open(new_path, "w") as dest:
            dest.write(current_config)

        print(f"[INFO]: Configuration copied to {new_path}")
        config_File = new_path
        return jsonify({"status": "success", "new_config_file": new_path}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to save config -> {e}")
        return jsonify({"error": f"Failed to save config: {str(e)}"}), 500

@app.route("/api/duplicate-layer", methods=["POST"])
def duplicate_layer():
    global config_File
    json_package = request.get_json()
    name = json_package.get("name")

    if not config_File:
        return jsonify({"error": "No configuration file selected"}), 400
    if not name:
        return jsonify({"error": "No element name provided"}), 400

    try:
        with open(config_File, "r") as f:
            data = yaml.safe_load(f) or {"settings": {}, "elements": {}}

        elements = data.get("elements", {})
        if name not in elements:
            return jsonify({"error": f"Element '{name}' not found"}), 404

        base = name + "_copy"
        new_name = base
        i = 1
        while new_name in elements:
            new_name = f"{base}_{i}"
            i += 1

        new_element = copy.deepcopy(elements[name])
        new_id = max(el["id"] for el in elements.values()) + 1
        new_element["id"] = new_id
        if "location" in new_element and len(new_element["location"]) == 2:
            new_element["location"] = [new_element["location"][0] + 1, new_element["location"][1] + 1]

        print(f"[INFO]: Duplicated '{name}' as '{new_name}' with id {new_id} (not yet saved)")
        return jsonify({"status": "success", "new_name": new_name, "new_id": new_id}), 201

    except Exception as e:
        print(f"[ERROR]: Failed to duplicate layer -> {e}")
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)