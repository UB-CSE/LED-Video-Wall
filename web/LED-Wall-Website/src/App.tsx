import styles from './Styles.module.css';
import Element from './components/element';

function App(){
  function get_image(filename: String){
    return 'http://127.0.0.1:5000/static/image/'+filename;
  }

  return(
    <div className={styles.canvas}>
      <Element url="https://media.tenor.com/cb9L14uH-YAAAAAM/cool-fun.gif" size={100}/>
      <Element url={get_image('img.jpg')} size={100}/>
    </div>
  );
}
export default App;