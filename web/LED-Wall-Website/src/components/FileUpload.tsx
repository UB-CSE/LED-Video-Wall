import React, { useState, type ChangeEvent, type JSX } from "react";
import styles from "../Styles.module.css";
import { useDispatch } from "react-redux";
import { addElement } from "../state/config/configSlice.ts";
import Element from "./element";
import SaveButton from "./saveButton.tsx";

type Props = {
  elements: JSX.Element[];
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>;
  canvasDimensions: number[];
  sizeMultiplier: number;
};

//Creates a canvas that can be uploaded to along with a upload button
function FileUpload(props: Props) {
  const [newFile, setFile] = useState<File | null>(null);
  const [message, setMessage] = useState("");
  const dispatch = useDispatch();

  //Adds the new element to the state and sends the file to the web server
  async function uploadFile(
    location: number[],
    file: File | null = newFile,
    sizeMultiplier: number
  ) {
    //We require that the image file must be under 5MB
    if (file != null && file.size > 5000000) {
      setMessage("File must be under 5MB");
    } else if (file != null) {
      const formData = new FormData();
      formData.append("file", file);
      try {
        //Sends file to the server and receives a new filename from the server
        const res = await fetch("/api/upload-file", {
          method: "POST",
          body: formData,
        });
        const json = await res.json();
        const filename = json["filename"];
        //Adds new element using the filename to the rendered Elements
        const newElement = (
          <Element
            key={props.elements.length + 1}
            name={"elem" + String(props.elements.length + 1)}
            id={props.elements.length + 1}
            type={file.type.split("/")[0]}
            path={"images/" + filename}
            location={[
              location[0] * sizeMultiplier,
              location[1] * sizeMultiplier,
            ]}
            sizeMultiplier={sizeMultiplier}
          />
        );
        props.setElements([...props.elements, newElement]);
        //Adds new element using the filename to the redux config
        dispatch(
          addElement({
            name: "elem" + String(props.elements.length + 1),
            id: props.elements.length + 1,
            type: file.type.split("/")[0],
            filepath: "images/" + filename,
            location: location,
          })
        );
        setMessage("Success!");
      } catch (err) {
        setMessage("Server Error");
      }
    }
  }

  //Sets the file and calls upload with the current mouse location
  async function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    if (!e.dataTransfer.files || e.dataTransfer.files.length === 0) {
      return;
    }
    //Gets the canvas position
    const canvasRect = e.currentTarget.getBoundingClientRect();
    //Calculates position relative to canvas
    const relativeX = e.clientX - canvasRect.left;
    const relativeY = e.clientY - canvasRect.top;
    setFile(e.dataTransfer.files[0]);
    uploadFile(
      [relativeX, relativeY],
      e.dataTransfer.files[0],
      props.sizeMultiplier
    );
  }

  //Prevents browser not allowing dragging the image
  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  function handleChange(e: ChangeEvent<HTMLInputElement>) {
    if (e.target.files) {
      setFile(e.target.files[0]);
    }
  }
  /*
      <p>{message}</p>
      <input
        type="file"
        name="Add Element"
        accept="image/*"
        onChange={(e) => handleChange(e)}
      />
      <button onClick={() => uploadFile([0, 0], null, props.sizeMultiplier)}>
        Upload
      </button>
  */
  return (
    <div>
      <div
        className={styles.canvas}
        onDrop={(e) => handleDrop(e)}
        onDragOver={(e) => handleDragOver(e)}
        style={{
          cursor: "grab",
          width: props.canvasDimensions[0],
          height: props.canvasDimensions[1],
        }}
      >
        {props.elements}
      </div>
    </div>
  );
}
export default FileUpload;
