import React, { useRef, useState, type JSX } from "react";
import styles from "./Styles.module.css";
import Element from "../components/element";
import { useDispatch } from "react-redux";
import { addElement } from "../state/config/configSlice.ts";

type Props = {
  elements: JSX.Element[];
};

function FileUpload(props: Props) {
  const upload = useRef(null);
  const [newFile, setFile] = useState<File>();
  const [message, setMessage] = useState("");
  const dispatch = useDispatch();

  function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    setFile(e.dataTransfer.files[0]);
    if (newFile == null || newFile.size > 5000000) {
      setMessage("File must be under 5MB");
    } else {
      setMessage("Success!");
      dispatch(
        addElement({
          name: "elem" + String(props.elements.length + 1),
          id: props.elements.length + 1,
          type: "image",
          filepath: "images/" + newFile.name,
          location: [e.clientX, e.clientY],
        })
      );
    }
  }

  function handleDrag(e: React.DragEvent) {
    e.preventDefault();
  }

  return (
    <div
      className={styles.canvas}
      onDrop={(e) => handleDrop(e)}
      onDrag={(e) => handleDrag(e)}
    >
      <p>{message}</p>
      <input ref={upload} type="file" />
      {props.elements}
    </div>
  );
}
export default FileUpload;
