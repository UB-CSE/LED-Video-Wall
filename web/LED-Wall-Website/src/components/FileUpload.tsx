import React, { useState, type ChangeEvent, type JSX } from "react";
import styles from "../Styles.module.css";
import { useDispatch } from "react-redux";
import { addElement } from "../state/config/configSlice.ts";
import Element from "./element";

type Props = {
  elements: JSX.Element[];
};

//Creates a canvas that can be uploaded to along with a upload button
function FileUpload(props: Props) {
  const [newFile, setFile] = useState<File | null>(null);
  const [message, setMessage] = useState("");
  const dispatch = useDispatch();

  //Adds the new element to the state and sends the file to the web server
  async function uploadFile(location: number[]) {
    if (newFile != null && newFile.size > 5000000) {
      setMessage("File must be under 5MB");
    } else if (newFile != null) {
      dispatch(
        addElement({
          name: "elem" + String(props.elements.length + 1),
          id: props.elements.length + 1,
          type: newFile.type.split("/")[0],
          filepath: "images/" + newFile.name,
          location: location,
        })
      );

      const formData = new FormData();
      formData.append("file", newFile);
      try {
        await fetch("/api/upload-file", {
          method: "POST",
          body: formData,
        });
        props.elements.push(
          <Element
            key={props.elements.length + 1}
            name={"elem" + String(props.elements.length + 1)}
            id={props.elements.length + 1}
            type={newFile.type.split("/")[0]}
            path={"images/" + newFile.name}
            location={[location[0], location[1]]}
            size={100}
          />
        );
        setMessage("Success!");
      } catch (err) {
        setMessage("Server Error");
      }
    }
  }

  //Sets the file and calls upload with the current mouse location
  function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    setFile(e.dataTransfer.files[0]);
    uploadFile([e.clientX, e.clientY]);
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
