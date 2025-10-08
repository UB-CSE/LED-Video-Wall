import React, { useState, useEffect } from "react";

const ButtonControls: React.FC = () => {
  const [configFile, setConfigFile] = useState("");
  const [configs, setConfigs] = useState<string[]>([]);
  const [message, setMessage] = useState("");

  const showMessage = (msg: string) => {
    setMessage(msg);
    setTimeout(() => setMessage(""), 2000);
  };

  // Fetch available YAML configs from backend
  useEffect(() => {
    const fetchConfigs = async () => {
      try {
        const response = await fetch("/list_configs");
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
  }, []);

  const startServer = async () => {
    if (!configFile) {
      showMessage("[ERROR]: Please select a configuration file");
      return;
    }

    try {
      const response = await fetch("/start_server", {
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
  };

  const stopServer = async () => {
    try {
      const response = await fetch("/stop_server", { method: "POST" });
      const data = await response.json();

      if (data.error) showMessage(`[ERROR]: ${data.error}`);
      else if (data.status) showMessage(data.status);
    } catch (error) {
      showMessage("[ERROR]: Could not stop server");
    }
  };

  return (
    <div style={{ padding: "20px" }}>
      <h2>LED Video Wall Controls</h2>
      <div>
        <select
          value={configFile}
          onChange={(e) => setConfigFile(e.target.value)}
          style={{ marginRight: "10px" }}
        >
          <option value="">-- Select a Configuration --</option>
          {configs.map((cfg) => {
            const fileName = cfg.split("/").pop() || cfg;
            return (
              <option key={cfg} value={cfg}>
                {fileName}
              </option>
            );
          })}
        </select>
        <button onClick={startServer}>Start</button>
        <button onClick={stopServer} style={{ marginLeft: "10px" }}>
          Stop
        </button>
      </div>
      {message && <p>{message}</p>}
    </div>
  );
};

export default ButtonControls;


/*
import React, { useState } from "react";

const ButtonControls: React.FC = () => {
  const [configFile, setConfigFile] = useState("");
  const [message, setMessage] = useState("");

  const showMessage = (msg: string) => {
    setMessage(msg);
    // Clear message after 2 seconds
    setTimeout(() => setMessage(""), 2000);
  };

  const startServer = async () => {
    try {
      const response = await fetch("/start_server", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ config_file: configFile }),
      });
      const data = await response.json();
      
      if (data.error) {
        showMessage(`[ERROR]: ${data.error}`);
      } else if (data.status) {
        showMessage(data.status);
      }
    } catch (error) {
      showMessage("[ERROR]: Could not start server");
    }
  };

  const stopServer = async () => {
    try {
      const response = await fetch("/stop_server", { method: "POST" });
      const data = await response.json();

      if (data.error) {
        showMessage(`[ERROR]: ${data.error}`);
      } else if (data.status) {
        showMessage(data.status);
      }
    } catch (error) {
      showMessage("[ERROR]: Could not stop server");
    }
  };

  return (
    <div style={{ padding: "20px" }}>
      <h2>LED Video Wall Controls</h2>
      <div>
        <select
          value={configFile}
          onChange={(e) => setConfigFile(e.target.value)}
          style={{ marginRight: "10px" }}
        >
          <option value="">-- Select an Image --</option>
          <option value="../../server/input-conway.yaml">Conways</option>
          <option value="../../server/input-rainbow.yaml">Rainbow</option>
          <option value="../../server/input-alan-face.yaml">Alan</option>
        </select>
        <button onClick={startServer}>Start</button>
        <button onClick={stopServer} style={{ marginLeft: "10px" }}>
          Stop
        </button>
      </div>
      {message && <p>{message}</p>}
    </div>
  );
};

export default ButtonControls;
*/