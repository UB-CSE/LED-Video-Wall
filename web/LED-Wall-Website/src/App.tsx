import styles from './Styles.module.css';
import Element from './components/element';
import {useEffect, type JSX} from 'react';

function App(){
  function get_image(filename: String){
    return 'http://127.0.0.1:5000/static/'+filename;
  }
  
  const elements: JSX.Element[] = []

  async function get_config(){
    try{
      const response = await fetch('/get_yaml_Config', {'method': 'GET'})
      const config = await response.json()
      for(const element of config['elements']){
        elements.push(
        <Element 
          key={element['id']} 
          type={element['type']} 
          path={get_image(element['filepath'])} 
          location={element['location']} 
          size={100} 
          />
        );
      }
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
    <div className={styles.canvas}>
      {elements}
    </div>
  );
}
export default App;