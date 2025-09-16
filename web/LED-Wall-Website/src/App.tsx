import styles from './Styles.module.css';
import Element from './components/element';

function App(){

  return(
    <div className={styles.canvas}>
      <Element url="https://media.tenor.com/cb9L14uH-YAAAAAM/cool-fun.gif" size={100}/>
      <Element url="https://imgs.search.brave.com/uUrbr6g_HS4YuAgC3JWy5KIRWZnSrn3spfDMZJHmLGY/rs:fit:860:0:0:0/g:ce/aHR0cHM6Ly9pLmt5/bS1jZG4uY29tL3Bo/b3Rvcy9pbWFnZXMv/bGlzdC8wMDMvMDk5/LzMzNi84NWMuanBn" size={100}/>
    </div>
  );
}
export default App;