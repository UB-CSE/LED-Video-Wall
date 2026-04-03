import styles from "../Styles.module.css";
import React, { useState } from "react";
import { useDispatch, useSelector } from "react-redux";
import type { RootState } from "../state/store";
// Import reorderElements and the RtmpElem type
import { addElement, reorderElements, setSelectedElement } from "../state/config/configSlice.ts";import type { RtmpElem } from "../state/config/configSlice.ts";

type Props = {
  sizeMultiplier: number;
  setAddRtmpIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddRtmpPopup(props: Props) {
  const [name, setName] = useState("");
  const [streamName, setStreamName] = useState("");
  const [framerate, setFramerate] = useState(30);
  const [sizeW, setSizeW] = useState(0);
  const [sizeH, setSizeH] = useState(0);
  
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  function handleAdd() {
    const newId = configState.elements.length + 1;
    const size = sizeW > 0 || sizeH > 0 ? [sizeW, sizeH] : undefined;
    const newElement = {
      name: name || `rtmp${newId}`,
      id: 1,
      type: "rtmp" as const,
      location: [0, 0],
      stream_name: streamName,
      framerate,
      size,
      visible: true,
    };
    const shifted = configState.elements.map((el) => ({ ...el, id: el.id + 1 }));
    dispatch(reorderElements([newElement, ...shifted]));
    dispatch(setSelectedElement(1));
    props.setAddRtmpIsClicked(false);
  }

  return (
    <div className={styles.popup} onClick={(e) => e.stopPropagation()}>
      <ul style={{ padding: "0px", width: "80%" }}>
        <li key={1} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>name</p>
          <input className={styles.box} style={{ width: "75%" }}
            onChange={(e) => setName(e.target.value)} type="text" value={name} />
        </li>
        <li key={2} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>stream</p>
          <input className={styles.box} style={{ width: "75%" }}
            onChange={(e) => setStreamName(e.target.value)} type="text" value={streamName} />
        </li>
        <li key={3} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>framerate</p>
          <input className={styles.box} style={{ width: "75%" }}
            type="number" value={framerate} min={1}
            onChange={(e) => setFramerate(Math.max(1, e.target.valueAsNumber))} />
        </li>
        <li key={4} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>size</p>
          <div className={styles.box} style={{ width: "75%", display: "flex", padding: "3px" }}>
            <p>w:</p>
            <input type="number" value={sizeW} min={0}
              onChange={(e) => setSizeW(Math.max(0, e.target.valueAsNumber))}
              style={{ width: "30%", backgroundColor: "whitesmoke", margin: "auto" }} />
            <p>h:</p>
            <input type="number" value={sizeH} min={0}
              onChange={(e) => setSizeH(Math.max(0, e.target.valueAsNumber))}
              style={{ width: "30%", backgroundColor: "whitesmoke", margin: "auto" }} />
          </div>
        </li>
      </ul>
      <button onClick={handleAdd} style={{ left: "0%", transform: "translate(0%, 0%)" }}>Add</button>
    </div>
  );
}

export default AddRtmpPopup;