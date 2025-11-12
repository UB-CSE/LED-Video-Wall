import styles from "../Styles.module.css";

function DetailsPanel() {
  return (
    <div className={styles.panel} style={{ float: "right" }}>
      <h2 className={styles.panelHeader}>Details Panel</h2>
    </div>
  );
}
export default DetailsPanel;
