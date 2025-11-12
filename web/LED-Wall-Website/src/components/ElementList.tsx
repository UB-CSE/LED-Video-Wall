import styles from "../Styles.module.css";
import { useSelector } from "react-redux";
import type { RootState } from "../state/store";

function ElementList() {
  const configState = useSelector((state: RootState) => state.config);
  return (
    <div className={styles.panel}>
      <h2 className={styles.panelHeader} style={{ paddingRight: "0px" }}>
        Element List
      </h2>
      <header style={{ display: "flex" }}>
        <h3>back</h3>
        <h3 style={{ marginLeft: "178px" }}>type</h3>
      </header>
      <ul style={{ paddingLeft: "0px", paddingRight: "60px" }}>
        {configState.elements.map((element) => (
          <li key={element.id} style={{ display: "flex" }}>
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
