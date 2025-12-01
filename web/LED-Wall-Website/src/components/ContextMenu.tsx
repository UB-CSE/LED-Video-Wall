import styles from "../Styles.module.css";

type Option = {
  name: string;
  function: Function;
};

export type { Option };

type Props = {
  options: Option[];
  location: number[];
};

function ContextMenu(props: Props) {
  return (
    <div
      style={{
        position: "fixed",
        left: `${props.location[0]}px`,
        top: `${props.location[1]}px`,
        padding: "0",
      }}
    >
      <ul style={{ listStyle: "none", padding: "0" }}>
        {props.options.map((option) => (
          <li>
            <button
              className={styles.contextButton}
              onClick={(e) => option.function(e)}
            >
              {option.name}
            </button>
          </li>
        ))}
      </ul>
    </div>
  );
}
export default ContextMenu;
