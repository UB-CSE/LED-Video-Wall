import { useSelector } from 'react-redux';
import type { RootState } from '../state/store';

function SaveButton(){
    const configState = useSelector((state: RootState) => state.config);

    //Sends the current configuration in state to the web server
    function sendToServer(){
        //Creates JSON object and sets gamma
        let config: {settings: {gamma: number}, elements: Record<string, any>} = {
            'settings': {'gamma': configState.settings.gamma},
            'elements': {}
        }
        //Adds all elements from state
        for(let i = 0; i < configState.elements.length; i++){
            let element = configState.elements[i];
            config['elements'][element.name] = {
                'id': element.id,
                'type': element.type,
                'filepath': element.filepath,
                'location': element.location
            }
        }
        //Sends JSON to web server
        fetch('/set_yaml_Config', {
            method: 'POST',
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({
                config
            })})
    }
    //Button JSX with a description of the button functionality
    return (
        <div>
            <button onClick={() => sendToServer()}>Save</button>
            <p>Click to save the current configuration to the yaml file</p>
        </div>
    );
}
export default SaveButton;