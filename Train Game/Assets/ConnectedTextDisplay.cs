using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ConnectedTextDisplay : MonoBehaviour
{
    public GameManagerScript gameManager;
    public Text connectedText;
    private HashSet<int> connected = new HashSet<int>();

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        bool changed = false;
        foreach (int user in gameManager.portDictionary.Keys)
        {
            if (!connected.Contains(user)) {
                connected.Add(user);
                changed = true;
            }
        }
        if (gameManager.portDictionary.Keys.Count != connected.Count)
        {
            // remove the extra ones
            Debug.LogError("Error, have connected things that aren't connected");
            changed = true;
        }

        if (changed)
        {
            //Debug.Log("Connected text changed " + connected.Count);
            string text = "";
            foreach (int i in connected)
            {
                //Debug.Log("Connected to " + i);
                if (i == 6)
                {
                    text += "Lights " + " ";
                }
                else
                {
                    text += "Player " + i + " " ;
                }
            }
            connectedText.text = text;
        }
    }
}
