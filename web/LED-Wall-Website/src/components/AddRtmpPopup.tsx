import styles from "../Styles.module.css";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { addElement, setSelectedElement } from "../state/config/configSlice.ts";

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
    const size = sizeW > 0 && sizeH > 0 ? [sizeW, sizeH] : undefined;
    dispatch(addElement({
      name: name || `rtmp${newId}`,
      id: newId,
      type: "rtmp",
      location: [0, 0],
      stream_name: streamName,
      framerate,
      size,
      visible: true,
    }));
    dispatch(setSelectedElement(newId));
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