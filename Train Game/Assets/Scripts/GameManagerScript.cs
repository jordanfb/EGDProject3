using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System.IO.Ports;
using System;
using UnityEngine.UI;
using TMPro;

public class GameManagerScript : MonoBehaviour
{
    // Start is called before the first frame update

    //harcoded to only work on my laptop
    public SerialPort stream1 = new SerialPort("/dev/tty.usbmodem142101", 9600);
    //public SerialPort stream2 = new SerialPort("/dev/tty.usbmodem1424401", 9600);
    public List<SerialPort> serialPortsAvailable = new List<SerialPort>();
    public List<SerialPort> serialPortsInUse = new List<SerialPort>();

    //visual Debug
    [Header("Visual Debugging Variables")]
    public GameObject flash;
    public InputField debugInputField;
    public Transform scrollViewContent;
    public GameObject logTextPrefab;
    private string debugInputString;
    public Dropdown debugFromDropDown;
    public Dropdown debugToDropDown;
    private int debugFromPlayer = 1;
    private int debugToPlayer =1;
    public Dictionary<int, GameObject> visualTrainDictionary = new Dictionary<int, GameObject>();
    public GameObject trainPrefab;
    float outerRadius = 4f;
    float innerRadius = 2f;
    //dictionary of int to the port that it controls
    //1-5 are players
    //6 is the lights
    [Header("Game Logic Variables")]
    public Dictionary<int, SerialPort> portDictionary = new Dictionary<int, SerialPort>();

    public Dictionary<SerialPort, Queue<byte>> debugCommandsDictionary = new Dictionary<SerialPort, Queue<byte>>();

    public Dictionary<int, TrainData> trainDictionary = new Dictionary<int, TrainData>();
    public Dictionary<int, PlayerData> playerInfoDictionary = new Dictionary<int, PlayerData>();

    public Dictionary<int, List<PlayerData>> votingDicrionary = new Dictionary<int, List<PlayerData>>();
    public int currentTrainID = 1;
    public float timeInBetweenStations = 4f;
    bool allSerialOpened = true;

    public static GameManagerScript instance;


    bool allHaveBeenAssigned = false;
    private void Awake()
    {
        instance = this;
    }

    public void handleDebugFrom(int input) {
        debugFromPlayer = input+1;
        
    }

    public void addDebugTrain(int id) {
        GameObject newTrain = Instantiate(trainPrefab);

        float radians = trainDictionary[id].radians;
        newTrain.transform.position = new Vector3(
            Mathf.Cos(radians)*outerRadius,
            Mathf.Sin(radians) * outerRadius,
            0f

            );
        visualTrainDictionary.Add(id, newTrain);
    }
    public void addDebugStation(int id)
    {
        GameObject newStation = Instantiate(trainPrefab);
        float radians = playerInfoDictionary[id].radians;
        Debug.Log(radians);
        newStation.transform.position = new Vector3(
            Mathf.Cos(radians) * outerRadius,
            Mathf.Sin(radians) * outerRadius,
            0f
            );
    }

    public void log(string content) {
        TMP_Text text = Instantiate(logTextPrefab, scrollViewContent).GetComponent<TMP_Text>();
        text.text = content;

    }
    public void handleDebugTo(int input)
    {
        debugToPlayer = input+1;
    }
    public void handleDebugInput(string input) {

        QueueDebugCommand(input);
        //MessageData md = new MessageData(SenderHelper.instance.generateCharArray(input));
        //requests.Enqueue();

    }

    void Start()
    {
        //ex 1 and 2 are players
        serialPortsAvailable.Add(stream1);
        //serialPortsAvailable.Add(stream2);

        
        

        

        flash.SetActive(false);


    }
    public bool waitForAssignmentOnStartUp() {


        if (serialPortsAvailable.Count == 0) {

            return true;
        }
        foreach (SerialPort sp in serialPortsAvailable)
        {
            
            try
            {
                if (sp.IsOpen)
                {
                    continue;
                }
                sp.Open();
                sp.ReadTimeout = 1;
                StartCoroutine(readPort(sp));

                StartCoroutine(pingWhoAreYouUntilAssigned(sp));
                Debug.Log("success");
                log("success");
            }
            catch (System.IO.IOException)
            {
                
            }

        }
        return false;
    }


    //will ask who it is until there are no more serial ports to fill
    public IEnumerator pingWhoAreYouUntilAssigned(SerialPort sp) {
        
        while (serialPortsAvailable.Contains(sp)){
            
            SenderHelper.instance.WhoAreYou(sp);
            yield return new WaitForSeconds(0.5f);


        }
        yield return null;
    }



    //each port is constantly running this function, basically just lisening for chars
    //to come through and will build a command, once it finds a null character then the
    //command must be complete

