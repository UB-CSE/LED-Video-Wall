import { useEffect, useState } from "react";
import { useDispatch, useSelector } from "react-redux";
import {
  setGamma,
  resetState,
  addElement,
} from "./state/config/configSlice.ts";
import Canvas from "./components/Canvas.tsx";
import Buttoncontrols from "./button-controls.tsx";
import DetailsPanel from "./components/DetailsPanel.tsx";
import ElementList from "./components/ElementList.tsx";
import type { RootState } from "./state/store.ts";

function App() {
  const dispatch = useDispatch();

  const [canvasDimensions, setCanvasDimensions] = useState([0, 0]);

  const [sizeMultiplier, setSizeMultiplier] = useState(0);

  const configState = useSelector((state: RootState) => state.config);

  //Gets the current configuration file from the backend
  async function getConfig(multiplier: number) {
    try {
      //requests config from backend
      const response = await fetch("/api/get-yaml-config", { method: "GET" });
      const config = await response.json();
      dispatch(resetState(config));
      //Sets the gamma in the state
      dispatch(setGamma(config.settings.gamma));
      //Creates JSX elements and saves initial state
      const elements = [];
      for (const key in config["elements"]) {
        elements.push({ key, ...config.elements[key] });
      }
      elements.sort((a, b) => a.id - b.id);
      for (const element of elements) {
        //Adds the element to the state
        if (element.type === "image") {
          dispatch(
            addElement({
              name: element.key,
              id: element.id,
              type: element.type,
              location: [
                element.location[0] * multiplier,
                element.location[1] * multiplier,
              ],
              filepath: element.filepath,
              scale: element.scale,
            })
          );
        } else if (element.type === "text") {
          dispatch(
            addElement({
              name: element.key,
              id: element.id,
              type: element.type,
              location: [
                element.location[0] * multiplier,
                element.location[1] * multiplier,
              ],
              content: element.content,
              size: element.size,
              color: element.color,
              font_path: element.font_path,
            })
          );
        }
      }
    } catch (error) {
      console.log("Get Config encountered an error: " + error);
      return;
    }
  }

  async function setCanvas() {
    //Setting starting width and height to define maximum dimensions for the canvas
    const maxWidth = window.innerWidth * 0.5;
    const maxHeight = window.innerHeight * 0.9;

    //Fetch LED wall config
    const response = await fetch("/api/get-matrix-config", { method: "GET" });
    const config = await response.json();
    let minX;
    let minY;
    let maxX;
    let maxY;

    //Copies the matrix specifications into a map for easy access
    const specs = new Map();
    for (const key in config["matrix-specs"]) {
      specs.set(key, config["matrix-specs"][key]["width-height"]);
    }

    //Populate mins and maxs using the matrix positions and width-heights
    for (const key in config["matrices"]) {
      const startingX = config["matrices"][key]["pos"][0];
      const startingY = config["matrices"][key]["pos"][1];
      let x: number;
      let y: number;
      const rot = config["matrices"][key]["rot"];
      const spec = config["matrices"][key]["spec"];
      const widthHeight: number[] = specs.get(spec);

      if (rot == "down") {
        x = startingX + widthHeight[0];
        y = startingY + widthHeight[1];
      } else if (rot == "up") {
        x = startingX - widthHeight[0];
        y = startingY - widthHeight[1];
      } else if (rot == "right") {
        x = startingX + widthHeight[1];
        y = startingY - widthHeight[0];
      } else if (rot == "left") {
        x = startingX - widthHeight[1];
        y = startingY + widthHeight[0];
      } else {
        return;
      }

      if (typeof minX == "undefined") {
        minX = Math.min(x, startingX);
      } else {
        minX = Math.min(x, startingX, minX);
      }
      if (typeof maxX == "undefined") {
        maxX = Math.max(x, startingX);
      } else {
        maxX = Math.max(x, startingX, maxX);
      }
      if (typeof minY == "undefined") {
        minY = Math.min(y, startingY);
      } else {
        minY = Math.min(y, startingY, minY);
      }
      if (typeof maxY == "undefined") {
        maxY = Math.max(y, startingY);
      } else {
        maxY = Math.max(y, startingY, maxY);
      }
    }
    if (
      typeof minX == "undefined" ||
      typeof maxX == "undefined" ||
      typeof minY == "undefined" ||
      typeof maxY == "undefined"
    ) {
      return;
    }

    //Calculates LED-Wall pixel width and height
    const configWidth = maxX - minX;
    const configHeight = maxY - minY;

    //Finds the ratio between the allotted pixels on screen versus the config
    const multiplierX = maxWidth / configWidth;
    const multiplierY = maxHeight / configHeight;

    //The dimension that takes up the largest portion of the allotted space is chosen
    if (multiplierX > multiplierY) {
      setSizeMultiplier(multiplierY);
      //Sets the canvas dimensions so that it will take up as much space as possible
      //without exceeding the maximum dimensions
      setCanvasDimensions([
        configWidth * multiplierY,
        configHeight * multiplierY,
      ]);
      getConfig(multiplierY);
      console.log(configState.elements.length);
    } else {
      setSizeMultiplier(multiplierX);
      setCanvasDimensions([
        configWidth * multiplierX,
        configHeight * multiplierX,
      ]);
      getConfig(multiplierX);
      console.log(configState.elements.length);
    }
  }

  //Calls get_config when page loads
  useEffect(() => {
    setCanvas();
  }, []);
  //Displays button controls, save button,
  //and passes elements to the file upload where the canvas and elements will be
  return (
    <div>
      <Canvas
        canvasDimensions={canvasDimensions}
        sizeMultiplier={sizeMultiplier}
      ></Canvas>
      <h1>LED Video Wall Controls</h1>
      <Buttoncontrols getConfig={getConfig} sizeMultiplier={sizeMultiplier} />
      <div style={{ position: "fixed", right: "0%", top: "0%" }}>
        <DetailsPanel sizeMultiplier={sizeMultiplier}></DetailsPanel>
        <ElementList sizeMultiplier={sizeMultiplier}></ElementList>
      </div>
    </div>
  );
}
export default App;
