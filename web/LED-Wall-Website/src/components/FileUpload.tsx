import React, { useState, type ChangeEvent, type JSX } from "react";
import styles from "../Styles.module.css";
import { useDispatch } from "react-redux";
import { addElement } from "../state/config/configSlice.ts";
import Element from "./element";

type Props = {
  elements: JSX.Element[];
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>;
};

//Creates a canvas that can be uploaded to along with a upload button
function FileUpload(props: Props) {
  const [newFile, setFile] = useState<File | null>(null);
  const [message, setMessage] = useState("");
  const dispatch = useDispatch();

  //Adds the new element to the state and sends the file to the web server
  async function uploadFile(location: number[], file: File | null = newFile) {
    if (file != null && file.size > 5000000) {
      setMessage("File must be under 5MB");
    } else if (file != null) {
      const formData = new FormData();
      formData.append("file", file);
      try {
        const res = await fetch("/api/upload-file", {
          method: "POST",
          body: formData,
        });
        const json = await res.json();
        const filename = json["filename"];
        const newElement = (
          <Element
            key={props.elements.length + 1}
            name={"elem" + String(props.elements.length + 1)}
            id={props.elements.length + 1}
            type={file.type.split("/")[0]}
            path={"images/" + filename}
            location={[location[0], location[1]]}
            size={100}
          />
        );
        props.setElements([...props.elements, newElement]);
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
    uploadFile([relativeX, relativeY], e.dataTransfer.files[0]);
  }

  //Prevents browser not allowing dragging the image
  function handleDrag(e: React.DragEvent) {
    e.preventDefault();
  }

  function handleChange(e: ChangeEvent<HTMLInputElement>) {
    if (e.target.files) {
      setFile(e.target.files[0]);
    }
  }

  return (
    <div>
      <p>{message}</p>
      <input
        type="file"
        name="Add Element"
        accept="image/*"
        onChange={(e) => handleChange(e)}
      />
      <button onClick={() => uploadFile([0, 0])}>Upload</button>
      <div
        className={styles.canvas}
        onDrop={(e) => handleDrop(e)}
        onDrag={(e) => handleDrag(e)}
      >
        {props.elements}
      </div>
    </div>
  );
}
export default FileUpload;
