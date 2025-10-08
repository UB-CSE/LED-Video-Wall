import Element from "./components/element";
import { useEffect, type JSX, useRef, useState } from "react";
import { useDispatch } from "react-redux";
import { setGamma } from "./state/config/configSlice.ts";
import { addElement } from "./state/config/configSlice.ts";
import SaveButton from "./components/saveButton.tsx";
import FileUpload from "./components/FileUpload.tsx";

function App() {
  //const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  const hasRun = useRef(false);

  //List of elements to be displayed onscreen
  const [elements, setElements] = useState<JSX.Element[]>([]);

  //Gets the current configuration file from the backend
  async function get_config() {
    //prevents double running during testing
    if (hasRun.current) {
      return;
    } else {
      hasRun.current = true;
    }
    try {
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
            size={100}
          />
        );
      }
      setElements(newElements);
    } catch (error) {
      console.log("Get Config encountered an error: " + error);
      return;
    }
  }
  //Handles cursor image
  const handleDrop = (e: DragEvent) => {
    e.preventDefault();
  };
  //Handles cursor image
  const handleDrag = (e: DragEvent) => {
    e.preventDefault();
    e.dataTransfer!.dropEffect = "move";
  };
  //Adds and removes event listeners and calls get_config
  useEffect(() => {
    document.addEventListener("drop", handleDrop);
    document.addEventListener("dragover", handleDrag);
    document.addEventListener("dragenter", handleDrag);
    get_config();
    return () => {
      document.removeEventListener("drop", handleDrop);
      document.removeEventListener("dragover", handleDrag);
      document.removeEventListener("dragenter", handleDrag);
    };
  }, []);
  //Displays all elements and the canvas
  return (
    <div>
      <SaveButton></SaveButton>
      <FileUpload elements={elements} setElements={setElements}></FileUpload>
    </div>
  );
}
export default App;
