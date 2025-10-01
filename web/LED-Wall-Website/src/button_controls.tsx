import React, { useState } from "react";

const button_controls: React.FC = () => {
  const [configFile, setConfigFile] = useState("");
  const [message, setMessage] = useState("");

  const startServer = async () => {
    try {
      const response = await fetch("/start_server", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ config_file: configFile }),
      });
      const data = await response.json();
      setMessage(data.message);
    } catch (error) {
      setMessage("[ERROR]: Could not start server");
    }
  };

  const stopServer = async () => {
    try {
      const response = await fetch("/stop_server", { method: "POST" });
      const data = await response.json();
      setMessage(data.message);
    } catch (error) {
      setMessage("[ERROR]: Could not stop server");
    }
  };

  return (
    <div style={{ padding: "20px" }}>
      <h2>LED Video Wall Controls</h2>
      <div>
        <select
            value={configFile}
            onChange={(e) => setConfigFile(e.target.value)}
            style={{ marginRight: "10px" }}>
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

export default button_controls;