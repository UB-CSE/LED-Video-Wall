import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { type JSX, useEffect, useState } from "react";
import Element from "./element";
import { useDispatch } from "react-redux";
import { setSelectedElement, updateElement } from "../state/config/configSlice";
import { current } from "@reduxjs/toolkit";

type Props = {
  elements: JSX.Element[];
  setElements: React.Dispatch<React.SetStateAction<JSX.Element[]>>;
  sizeMultiplier: number;
};

function DetailsPanel(props: Props) {
  const configState = useSelector((state: RootState) => state.config);
  const [type, setType] = useState("");
  const [name, setName] = useState("");
  const [id, setId] = useState(0);
  const [path, setPath] = useState("");
  const [layer, setLayer] = useState(0);
  const [location, setLocation] = useState<number[]>([0, 0]);
  //const [size, setSize] = useState([0, 0]);
  const dispatch = useDispatch();

  function handleChange(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === "Enter") {
      const newElements = [];
      for (const key in props.elements) {
        newElements.push(props.elements[key]);
      }
      newElements[configState.selectedElement - 1] = (
        <Element
          key={id}
          name={name}
          id={id}
          type={type}
          path={path}
          location={[
            location[0] * props.sizeMultiplier,
            location[1] * props.sizeMultiplier,
          ]}
          sizeMultiplier={props.sizeMultiplier}
        />
      );
      props.setElements(newElements);
      dispatch(
        updateElement({
          name: name,
          id: id,
          type: type,
          filepath: path,
          location: location,
        })
      );
    }
  }

  async function handleLayerChange(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === "Enter") {
      const newElements = [];
      const targetElement = configState.elements[layer - 1];
      for (let i = 0; i < props.elements.length; i++) {
        if (i == layer - 1) {
          newElements.push(
            <Element
              key={layer}
              name={name}
              id={layer}
              type={type}
              path={path}
              location={[
                location[0] * props.sizeMultiplier,
                location[1] * props.sizeMultiplier,
              ]}
              sizeMultiplier={props.sizeMultiplier}
            />
          );
        } else if (i == id - 1) {
          newElements.push(
            <Element
              key={id}
              name={targetElement.name}
              id={id}
              type={targetElement.type}
              path={targetElement.filepath}
              location={[
                targetElement.location[0] * props.sizeMultiplier,
                targetElement.location[1] * props.sizeMultiplier,
              ]}
              sizeMultiplier={props.sizeMultiplier}
            />
          );
        } else {
          const currentElement = configState.elements[i];
          newElements.push(
            <Element
              key={currentElement.id}
              name={currentElement.name}
              id={currentElement.id}
              type={currentElement.type}
              path={currentElement.filepath}
              location={[
                currentElement.location[0] * props.sizeMultiplier,
                currentElement.location[1] * props.sizeMultiplier,
              ]}
              sizeMultiplier={props.sizeMultiplier}
            />
          );
        }
      }
      await dispatch(
        updateElement({
          name: targetElement.name,
          id: id,
          type: targetElement.type,
          filepath: targetElement.filepath,
          location: targetElement.location,
        })
      );
      await dispatch(
        updateElement({
          name: name,
          id: layer,
          type: type,
          filepath: path,
          location: location,
        })
      );
      await dispatch(setSelectedElement(layer));
      await props.setElements(newElements);
      setId(layer);
    }
  }

  useEffect(() => {
    const element = configState.elements[configState.selectedElement - 1];
    if (element) {
      setType(element.type);
      setName(element.name);
      setId(element.id);
      setLayer(element.id);
      setLocation(element.location);
      setPath(element.filepath);
    }
  }, [configState.selectedElement]);

  return (
    <div className={styles.panel}>
      <h2
        className={styles.panelHeader}
        style={{ paddingRight: "0px", marginBottom: "0px" }}
      >
        Details Panel
      </h2>
      <ul
        style={{ paddingLeft: "0px", paddingRight: "60px", marginTop: "0px" }}
      >
        {type && (
          <li
            key={1}
            style={{
              display: "flex",
            }}
          >
            <p className={styles.box} style={{ width: "25%" }}>
              type
            </p>
            <p className={styles.box} style={{ width: "75%" }}>
              {type}
            </p>
          </li>
        )}
        {type && (
          <li
            key={2}
            style={{
              display: "flex",
            }}
          >
            <p className={styles.box} style={{ width: "25%" }}>
              name
            </p>
            <input
              className={styles.box}
              style={{ width: "75%" }}
              onChange={(e) => setName(e.target.value)}
              onKeyDown={(e) => handleChange(e)}
              type="text"
              value={name}
            />
          </li>
        )}
        {type && (
          <li
            key={3}
            style={{
              display: "flex",
            }}
          >
            <p className={styles.box} style={{ width: "25%" }}>
              layer
            </p>
            <input
              className={styles.box}
              style={{ width: "75%" }}
              onChange={(e) => {
                setLayer(e.target.valueAsNumber);
              }}
              onKeyDown={(e) => handleLayerChange(e)}
              type="number"
              value={layer}
            />
          </li>
        )}
      </ul>
    </div>
  );
}
export default DetailsPanel;
