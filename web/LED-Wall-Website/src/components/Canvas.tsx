import React, { type JSX } from "react";
import styles from "../Styles.module.css";
import uploadFile from "./Upload.tsx";
import { useDispatch } from "react-redux";

type Props = {
  elements: JSX.Element[];
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>;
  canvasDimensions: number[];
  sizeMultiplier: number;
};

//Creates a canvas that can be uploaded to along with a upload button
function Canvas(props: Props) {
  const dispatch = useDispatch();

  //Sets the file and calls upload with the current mouse location
  async function handleDrop(e: React.DragEvent) {
    e.preventDefault();
    if (!e.dataTransfer.files || e.dataTransfer.files.length === 0) {
      return;
    }
    //Gets the canvas position
    const canvasRect = e.currentTarget.getBoundingClientRect();
    //Calculates position relative to canvas
    let relativeX = e.clientX - canvasRect.left;
    relativeX = Math.trunc(relativeX / props.sizeMultiplier);
    let relativeY = e.clientY - canvasRect.top;
    relativeY = Math.trunc(relativeY / props.sizeMultiplier);
    uploadFile(
      [relativeX, relativeY],
      e.dataTransfer.files[0],
      props.sizeMultiplier,
      props.elements,
      props.setElements,
      dispatch
    );
  }

  //Prevents browser not allowing dragging the image
  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  return (
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
  );
}
export default Canvas;