    public void QueueDebugCommand(string command) {

        
        SerialPort originPort = portDictionary[debugFromPlayer];
        Queue<byte> relevantQueue = debugCommandsDictionary[originPort];
        
        
        
        
        foreach (char c in command) {
            if (c == ' ')
            {
                relevantQueue.Enqueue((byte)'\n');
            }
            else if (char.IsDigit(c)) {
                relevantQueue.Enqueue((byte)(c- '0'));
            }
            else
            {
                relevantQueue.Enqueue((byte)c);
            }
            
        }
        relevantQueue.Enqueue(0);
    }
    public IEnumerator readPort(SerialPort sp) {
        //List<char> currentCommand = new List<char>();

        Debug.Log("starting reading on port: " + sp.PortName);
        RecieveCommand currentCommand = null;

        //constantly checking if there is an incoming char to the specific serial port
        //if there is a char to read, then it adds it to the current command
        //if there is not a char to read it just continues.
        bool executingCommand = false;

        while (true) {
            //check if device was disconnected
            
            bool debugInitializedAndPopulated = debugCommandsDictionary.ContainsKey(sp) && debugCommandsDictionary[sp].Count != 0;
            while (sp.BytesToRead > 0 || debugInitializedAndPopulated)
            {

                try
                {
                    //I should be parsing it as they come in
                    byte incomingByte;
                    if (debugInitializedAndPopulated)
                    {
                        
                        incomingByte = debugCommandsDictionary[sp].Dequeue();
                        debugInitializedAndPopulated = debugCommandsDictionary.ContainsKey(sp) && debugCommandsDictionary[sp].Count != 0;
                        
                    }
                    else
                    {

                        incomingByte = (byte)sp.ReadChar();

                    }

                    if (!executingCommand)
                    {
                        //spawns a new command
                        switch ((char)incomingByte)
                        {
                            case 'r':
                                Debug.Log("setting new command to I AM");
                                currentCommand = new RecieveIAm(sp);
                                executingCommand = true;
                                break;
                            case 'n':
                                Debug.Log("setting new command to CREATE TRAIN");
                                currentCommand = new CreateTrain();
                                executingCommand = true;
                                break;
                            case 'o':
                                Debug.Log("setting new command to STOP TRAIN PRESSED");
                                currentCommand = new StopTrainPressed();
                                executingCommand = true;
                                break;
                            case 'p':
                                Debug.Log("setting new command to ANSWER TRAIN");
                                currentCommand = new AnswerTrain();
                                executingCommand = true;
                                break;
                            case 'q':
                                Debug.Log("setting new command to SEND VOTE");
                                currentCommand = new SendVote();
                                executingCommand = true;
                                break;
                            case 's':
                                Debug.Log("setting new command to DEBUG");
                                currentCommand = new ReadError();
                                executingCommand = true;
                                break;

                            default:
                                break;


                        }
                    }
                    else
                    {

                        if (incomingByte == 0)
                        {
                            if (currentCommand != null) {
                                currentCommand.executePopulatedMessage();
                                currentCommand = null;
                                executingCommand = false;
                            }
                            
                        }
                        else
                        {
                            currentCommand.readNextByte(incomingByte);
                        }

                    }


                }
                catch (TimeoutException)
                {

                    //Debug.Log("timeout exception");
                }
            }
            yield return null;
        }
    }




    // Update is called once per frame

    //5 seconds for a circle
    float speed = (2 * Mathf.PI) / 5;
    public List<string> keywords = new List<string> { "z", "z", "z" };
    void Update()
    {

        if (Input.GetKeyDown(KeyCode.Q)) {
            serialPortsAvailable.Clear();
        }
        if (!waitForAssignmentOnStartUp()) {
            return;
        }

        //maybe put this in a coroutine
        foreach (int trainID in trainDictionary.Keys) {
            if (trainDictionary[trainID].isPaused) {
                continue;
            }

            trainDictionary[trainID].radians += speed * Time.deltaTime;
            if (trainDictionary[trainID].isOnAnswerStrip)
            {
                visualTrainDictionary[trainID].transform.position = new Vector3(
                Mathf.Cos(trainDictionary[trainID].radians) * innerRadius,
                Mathf.Sin(trainDictionary[trainID].radians) * innerRadius,
                0f

                );
            }
            else {
                visualTrainDictionary[trainID].transform.position = new Vector3(
                Mathf.Cos(trainDictionary[trainID].radians) * outerRadius,
                Mathf.Sin(trainDictionary[trainID].radians) * outerRadius,
                0f

                );

            }
            
        }

        //if (Input.GetKeyDown(KeyCode.P))
        //{
        //    foreach (int clientID in portDictionary.Keys)
        //    {
        //        SenderHelper.instance.SendCodewords(clientID, keywords.Count, keywords);
        //    }
        //}



    }

    public IEnumerator turnFlashOff() {
        yield return new WaitForSeconds(0.5f);
        flash.SetActive(false);
    }
    public void logOutput(string s) {
        Debug.Log(s);
    }


   
}

//immutable
public struct MessageData {

    public SerialPort sp;
    public char[] message;
    public MessageData(SerialPort _sp, char[] _message) {
        sp = _sp;
        message = _message;
    }
}

public class TrainData {
    public string rightText;
    public string leftText;
    public string answer;
    public int sender;
    public int reciever;
    public float radians;
    public bool isOnAnswerStrip;
    public bool isPaused = false;
    public TrainData(string _rightText, string _leftText, int _sender, int _reciever, float _radians) {
        rightText = _rightText;
        leftText = _leftText;
        sender = _sender;
        reciever = _reciever;
        answer = null;
        radians = _radians;

    }
    

}
//immutable
public struct PlayerData {
    public int id;
    public SerialPort sp;
    public float radians;

    public PlayerData(int _id, SerialPort _sp, float _radians) {
        id = _id;
        sp = _sp;
        radians = _radians;
    }


}
