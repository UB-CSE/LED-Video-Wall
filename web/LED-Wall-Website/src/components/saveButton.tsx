import { useState } from "react";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";

type Props = {
  sizeMultiplier: number;
};

function SaveButton(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const [message, setMessage] = useState("");

  //Sends the current configuration in state to the web server
  function sendToServer() {
    //Creates JSON object and sets gamma
    const elements: Record<string, any> = {};
    for (let i = 0; i < configState.elements.length; i++) {
      let element = configState.elements[i];
      if (element.type === "image") {
        elements[element.name] = {
          id: element.id,
          type: element.type,
          filepath: element.filepath,
          location: [
            Math.trunc(element.location[0] / props.sizeMultiplier),
            Math.trunc(element.location[1] / props.sizeMultiplier),
          ],
          scale: element.scale,
        };
      } else if (element.type === "text") {
        elements[element.name] = {
          id: element.id,
          type: element.type,
          content: element.content,
          size: element.size,
          color: element.color,
          font_path: element.font_path,
          location: [
            Math.trunc(element.location[0] / props.sizeMultiplier),
            Math.trunc(element.location[1] / props.sizeMultiplier),
          ],
        };
        console.log("font_path: " + element.font_path);
      }
    }
    //Sends JSON to web server
    fetch("/api/set-yaml-config", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        settings: { gamma: configState.settings.gamma },
        elements: elements,
      }),
    })
      .then((res) => {
        if (res.status == 200) {
          return res.text();
        } else {
          return "[ERROR]: Server communication failure";
        }
      })
      .then((m) => {
        setMessage(m);
      });
  }
  //Button JSX with a description of the button functionality
  return (
    <div>
      <h3>Save Configuration File:</h3>
      <button onClick={() => sendToServer()}>Overwrite current file</button>
      <p>{message}</p>
    </div>
  );
}
export default SaveButton;
