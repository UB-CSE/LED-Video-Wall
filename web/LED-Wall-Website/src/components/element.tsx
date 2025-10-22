import { useState, useEffect } from "react";
import { useDispatch } from "react-redux";
import { updateElement } from "../state/config/configSlice.ts";

type ElementProps = {
  name: string;
  id: number;
  type: string;
  path: string;
  location: [number, number];
};
//Image Element that can be dragged and dropped inside the canvas
function Element(props: ElementProps) {
  const dispatch = useDispatch();

  //Store current position
  const [x, setX] = useState(0);
  const [y, setY] = useState(0);
  //Store position at start of dragging
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const [isDragging, setIsDragging] = useState(false);

  //Overwrites redux state of this element in the config
  function updateState() {
    dispatch(
      updateElement({
        name: props.name,
        id: props.id,
        type: props.type,
        filepath: props.path,
        location: [props.location[0] + x, props.location[1] + y],
      })
    );
  }

  function startDragging(e: React.MouseEvent) {
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
        x: props.location[0] + x,
        y: props.location[1] + y,
      }),
    });
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
      style={{
        position: "fixed",
        left: props.location[0] + x,
        top: props.location[1] + y,
        cursor: isDragging ? "grabbing" : "grab",
      }}
    />
  );
}
export default Element;
