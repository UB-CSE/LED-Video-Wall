import { useEffect, useState } from "react";

function useContextMenu() {
  const [location, setLocation] = useState([0, 0]);
  const [isClicked, setIsClicked] = useState(false);

  function handleClick() {
    setIsClicked(false);
  }

  useEffect(() => {
    document.addEventListener("click", handleClick);
    return () => {
      document.removeEventListener("click", handleClick);
    };
  }, []);

  return {
    location,
    setLocation,
    isClicked,
    setIsClicked,
  };
}

export default useContextMenu;
