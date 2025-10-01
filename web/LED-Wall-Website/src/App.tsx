import styles from './Styles.module.css';
import Element from './components/element';
import Button_controls from './button_controls';
import {useEffect, type JSX, useRef, useState} from 'react';

function App(){
  const hasRun = useRef(false);

  function get_image(filename: String){
    return 'http://127.0.0.1:5000/static/'+filename;
  }
  
  const [elements, setElements] = useState<JSX.Element[]>([]);

  async function get_config(){
    if(hasRun.current){
      return;
    } else {
      hasRun.current = true;
    }
    try{
      const response = await fetch('/get_yaml_Config', {'method': 'GET'})
      const config = await response.json()
      const newElements = [];
      for(const key in config['elements']){
        newElements.push(
        <Element 
          key={config.elements[key].id}
          type={config.elements[key].type} 
          path={get_image(config.elements[key].filepath)} 
          location={config.elements[key].location} 
          size={100} 
          />
        );
      }
      setElements(newElements);
    } catch(error){
      console.log('Get Config encountered an error: ' + error)
      return
    }
  }

  const handleDrop = (e : DragEvent) => {
    e.preventDefault();
  };

  const handleDrag = (e : DragEvent) => {
    e.preventDefault();
    e.dataTransfer!.dropEffect = 'move';
  };

  useEffect(()=>{
    document.addEventListener('drop', handleDrop);
    document.addEventListener('dragover', handleDrag);
    document.addEventListener('dragenter', handleDrag);
    get_config()
    return () => {
      document.removeEventListener('drop', handleDrop);
      document.removeEventListener('dragover', handleDrag);
      document.removeEventListener('dragenter', handleDrag);
    }
  },[]);
  
  return(
    <div>
      <Button_controls/>

    <div className={styles.canvas}>
      {elements}
    </div>
    </div>
  );
}
export default App;