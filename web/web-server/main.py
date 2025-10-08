
import yaml
from flask import Flask, request, jsonify
import subprocess, os, signal

app = Flask(__name__)

imageCoords = {}
server_process = None
config_File = "../../server/input-image-test.yaml"
active_config = None
CONFIG_DIR = "../../server"


@app.route('/get_Data', methods=['GET', 'POST'])
def get_Data():
    data = request.get_json()
    x, y, imgId = data.get("x"), data.get("y"), data.get("id")

    if x is None or y is None or imgId is None:
        print("[ERROR]: Invalid message received")
        return jsonify({"error": "Invalid message received"}), 400

    imageCoords[imgId] = {"x": x, "y": y}
    print(f"Data received -> ID: {imgId}, Coordinates: ({x}, {y})")
    return jsonify({"status": "OK"})



@app.route('/get_yaml_Config', methods = ['GET'])
def get_yaml_Config():
    global config_File, active_config
    used_config = active_config or config_File


    try:
        with open(used_config, "r") as file:
            config_Data = yaml.safe_load(file)
        
        if ("settings" not in config_Data or "elements" not in config_Data):
            print("[ERROR]: Invalid configuration format")
            jsonify({"[ERROR]: Invalid configuration format"})
        
        return jsonify(config_Data) #Sends the client a JSON file that follows the YAML configuration 
    except FileNotFoundError:
        return jsonify({"[ERROR]: Configuration file, {used_file}, not found"}), 404



@app.route('/start_server', methods=['POST'])
def start_server():
    global server_process, config_File

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

    config_File = abs_path  # update global

    try:
        # Important: run led-wall-server in same directory as the executable
        exe_dir = os.path.abspath("../../server")
        server_process = subprocess.Popen(
            ["./led-wall-server", config_File],
            cwd=exe_dir
        )
        print(f"[INFO]: Server started with config: {config_File}")
        return jsonify({"status": "Server starting", "config_file": config_File}), 200
    except Exception as e:
        print(f"[ERROR]: Failed to start server -> {e}")
        return jsonify({"error": f"Server couldn't be started: {str(e)}"}), 500



@app.route('/stop_server', methods=['POST'])
def stop_server():
    global server_process
    if server_process is None:
        print("[ERROR]: Server not currently running")
        return jsonify({"error": "Server not currently running"}), 400

    try:
        os.kill(server_process.pid, signal.SIGTERM)
        server_process = None
        print("[INFO]: Server stopped successfully")
        return jsonify({"status": "Server stopped"})
    except Exception as e:
        print(f"[ERROR]: Server couldn't be stopped -> {e}")
        return jsonify({"error": "Server couldn't be stopped"}), 500
    

@app.route('/list_configs', methods=['GET'])
def list_configs():
    try:
        # List all .yaml files in CONFIG_DIR
        files = [f for f in os.listdir(CONFIG_DIR) if f.endswith(".yaml") and f.lower() != "matrix.yaml"]
        # Return full relative paths so frontend can send them to /start_server
        files_with_path = [os.path.join(CONFIG_DIR, f) for f in files]
                # Step 3: Debug print to see what is returned
        print(files_with_path)

        return jsonify({"configs": files_with_path})
    except Exception as e:
        print(f"[ERROR]: Failed to list config files -> {e}")
        return jsonify({"error": "Could not list config files"}), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)

