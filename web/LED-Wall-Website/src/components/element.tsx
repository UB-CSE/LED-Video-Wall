import type React from "react";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { updateElement } from "../state/config/configSlice.ts";

type UrlProps = {
  name: string;
  id: number;
  type: string;
  path: string;
  location: [number, number];
  size: number;
};
function Element(props: UrlProps) {
  const dispatch = useDispatch();

  const [x, setx] = useState(0);
  const [y, sety] = useState(0);
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const element_id = props.id;
  //const [count, setCount] = useState(0);

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

  function handleDrag(e: React.DragEvent) {
    setx(e.clientX - startX);
    sety(e.clientY - startY);
    /*if (count >= 200) {
      sendPosition();
      setCount(0);
    } else {
      setCount(count + 1);
    }*/
  }
  function handleDragStart(e: React.DragEvent) {
    e.dataTransfer.setDragImage(e.currentTarget, -1000, -1000);
    setStartX(e.clientX - x);
    setStartY(e.clientY - y);
  }
  function handleDragEnd(e: React.DragEvent) {
    var newX = e.clientX - startX;
    var newY = e.clientY - startY;
    setx(newX);
    sety(newY);
    sendPosition();
    updateState();
  }
  async function sendPosition() {
    fetch("/api/send-location", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        id: String(element_id),
        x: props.location[0] + x,
        y: props.location[1] + y,
      }),
    });
  }
  return (
    <div
      className="box"
      draggable
      onDrag={(e) => handleDrag(e)}
      onDragStart={(e) => handleDragStart(e)}
      onDragEnd={(e) => handleDragEnd(e)}
      style={{
        position: "fixed",
        left: props.location[0] + x,
        top: props.location[1] + y,
        width: `200px`,
        height: `200px`,
      }}
    >
      <img
        src={"static/" + props.path}
        draggable={false}
        style={{ width: `${props.size}%`, height: `${props.size}%` }}
      />
    </div>
  );
}
export default Element;
