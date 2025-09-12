import styles from './Styles.module.css';
import Element from './components/element';

function App(){

  return(
    <div className={styles.canvas}>
      <Element url="https://imgs.search.brave.com/vzVb420qkaR-r4_3HMcbvH5eNniSvO1Ieq69YJzhYxY/rs:fit:500:0:1:0/g:ce/aHR0cHM6Ly9pLmlt/Z2ZsaXAuY29tLzQv/M3UwNGg1LmpwZw" size={300}/>
      <Element url="https://imgs.search.brave.com/uUrbr6g_HS4YuAgC3JWy5KIRWZnSrn3spfDMZJHmLGY/rs:fit:860:0:0:0/g:ce/aHR0cHM6Ly9pLmt5/bS1jZG4uY29tL3Bo/b3Rvcy9pbWFnZXMv/bGlzdC8wMDMvMDk5/LzMzNi84NWMuanBn" size={300}/>
    </div>
  );
}
export default App;