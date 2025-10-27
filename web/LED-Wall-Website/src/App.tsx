import Element from "./components/element";
import { useEffect, type JSX, useState } from "react";
import { useDispatch } from "react-redux";
import { setGamma } from "./state/config/configSlice.ts";
import { addElement } from "./state/config/configSlice.ts";
import SaveButton from "./components/saveButton.tsx";
import FileUpload from "./components/FileUpload.tsx";
import Buttoncontrols from "./button-controls.tsx";

function App() {
  const dispatch = useDispatch();

  //List of elements to be displayed onscreen
  const [elements, setElements] = useState<JSX.Element[]>([]);

  //Gets the current configuration file from the backend
  async function getConfig() {
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
  //Calls get_config when page loads
  useEffect(() => {
    getConfig();
  }, []);
  //Displays button controls, save button,
  //and passes elements to the file upload where the canvas and elements will be
  return (
    <div>
      <Buttoncontrols getConfig={getConfig} />
      <SaveButton></SaveButton>
      <FileUpload elements={elements} setElements={setElements}></FileUpload>
    </div>
  );
}
export default App;
