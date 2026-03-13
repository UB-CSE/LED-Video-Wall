import styles from "../Styles.module.css";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { addElement, setSelectedElement } from "../state/config/configSlice.ts";

type Props = {
  sizeMultiplier: number;
  setAddVideoIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddVideoPopup(props: Props) {
  const [name, setName] = useState("");
  const [filepath, setFilepath] = useState("");
  const [framerate, setFramerate] = useState(30);
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  function handleAdd() {
    const newId = configState.elements.length + 1;
    dispatch(addElement({
      name: name || `video${newId}`,
      id: newId,
      type: "video",
      location: [0, 0],
      filepath,
      framerate,
      visible: true,
    }));
    dispatch(setSelectedElement(newId));
    props.setAddVideoIsClicked(false);
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
          <p className={styles.box} style={{ width: "25%" }}>filepath</p>
          <input className={styles.box} style={{ width: "75%" }}
            onChange={(e) => setFilepath(e.target.value)} type="text" value={filepath} />
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

export default AddVideoPopup;