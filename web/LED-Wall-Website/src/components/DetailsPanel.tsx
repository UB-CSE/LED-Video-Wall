import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { type JSX, useEffect, useState } from "react";
import Element from "./element";
import { useDispatch } from "react-redux";
import { setSelectedElement, updateElement } from "../state/config/configSlice";

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
  const [scale, setScale] = useState(1);
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
          scale={scale}
        />
      );
      props.setElements(newElements);
      dispatch(
        updateElement({
          name: name,
          id: id,
          type: type,
          filepath: path,
          location: [
            location[0] * props.sizeMultiplier,
            location[1] * props.sizeMultiplier,
          ],
          scale: scale,
        })
      );
      //Send updated position to server
      fetch("/api/send-location", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          id: String(id),
          x: location[0],
          y: location[1],
        }),
      });
    }
  }

  async function handleLayerChange(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === "Enter") {
      const newElements = [];
      const oldElements = configState.elements;

      //Clamp layer value to 1-length of elements and if the layer is the same as current, return early
      if (layer < 1) {
        await setLayer(1);
        return;
      }
      if (layer === id) {
        return;
      }
      if (layer > oldElements.length) {
        await setLayer(oldElements.length);
        return;
      }

      let newLayer = 1;
      for (let oldLayer = 1; oldLayer <= oldElements.length; oldLayer++) {
        // When we reach the desired layer, insert the moved element
        if (newLayer === layer) {
          const movedElement = {
            name: name,
            id: newLayer,
            type: type,
            filepath: path,
            location: [
              location[0] * props.sizeMultiplier,
              location[1] * props.sizeMultiplier,
            ],
            scale: scale,
          };

          newElements.push(
            <Element
              key={newLayer}
              name={movedElement.name}
              id={newLayer}
              type={movedElement.type}
              path={movedElement.filepath}
              location={[movedElement.location[0], movedElement.location[1]]}
              sizeMultiplier={props.sizeMultiplier}
              scale={scale}
            />
          );
          dispatch(updateElement(movedElement));
          newLayer++;
        }
        if (oldLayer === id) {
          continue;
        }
        const element = { ...oldElements[oldLayer - 1] };
        element.id = newLayer;

        newElements.push(
          <Element
            key={element.id}
            name={element.name}
            id={element.id}
            type={element.type}
            path={element.filepath}
            location={[element.location[0], element.location[1]]}
            sizeMultiplier={props.sizeMultiplier}
            scale={element.scale}
          />
        );
        dispatch(updateElement(element));
        newLayer++;
      }
      if (newLayer === layer) {
        const movedElement = {
          name: name,
          id: newLayer,
          type: type,
          filepath: path,
          location: [
            location[0] * props.sizeMultiplier,
            location[1] * props.sizeMultiplier,
          ],
          scale: scale,
        };

        newElements.push(
          <Element
            key={newLayer}
            name={movedElement.name}
            id={newLayer}
            type={movedElement.type}
            path={movedElement.filepath}
            location={[movedElement.location[0], movedElement.location[1]]}
            sizeMultiplier={props.sizeMultiplier}
            scale={movedElement.scale}
          />
        );
        dispatch(updateElement(movedElement));
      }

      dispatch(setSelectedElement(layer));
      props.setElements(newElements);
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
      setLocation([
        Math.trunc(element.location[0] / props.sizeMultiplier),
        Math.trunc(element.location[1] / props.sizeMultiplier),
      ]);
      setPath(element.filepath);
      setScale(element.scale);
    } else {
      setType("");
    }
  }, [configState.selectedElement, configState.elements]);

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
        {type && (
          <li
            key={4}
            style={{
              display: "flex",
            }}
          >
            <p className={styles.box} style={{ width: "24.5%" }}>
              location
            </p>
            <div
              className={styles.box}
              style={{ width: "75.5%", display: "flex", padding: "3px" }}
            >
              <p>x:</p>
              <input
                onChange={(e) => {
                  setLocation([e.target.valueAsNumber, location[1]]);
                }}
                onKeyDown={(e) => handleChange(e)}
                type="number"
                value={location[0]}
                style={{
                  width: "20%",
                  backgroundColor: "whitesmoke",
                  margin: "auto",
                }}
              />
              <p>y:</p>
              <input
                onChange={(e) => {
                  setLocation([location[0], e.target.valueAsNumber]);
                }}
                onKeyDown={(e) => handleChange(e)}
                type="number"
                value={location[1]}
                style={{
                  width: "20%",
                  backgroundColor: "whitesmoke",
                  margin: "auto",
                }}
              />
            </div>
          </li>
        )}
        {type && (
          <li
            key={5}
            style={{
              display: "flex",
            }}
          >
            <p className={styles.box} style={{ width: "25%" }}>
              scale
            </p>
            <input
              className={styles.box}
              style={{ width: "75%" }}
              onChange={(e) => {
                setScale(e.target.valueAsNumber);
              }}
              onKeyDown={(e) => handleChange(e)}
              type="number"
              value={scale}
            />
          </li>
        )}
      </ul>
    </div>
  );
}
export default DetailsPanel;
