import { useEffect, useRef } from 'react'
import './App.css'

function App() {
  const containerRef = useRef<HTMLDivElement>(null)
  const boxRef = useRef<HTMLDivElement>(null)
  
  const isClicked = useRef<boolean>(false);

  useEffect(() => {
    if (!boxRef.current || !containerRef.current) return;

    const box = boxRef.current;
    const container = containerRef.current;

    const onMouseDown = (e: MouseEvent) => {
      isClicked.current = true;
    }
    const onMouseUp = (e:MouseEvent) => {
      isClicked.current = false;
    }
    const onMouseMove = (e: MouseEvent) => {
      if (!isClicked.current) return;
      box.style.top = `${e.clientY}px`;
      box.style.left = `${e.clientX}px`;
    }

    box.addEventListener('mousedown', onMouseDown);
    box.addEventListener('mouseup', onMouseUp);
    container.addEventListener('mousemove', onMouseMove);

    const cleanup = () => {
      box.removeEventListener('mousedown', onMouseDown);
      box.removeEventListener('mouseup', onMouseUp);
      container.removeEventListener('mousemove', onMouseMove);
    }
    return cleanup;
  }, [])

  return (
    <main>
      <div ref={containerRef} className="container">
        <div ref={containerRef} className="box">

        </div>
      </div>
    </main>
  )
}

export default App
