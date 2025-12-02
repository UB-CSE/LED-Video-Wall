import { useEffect, useState } from "react";
import { useDispatch } from "react-redux";
import {
  setGamma,
  resetState,
  addElement,
} from "./state/config/configSlice.ts";
import Canvas from "./components/Canvas.tsx";
import Buttoncontrols from "./button-controls.tsx";
import DetailsPanel from "./components/DetailsPanel.tsx";
import ElementList from "./components/ElementList.tsx";

function App() {
  const dispatch = useDispatch();

  const [canvasDimensions, setCanvasDimensions] = useState([0, 0]);

  const [sizeMultiplier, setSizeMultiplier] = useState(0);

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
      for (const key in config["elements"]) {
        //Adds the element to the state
        if (config.elements[key].type === "image") {
          console.log(
            "elem" +
              String(config.elements[key].id) +
              " Location: " +
              String(config.elements[key].location)
          );
          dispatch(
            addElement({
              name: key,
              id: config.elements[key].id,
              type: config.elements[key].type,
              location: [
                config.elements[key].location[0] * multiplier,
                config.elements[key].location[1] * multiplier,
              ],
              filepath: config.elements[key].filepath,
              scale: config.elements[key].scale,
            })
          );
        } else if (config.elements[key].type === "text") {
          dispatch(
            addElement({
              name: key,
              id: config.elements[key].id,
              type: config.elements[key].type,
              location: config.elements[key].location,
              content: config.elements[key].content,
              size: config.elements[key].size,
              color: config.elements[key].color,
              font_path: config.elements[key].font_path,
            })
          );
          //Creates a JSX element and adds it to the list
          //newElements.push(

          //);
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
    } else {
      setSizeMultiplier(multiplierX);
      setCanvasDimensions([
        configWidth * multiplierX,
        configHeight * multiplierX,
      ]);
      getConfig(multiplierX);
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
