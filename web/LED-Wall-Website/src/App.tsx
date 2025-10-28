import Element from "./components/element";
import { useEffect, type JSX, useRef, useState } from "react";
import { useDispatch } from "react-redux";
import { setGamma } from "./state/config/configSlice.ts";
import { addElement } from "./state/config/configSlice.ts";
import SaveButton from "./components/saveButton.tsx";
import FileUpload from "./components/FileUpload.tsx";
import Buttoncontrols from "./button-controls.tsx";

function App() {
  const dispatch = useDispatch();

  const hasRun = useRef(false);

  //List of elements to be displayed onscreen
  const [elements, setElements] = useState<JSX.Element[]>([]);

  const [canvasDimensions, setCanvasDimensions] = useState([0, 0]);

  const [sizeMultiplier, setSizeMultiplier] = useState(0);

  //Gets the current configuration file from the backend
  async function get_config() {
    //prevents double running during testing
    if (hasRun.current) {
      return;
    } else {
      hasRun.current = true;
    }
    try {
      //requests config from backend
      const response = await fetch("/api/get-yaml-config", { method: "GET" });
      const config = await response.json();
      const newElements = [];
      //Sets the gamma in the state
      dispatch(setGamma(config.settings.gamma));
      //Creates JSX elements and saves initial state
      for (const key in config["elements"]) {
        //Adds the element to the state
        dispatch(
          addElement({
            name: key,
            id: config.elements[key].id,
            type: config.elements[key].type,
            filepath: config.elements[key].filepath,
            location: config.elements[key].location,
          })
        );
        //Creates a JSX element and adds it to the list
        newElements.push(
          <Element
            key={config.elements[key].id}
            name={key}
            id={config.elements[key].id}
            type={config.elements[key].type}
            path={config.elements[key].filepath}
            location={config.elements[key].location}
          />
        );
      }
      setElements(newElements);
    } catch (error) {
      console.log("Get Config encountered an error: " + error);
      return;
    }
  }

  async function setCanvas() {
    //Setting starting width and height to define maximum dimensions for the canvas
    const maxWidth = window.innerWidth * 0.75;
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
    } else {
      setSizeMultiplier(multiplierX);
    }

    //Sets the canvas dimensions so that it will take up as much space as possible
    //without exceeding the maximum dimensions
    setCanvasDimensions([
      configWidth * sizeMultiplier,
      configHeight * sizeMultiplier,
    ]);
  }

  //Calls get_config when page loads
  useEffect(() => {
    get_config();
    setCanvas();
  }, []);
  //Displays button controls, save button,
  //and passes elements to the file upload where the canvas and elements will be
  return (
    <div>
      <Buttoncontrols />
      <SaveButton></SaveButton>
      <FileUpload
        elements={elements}
        setElements={setElements}
        canvasDimensions={canvasDimensions}
      ></FileUpload>
    </div>
  );
}
export default App;
