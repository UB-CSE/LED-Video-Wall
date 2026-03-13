import styles from "../Styles.module.css";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { addElement, setSelectedElement } from "../state/config/configSlice.ts";

type Props = {
  sizeMultiplier: number;
  setAddWebcamIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddWebcamPopup(props: Props) {
  const [name, setName] = useState("");
  const [cameraNumber, setCameraNumber] = useState(0);
  const [framerate, setFramerate] = useState(30);
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  function handleAdd() {
    const newId = configState.elements.length + 1;
    dispatch(addElement({
      name: name || `webcam${newId}`,
      id: newId,
      type: "webcam",
      location: [0, 0],
      camera_number: cameraNumber,
      framerate,
      visible: true,
    }));
    dispatch(setSelectedElement(newId));
    props.setAddWebcamIsClicked(false);
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
          <p className={styles.box} style={{ width: "25%" }}>camera #</p>
          <input className={styles.box} style={{ width: "75%" }}
            type="number" value={cameraNumber} min={0}
            onChange={(e) => setCameraNumber(Math.max(0, e.target.valueAsNumber))} />
        </li>
        <li key={3} style={{ display: "flex" }}>
          <p className={styles.box} style={{ width: "25%" }}>framerate</p>
          <input className={styles.box} style={{ width: "75%" }}
            type="number" value={framerate} min={1}
            onChange={(e) => setFramerate(Math.max(1, e.target.valueAsNumber))} />
        </li>
      </ul>
      <button onClick={handleAdd} style={{ left: "0%", transform: "translate(0%, 0%)" }}>Add</button>
    </div>
  );
}

export default AddWebcamPopup;