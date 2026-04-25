import { useState, useEffect, useContext, useRef } from "react";
import styles from "./Styles.module.css";
import SaveButton from "./components/saveButton.tsx";
import { PortsContext } from "./PortContext";

type ButtonControlsProps = {
  getConfig: (arg0: number, arg1: number) => Promise<void>;
  sizeMultiplier: number;
};

function ButtonControls(props: ButtonControlsProps) {
  const [configFile, setConfigFile] = useState("--select configuration file--");
  const [configs, setConfigs] = useState<string[]>([]);
  const [message, setMessage] = useState("");
  const [running, setRunning] = useState("Server is not running");
  const [configRunning, setConfigRunning] = useState("");
  const [preOpen, setPreOpen] = useState<string | null>(null);

  const [localLedvwPort, setLocalLedvwPort] = useState<number>(7070);
  const [localRtmpPort, setLocalRtmpPort] = useState<number>(1935);

  const ports = useContext(PortsContext) as
    | {
        ledvwPort: number;
        setLedvwPort: (n: number) => void;
        rtmpPort: number;
        setRtmpPort: (n: number) => void;
        isValidPort?: (n: number) => boolean;
      }
    | undefined;

  const ledvwPort = ports?.ledvwPort ?? localLedvwPort;
  const setLedvwPort = ports?.setLedvwPort ?? setLocalLedvwPort;
  const rtmpPort = ports?.rtmpPort ?? localRtmpPort;
  const setRtmpPort = ports?.setRtmpPort ?? setLocalRtmpPort;
  const isValidPort = ports?.isValidPort ?? ((p: number) => Number.isInteger(p) && p >= 1024 && p <= 65535);

  const [runningServers, setRunningServers] = useState<string[]>([]);

  const showMessage = (msg: string) => {
    setMessage(msg);
    setTimeout(() => setMessage(""), 5000);
  };

  // Fetch available YAML configs from backend
  useEffect(() => {
    const fetchConfigs = async () => {
      try {
        const response = await fetch("/api/list-configs");
        const data = await response.json();
        if (data.configs) {
          setConfigs(data.configs);
        } else if (data.error) {
          showMessage(`[ERROR]: ${data.error}`);
        }
      } catch (error) {
        showMessage("[ERROR]: Could not fetch configs");
      }
    };
    fetchConfigs();
    getCurrentlyRunningAndMount();

  }, []);

  async function getRunningServers() {
    const response = await fetch("/api/list-running-servers", { method: "GET" });
    const text = await response.text();
    if (text == "") {
      setRunningServers([]);
    } else {
      setRunningServers(text.split(","));
    }
  }

  // Fetch currently running config from backend (accepts optional port param)
  async function getCurrentlyRunning(port?: number) {
    const p = port ?? ledvwPort;
    if (!isValidPort(p)) {
      setRunning("Server is not running");
      setConfigRunning("");
      return "";
    }

    try {
      const response = await fetch("/api/" + p + "/get-current-config", { method: "GET" });
      const text = await response.text();
      if (text == "") {
        setRunning("Server is not running");
        setConfigRunning(text);
      } else {
        setRunning("Server currently running");
        setConfigRunning(text);
      }
      getRunningServers();
      return text;
    } catch (err) {
      console.error("getCurrentlyRunning error:", err);
      setRunning("Server is not running");
      setConfigRunning("");
      getRunningServers();
      return "";
    }
  }

  // Fetch currently running config from backend and set as selected
  async function getCurrentlyRunningAndMount() {
    if (!isValidPort(ledvwPort)) {
      setRunning("Server is not running");
      setConfigRunning("");
      return;
    }

    try {
      const response = await fetch("/api/" + ledvwPort + "/get-current-config", { method: "GET" });
      const text = await response.text();
      if (text == "") {
        setRunning("Server is not running");
        setConfigRunning(text);
      } else {
        setRunning("Server currently running");
        setConfigRunning(text);
      }
      if (text != "") {
        await fetch("/api/" + ledvwPort + "/update-config", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ config_file: text }),
        });
        setConfigFile(text);
      }
    } catch (err) {
      console.error("getCurrentlyRunningAndMount error:", err);
      setRunning("Server is not running");
      setConfigRunning("");
    }

    getRunningServers();
  }

  const handleConfigChange = async (value: string, port?: number) => {
    const p = port ?? ledvwPort;

    if (!isValidPort(p)) return;

    const selected = value;
    setConfigFile(selected);

    if (!selected) return;

    try {
      // POST selected config file to backend
      const res = await fetch("/api/" + p + "/update-config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ config_file: selected }),
      });

      const data = await res.json();
      if (!res.ok) {
        showMessage(`[ERROR]: ${data.error || "Failed to select config"}`);
        return;
      }

      showMessage(`Loaded config: ${selected.split("/").pop()}`);

      // Reload the elements and redux state
      props.getConfig(props.sizeMultiplier, p);
    } catch (err) {
      console.error(err);
      showMessage("[ERROR]: Could not set config file");
    }
  };

  // Start the LED wall server with the selected config file
  const startServer = async () => {
    if (!configFile) {
      showMessage("[ERROR]: Please select a configuration file");
      return;
    }

    function validatePort(port: number, portName: string) {
      if (!Number.isInteger(port)) {
        showMessage("[ERROR]: " + portName + " port is not a valid integer");
        return false;
      }
      if (port < 0 || port > 65535) {
        showMessage("[ERROR]: " + portName + " port must be a valid port number (0-65535)");
        return false;
      }
      if (port < 1024) {
        showMessage("[ERROR]: " + portName + " port cannot be a well-known port (0-1023)");
        return false;
      }
      return true;
    }

    if (!validatePort(ledvwPort, "LEDVW") || !validatePort(rtmpPort, "RTMP")) {
      return;
    }

    if (ledvwPort === rtmpPort) {
      showMessage("[ERROR]: LEDVW port and RTMP port cannot be the same");
      return;
    }

    try {
      // POST request tells the backend to start the LED wall server with the selected config
      const response = await fetch("/api/" + ledvwPort + "/start-server", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          config_file: configFile,
          rtmp_port: rtmpPort,
        }),
      });
      const data = await response.json();

      if (data.error) showMessage(`[ERROR]: ${data.error}`);
      else if (data.status) showMessage(data.status);
    } catch (error) {
      showMessage("[ERROR]: Could not start server");
    }

    getCurrentlyRunning();
  };

  const stopServer = async () => {
    if (!isValidPort(ledvwPort)) {
      showMessage("[ERROR]: Invalid LEDVW port number, cannot determine which server to stop");
      return;
    }

    try {
      const response = await fetch("/api/" + ledvwPort + "/stop-server", { method: "POST" });
      const data = await response.json();

      if (data.error) showMessage(`[ERROR]: ${data.error}`);
      else if (data.status) showMessage(data.status);
    } catch (error) {
      showMessage("[ERROR]: Could not stop server");
    }

    getCurrentlyRunning();
  };

  const handlePortChange = (newPort: number) => {
    const text = getCurrentlyRunning(newPort);
    text.then((config) => {
      handleConfigChange(config, newPort);
    });
  }

  return (
    <div style={{ position: "fixed", left: "0px", top: "0px" }}>
      <div className={styles.panel} style={{ position: "relative", height: "500px" }}>
        <h2 className={styles.panelHeader}>Start/Stop Server</h2>

        <details style={{ width: "100%", display: "flex", justifyContent: "center", flexDirection: "column", alignItems: "center" }}>
          <summary style={{ textAlign: "center" }}><b>Port Numbers</b></summary>
          <div style={{ display: "flex", gap: "12px", marginBottom: "12px", alignItems: "center", justifyContent: "center" }}>
            <label style={{ display: "flex", flexDirection: "column", fontSize: "0.9rem", alignItems: "center" }}>
              LEDVW Port
              <input
                type="number"
                value={ledvwPort}
                onChange={(e) => {
                  const newPort = Number(e.target.value);
                  setLedvwPort(newPort);
                  handlePortChange(newPort);
                }}
                style={{ width: "110px", marginTop: "4px" }}
              />
            </label>

            <label style={{ display: "flex", flexDirection: "column", fontSize: "0.9rem", alignItems: "center" }}>
              RTMP Port
              <input
                type="number"
                value={rtmpPort}
                onChange={(e) => setRtmpPort(Number(e.target.value))}
                style={{ width: "110px", marginTop: "4px" }}
              />
            </label>
          </div>
        </details>

        <button onClick={startServer} style={{ left: "35%" }}>
          Start
        </button>
        <button onClick={stopServer} style={{ left: "40%" }}>
          Stop
        </button>
        <h3>Status:</h3>
        {message ? <p>{message}</p> : <p>{running}</p>}
        {runningServers.length > 0 && (
          <details style={{ width: "100%", display: "flex", justifyContent: "center", flexDirection: "column", alignItems: "center" }}>
            <summary><b>Running Servers:</b></summary>
            <ul>
              {runningServers.map((server) => (
                <li key={server}>{server}</li>
              ))}
            </ul>
          </details>
        )}
      </div>
      <div className={styles.panel} style={{ height: "400px" }}>
        <h2 className={styles.panelHeader}>Configuration Panel</h2>
        <h3>Select a Configuration File:</h3>
        {configRunning && (
          <div>
            <p>Live edit:</p>
            <button onClick={() => handleConfigChange(configRunning, ledvwPort)}>
              {configRunning.split("/").pop()}
            </button>
            <p>or</p>
          </div>
        )}
        <select
          value={configFile}
          onClick={(e) => {
            setPreOpen((e.currentTarget as HTMLSelectElement).value);
            console.log("MouseDown value:", e.currentTarget.value);
            if (
              e.currentTarget.value.split("/").pop() ===
              preOpen?.split("/").pop()
            ) {
              props.getConfig(props.sizeMultiplier, ledvwPort);
            }
          }}
          onChange={(e) => {
            const newConfig = e.target.value;
            if (newConfig === preOpen?.split("/").pop()) {
              props.getConfig(props.sizeMultiplier, ledvwPort);
            }
            handleConfigChange(newConfig, ledvwPort);
          }}
        >
          <option value={configFile}>{configFile.split("/").pop()}</option>
          {configs.map((cfg) => {
            if (cfg === configFile) return null;
            const fileName = cfg.split("/").pop() || cfg;
            return (
              <option key={cfg} value={cfg}>
                {fileName}
              </option>
            );
          })}
        </select>
        <SaveButton sizeMultiplier={props.sizeMultiplier}></SaveButton>
      </div>
    </div>
  );
}

export default ButtonControls;
