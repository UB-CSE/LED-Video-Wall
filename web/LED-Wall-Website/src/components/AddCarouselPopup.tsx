import styles from "../Styles.module.css";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { addElement, reorderElements, setSelectedElement } from "../state/config/configSlice.ts";
type Props = {
  sizeMultiplier: number;
  setAddCarouselIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddCarouselPopup(props: Props) {
  const [name, setName] = useState("");
  const [framerate, setFramerate] = useState(1);
  const dispatch = useDispatch();
  const configState = useSelector((state: RootState) => state.config);

  function handleAdd() {
    const newId = configState.elements.length + 1;
    const newElement = {
      name: name || `carousel${newId}`,
      id: 1,
      type: "carousel" as const,
      location: [0, 0],
      filepaths: [],
      framerate,
      visible: true,
    };
    const shifted = configState.elements.map((el) => ({ ...el, id: el.id + 1 }));
    dispatch(reorderElements([newElement, ...shifted]));
    dispatch(setSelectedElement(1));
    props.setAddCarouselIsClicked(false);
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

export default AddCarouselPopup;