import React from "react";
import styles from "../Styles.module.css";
import uploadFile from "./Upload.tsx";
import { useDispatch, useSelector } from "react-redux";
import type { RootState } from "../state/store";
import type { Elem } from "../state/config/configSlice.ts";
import Element from "./element.tsx";

type Props = {
  canvasDimensions: number[];
  sizeMultiplier: number;
};

//Canvas that holds all elements and allows drag and drop of image files onto it
function Canvas(props: Props) {
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

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
      dispatch,
      configState
    );
  }

  //Prevents browser not allowing dragging the image
  function handleDragOver(e: React.DragEvent) {
    e.preventDefault();
  }

  function createJSXElement(element: Elem) {
    if (element.type === "image") {
      return (
        <Element
          key={element.id}
          name={element.name}
          id={element.id}
          type={element.type}
          path={element.filepath}
          location={[element.location[0], element.location[1]]}
          sizeMultiplier={props.sizeMultiplier}
          scale={element.scale}
        />
      );
    } else if (element.type === "text") {
      return (
        <Element
          key={element.id}
          name={element.name}
          id={element.id}
          type={element.type}
          content={element.content}
          size={element.size}
          color={element.color}
          font_path={element.font_path}
          location={[element.location[0], element.location[1]]}
          sizeMultiplier={props.sizeMultiplier}
        />
      );
    }
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
      {configState.elements.map(createJSXElement)}
    </div>
  );
}
export default Canvas;
