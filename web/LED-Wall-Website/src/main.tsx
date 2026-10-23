import {StrictMode} from 'react'
import {createRoot} from 'react-dom/client'
import App from './App.tsx'
import {VideoWallProvider} from './context/VideoWallContext.tsx'

createRoot(document.getElementById('root')!).render(
    <StrictMode>
        <VideoWallProvider>
            <App/>
        </VideoWallProvider>
    </StrictMode>,
)
