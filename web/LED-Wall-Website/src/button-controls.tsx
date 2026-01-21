import { useState, useEffect } from "react";
import styles from "./Styles.module.css";
import SaveButton from "./components/saveButton.tsx";

type ButtonControlsProps = {
  getConfig: (arg0: number) => Promise<void>;
  sizeMultiplier: number;
};

function ButtonControls(props: ButtonControlsProps) {
  const [configFile, setConfigFile] = useState("--select configuration file--");
  const [configs, setConfigs] = useState<string[]>([]);
  const [message, setMessage] = useState("");
  const [running, setRunning] = useState("Server is not running");
  const [configRunning, setConfigRunning] = useState("");
  const [preOpen, setPreOpen] = useState<string | null>(null);

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

  // Fetch currently running config from backend
  async function getCurrentlyRunning() {
    const response = await fetch("/api/get-current-config", { method: "GET" });
    const text = await response.text();
    if (text == "") {
      setRunning("Server is not running");
      setConfigRunning(text);
    } else {
      setRunning("Server currently running");
      setConfigRunning(text);
    }
  }

  // Fetch currently running config from backend and set as selected
  async function getCurrentlyRunningAndMount() {
    const response = await fetch("/api/get-current-config", { method: "GET" });
    const text = await response.text();
    if (text == "") {
      setRunning("Server is not running");
      setConfigRunning(text);
    } else {
      setRunning("Server currently running");
      setConfigRunning(text);
    }
    if (text != "") {
      await fetch("/api/update-config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ config_file: text }),
      });
      setConfigFile(text);
    }
  }

  const handleConfigChange = async (value: string) => {
    const selected = value;
    setConfigFile(selected);

    if (!selected) return;

    try {
      // POST selected config file to backend
      const res = await fetch("/api/update-config", {
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
      props.getConfig(props.sizeMultiplier);
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

    try {
      // POST request tells the backend to start the LED wall server with the selected config
      const response = await fetch("/api/start-server", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ config_file: configFile }),
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
    try {
      const response = await fetch("/api/stop-server", { method: "POST" });
      const data = await response.json();

      if (data.error) showMessage(`[ERROR]: ${data.error}`);
      else if (data.status) showMessage(data.status);
    } catch (error) {
      showMessage("[ERROR]: Could not stop server");
    }

    getCurrentlyRunning();
  };

  return (
    <div style={{ position: "fixed", left: "0px", top: "0px" }}>
      <div className={styles.panel}>
        <h2 className={styles.panelHeader}>Start/Stop Server</h2>
        <button onClick={startServer} style={{ left: "35%" }}>
          Start
        </button>
        <button onClick={stopServer} style={{ left: "40%" }}>
          Stop
        </button>
        <h3>Status:</h3>
        {message ? <p>{message}</p> : <p>{running}</p>}
      </div>
      <div className={styles.panel} style={{ height: "400px" }}>
        <h2 className={styles.panelHeader}>Configuration Panel</h2>
        <h3>Select a Configuration File:</h3>
        {configRunning && (
          <div>
            <p>Live edit:</p>
            <button onClick={() => handleConfigChange(configRunning)}>
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
              props.getConfig(props.sizeMultiplier);
            }
          }}
          onChange={(e) => {
            const newConfig = e.target.value;
            if (newConfig === preOpen?.split("/").pop()) {
              props.getConfig(props.sizeMultiplier);
            }
            handleConfigChange(newConfig);
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
