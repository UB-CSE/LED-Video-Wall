import { useState } from "react";
import { useDispatch, useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { reorderElements, setSelectedElement } from "../state/config/configSlice.ts";
import styles from "../Styles.module.css";

type Props = {
  sizeMultiplier: number; // Keep if needed for scaling, though currently unused
  setAddWebcamIsClicked: React.Dispatch<React.SetStateAction<boolean>>;
};

function AddWebcamPopup({ setAddWebcamIsClicked }: Props) {
  const [name, setName] = useState("");
  const [cameraNumber, setCameraNumber] = useState(0);
  const [framerate, setFramerate] = useState(30);

  const dispatch = useDispatch();
  const elementsCount = useSelector((state: RootState) => state.config.elements.length);

  const configState = useSelector((state: RootState) => state.config);

  const handleAdd = () => {
    const newId = configState.elements.length + 1;
    const newElement = {
      name: name.trim() || `webcam${newId}`,
      id: 1,
      type: "webcam" as const,
      location: [0, 0],
      camera_number: cameraNumber || 0,
      framerate: framerate || 30,
      visible: true,
    };
    const shifted = configState.elements.map((el) => ({ ...el, id: el.id + 1 }));
    dispatch(reorderElements([newElement, ...shifted]));
    dispatch(setSelectedElement(1));
    setAddWebcamIsClicked(false);
  };

  return (
    <div className={styles.popup} onClick={(e) => e.stopPropagation()}>
      <ul style={{ padding: "0px", width: "80%", listStyle: "none" }}>
        <li style={{ display: "flex", marginBottom: "5px" }}>
          <span className={styles.box} style={{ width: "30%" }}>Name</span>
          <input 
            className={styles.box} 
            style={{ width: "70%" }}
            placeholder="Webcam Name"
            onChange={(e) => setName(e.target.value)} 
            type="text" 
            value={name} 
          />
        </li>
        <li style={{ display: "flex", marginBottom: "5px" }}>
          <span className={styles.box} style={{ width: "30%" }}>Camera #</span>
          <input 
            className={styles.box} 
            style={{ width: "70%" }}
            type="number" 
            value={cameraNumber} 
            min={0}
            onChange={(e) => setCameraNumber(e.target.valueAsNumber || 0)} 
          />
        </li>
        <li style={{ display: "flex" }}>
          <span className={styles.box} style={{ width: "30%" }}>Framerate</span>
          <input 
            className={styles.box} 
            style={{ width: "70%" }}
            type="number" 
            value={framerate} 
            min={1}
            onChange={(e) => setFramerate(e.target.valueAsNumber || 1)} 
          />
        </li>
      </ul>
      <button onClick={handleAdd} style={{ marginTop: "10px" }}>
        Add Webcam
      </button>
    </div>
  );
}

export default AddWebcamPopup;