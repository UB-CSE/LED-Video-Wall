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
  const menuWidth = 100; // approximate width of menu
  const leftPos = props.location[0] - 300 
    ? props.location[0] - menuWidth 
    : props.location[0];

  return (
    <div
      style={{
        position: "fixed",
        left: `${leftPos}px`,
        top: `${props.location[1]}px`,
        padding: "0",
        zIndex: 999,
      }}
      onClick={(e) => e.stopPropagation()}
    >
      <ul style={{ listStyle: "none", padding: "0" }}>
        {props.options.map((option) => (
          <li key={option.name}>
            <button
              className={styles.contextButton}
              onClick={(e) => { e.stopPropagation(); option.function(e); }}
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
