import React, { useState, useEffect } from "react";
import { useDispatch } from "react-redux";
import {
  setSelectedElement,
  updateElement,
} from "../state/config/configSlice.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";

type ElementProps = {
  name: string;
  id: number;
  type: string;
  path: string;
  location: [number, number];
  sizeMultiplier: number;
  scale: number;
};
//Image Element that can be dragged and dropped inside the canvas
function Element(props: ElementProps) {
  //Redux State
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  //Store current position
  const [x, setX] = useState(0);
  const [y, setY] = useState(0);
  //Store position at start of dragging
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const [isDragging, setIsDragging] = useState(false);
  //Store current dimensions
  const [dimensions, setDimensions] = useState([0, 0]);

  //Overwrites redux state of this element in the config
  function updateState() {
    dispatch(
      updateElement({
        name: props.name,
        id: props.id,
        type: props.type,
        filepath: props.path,
        location: [props.location[0] + x, props.location[1] + y],
        scale: props.scale,
      })
    );
  }

  function startDragging(e: React.MouseEvent) {
    dispatch(setSelectedElement(props.id));
    setIsDragging(true);
    setStartX(e.clientX - x);
    setStartY(e.clientY - y);
  }

  //Sends the current location of the element to the server
  function sendPosition() {
    fetch("/api/send-location", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        id: String(props.id),
        x: Math.trunc((props.location[0] + x) / props.sizeMultiplier),
        y: Math.trunc((props.location[1] + y) / props.sizeMultiplier),
      }),
    });
  }

  //Finds the size of the image and sets the new size with sizeMultiplier
  function handleLoad(e: React.SyntheticEvent<HTMLImageElement, Event>) {
    const { naturalHeight, naturalWidth } = e.currentTarget;
    setDimensions([
      naturalWidth * props.sizeMultiplier,
      naturalHeight * props.sizeMultiplier,
    ]);
  }

  //Had to change DragEvent to MouseEvent in order to have control over cursor style
  //Not using react's built in drag event required useEffect and event listeners,
  //because otherwise, the drag would stop if the cursor outpaced the image
  //Binding to the document solves that issue
  useEffect(() => {
    if (isDragging) {
      function handleDrag(e: MouseEvent) {
        setX(e.clientX - startX);
        setY(e.clientY - startY);
      }
      function stopDragging() {
        setIsDragging(false);
      }
      document.addEventListener("mousemove", handleDrag);
      document.addEventListener("mouseup", stopDragging);
      return () => {
        document.removeEventListener("mousemove", handleDrag);
        document.removeEventListener("mouseup", stopDragging);
      };
    } else {
      updateState();
      sendPosition();
    }
  }, [isDragging]);
  return (
    <img
      src={"/api/" + props.path}
      draggable={false}
      onMouseDown={(e) => startDragging(e)}
      onLoad={handleLoad}
      style={{
        position: "fixed",
        left: props.location[0] + x,
        top: props.location[1] + y,
        cursor: isDragging ? "grabbing" : "grab",
        width: dimensions[0] * props.scale,
        height: dimensions[1] * props.scale,
        border:
          configState.selectedElement == props.id
            ? "3px solid cornflowerblue"
            : "none",
      }}
    />
  );
}
export default Element;
