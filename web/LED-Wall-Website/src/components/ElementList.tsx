import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";
import { useDispatch } from "react-redux";
import { setSelectedElement } from "../state/config/configSlice.ts";

function ElementList() {
  const configState = useSelector((state: RootState) => state.config);
  const dispatch = useDispatch();

  function handleClick(id: number) {
    dispatch(setSelectedElement(id));
  }

  return (
    <div className={styles.panel} style={{ height: "800px" }}>
      <h2 className={styles.panelHeader} style={{ paddingRight: "0px" }}>
        Element List
      </h2>
      <header style={{ display: "flex" }}>
        <h3>back</h3>
        <h3 style={{ marginLeft: "178px" }}>type</h3>
      </header>
      <ul style={{ paddingLeft: "0px", paddingRight: "60px" }}>
        {configState.elements.map((element) => (
          <li
            onClick={() => handleClick(element.id)}
            key={element.id}
            style={{
              display: "flex",
              border:
                configState.selectedElement == element.id
                  ? "3px solid cornflowerblue"
                  : "none",
            }}
          >
            <p className={styles.box} style={{ width: "15%" }}>
              {element.id}
            </p>
            <p className={styles.box} style={{ width: "55%" }}>
              {element.name}
            </p>
            <p className={styles.box} style={{ width: "30%" }}>
              {element.type}
            </p>
          </li>
        ))}
      </ul>
    </div>
  );
}
export default ElementList;
