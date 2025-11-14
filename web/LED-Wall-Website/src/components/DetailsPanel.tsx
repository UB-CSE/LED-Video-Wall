import styles from "../Styles.module.css";

function DetailsPanel() {
  return (
    <div className={styles.panel}>
      <h2 className={styles.panelHeader} style={{ paddingRight: "0px" }}>
        Details Panel
      </h2>
    </div>
  );
}
export default DetailsPanel;
