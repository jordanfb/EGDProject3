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
    //public SerialPort stream1 = new SerialPort("/dev/tty.usbmodem142101", 9600);
    //public SerialPort stream0 = new SerialPort("COM4", 9600);
    //public SerialPort stream1 = new SerialPort("COM6", 9600);
    //public SerialPort stream2 = new SerialPort("COM7", 9600);
    //public SerialPort stream2 = new SerialPort("/dev/tty.usbmodem1424401", 9600);
    public List<SerialPort> serialPortsAvailable = new List<SerialPort>();
    public List<SerialPort> serialPortsInUse = new List<SerialPort>();
    public List<SerialPort> portsWeveOpened = new List<SerialPort>(); // this is in order to close them so that we can upload to the board and whatever else we want but don't have to use them for the game

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
    public GameObject stationPrefab;
    float outerRadius = 4f;
    float innerRadius = 2f;
    public Sprite trainHeadSprite;
    public Sprite trainTailSprite;
    public Color[] COLORS = { Color.red, Color.blue, Color.green, Color.magenta, Color.yellow };
    //dictionary of int to the port that it controls
    //1-5 are players
    //6 is the lights
    [Header("Game Logic Variables")]
    public Dictionary<int, SerialPort> portDictionary = new Dictionary<int, SerialPort>();

    public Dictionary<SerialPort, Queue<byte>> debugCommandsDictionary = new Dictionary<SerialPort, Queue<byte>>();

    public Dictionary<int, TrainData> trainDictionary = new Dictionary<int, TrainData>();
    public Dictionary<int, PlayerData> playerInfoDictionary = new Dictionary<int, PlayerData>();

    public Dictionary<int, List<PlayerData>> votingDictionary = new Dictionary<int, List<PlayerData>>();
    public int currentTrainID = 1;
    public float timeForRotation = 4f;
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

    public void SendNewGameAll() {
        byte[] message = { (byte)'k', 0 };

        foreach (SerialPort sp in portDictionary.Values) {
            sp.Write(message, 0, message.Length);
        }

        foreach (GameObject go in visualTrainDictionary.Values) {
            Destroy(go);
        }

        trainDictionary.Clear();
        visualTrainDictionary.Clear();
    }

    public void SendCodewordsAndKeywords()
    {
        foreach (int user in portDictionary.Keys)
        {
            if (user < 6)
            {
                // it's a player!
                if (UnityEngine.Random.Range(0f, 1) < .5f)
                {
                    // they're a townsfolk
                    SenderHelper.instance.SendKeywords(user, keywords.Count, keywords);
                } else
                {
                    // they're a spy
                    SenderHelper.instance.SendCodewords(user, keywords.Count, keywords);
                }
            }
        }
    }

    public void addDebugTrain(int id) {
        GameObject newTrain = Instantiate(trainPrefab);
        newTrain.transform.Find("trainHead").GetComponent<SpriteRenderer>().color =
            COLORS[trainDictionary[id].sender - 1];
        newTrain.transform.Find("trainBack").GetComponent<SpriteRenderer>().color =
            COLORS[trainDictionary[id].reciever - 1];
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
        GameObject newStation = Instantiate(stationPrefab);
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
        text.transform.SetAsFirstSibling();
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

    void TryAttachingToAllPorts()
    {
        string[] ports = SerialPort.GetPortNames();
        Debug.Log("The following serial ports were found:");
        // Display each port name to the console.
        foreach (string port in ports)
        {
            #if UNITY_STANDALONE_OSX
            if (!port.Contains("/dev/tty.usb")) {
                continue;
            }
            Debug.Log("MAC PORT BAYBEE: " + port);
            #else
            #endif
                try
            {
                SerialPort testPort = new SerialPort(port);
                if (testPort.IsOpen)
                {
                    continue; // don't try since someone else has it
                }
                else
                {
                    // try adding it to the list!
                    Debug.Log("Port baud " + testPort.BaudRate);
                    serialPortsAvailable.Add(testPort);
                    Debug.Log("Tried attaching to the port! " + port);
                }
            }
            catch (System.IO.IOException)
            {
                // don't worry we don't care if it fails to add it we just ignore it
            }
        }
    }

    void Start()
    {
        //ex 1 and 2 are players


        TryAttachingToAllPorts();

    }

    private void OnDestroy()
    {
        // loop over all the ports we've opened and close them, useful for uploading code
        foreach(SerialPort p in portsWeveOpened)
        {
            p.Close();
            Debug.Log("Closed serial port: " + p.PortName);
        }
        portsWeveOpened = new List<SerialPort>();
    }

    public bool waitForAssignmentOnStartUp() {


        if (serialPortsInUse.Count >= 2) {

            return true;
        }
        List<SerialPort> portsToRemove = new List<SerialPort>();
        foreach (SerialPort sp in serialPortsAvailable)
        {
            
            try
            {
                if (sp.IsOpen)
                {
                    continue;
                }
                sp.Open();
                if (sp.IsOpen)
                {
                    portsWeveOpened.Add(sp);
                    sp.ReadTimeout = 1;
                    StartCoroutine(readPort(sp));

                    StartCoroutine(pingWhoAreYouUntilAssigned(sp));
                    Debug.Log("Found a port to query");
                    log("Found a port to query");
                } else
                {
                    // should remove it from the list probably... it's defnitely not worth it
                    portsToRemove.Add(sp);
                }
            }
            catch (System.IO.IOException)
            {
                portsToRemove.Add(sp);
            }

        }
        for (int i =0; i < portsToRemove.Count; i++)
        {
            Debug.Log("Removed potential port that didn't work " + portsToRemove[i].PortName);
            serialPortsAvailable.Remove(portsToRemove[i]);
        }
        return false;
    }


    //will ask who it is until there are no more serial ports to fill
    public IEnumerator pingWhoAreYouUntilAssigned(SerialPort sp)
    {
        while (serialPortsAvailable.Contains(sp) && serialPortsInUse.Count < 6 && sp.IsOpen)
        {
            if (!SenderHelper.instance.WhoAreYou(sp))
            {
                // then it's a bad port and we should feel bad and close it
                break;
            }
            yield return new WaitForSeconds(3f);
        }
        yield return null;
        if (!serialPortsInUse.Contains(sp))
        {
            // then close it if we aren't using it!
            sp.Close();
            portsWeveOpened.Remove(sp);
            Debug.Log("Closed unused port " + sp.PortName);
        }
    }


    public void QueueDebugCommand(string command) {

        
        SerialPort originPort = portDictionary[debugFromPlayer];
        Queue<byte> relevantQueue = debugCommandsDictionary[originPort];

        Debug.Log("origin port: " + originPort.PortName);
        
        
        foreach (char c in command) {
            if (c == ' ')
            {
                relevantQueue.Enqueue((byte)'\n');
            }
            else if (char.IsDigit(c)) {
                relevantQueue.Enqueue((byte)(c - '0'));
                //relevantQueue.Enqueue((byte)(c));
            }
            else
            {
                relevantQueue.Enqueue((byte)c);
            }
            
        }
        relevantQueue.Enqueue(0);
    }

    //each port is constantly running this function, basically just lisening for chars
    //to come through and will build a command, once it finds a null character then the
    //command must be complete
    public IEnumerator readPort(SerialPort sp) {
        //List<char> currentCommand = new List<char>();

        Debug.Log("starting reading on port: " + sp.PortName);
        RecieveCommand currentCommand = null;

        //constantly checking if there is an incoming char to the specific serial port
        //if there is a char to read, then it adds it to the current command
        //if there is not a char to read it just continues.

        while (true) {
            //check if device was disconnected
            bool debugInitializedAndPopulated = debugCommandsDictionary.ContainsKey(sp) && debugCommandsDictionary[sp].Count != 0;

            //if it has bytes to read OR the debug array exists AND had stuff in it



            

            while (debugInitializedAndPopulated || (sp.IsOpen && sp.BytesToRead > 0))
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

                        //incomingByte = (byte)sp.ReadChar();

                        if (sp.IsOpen)
                        {
                            incomingByte = (byte)sp.ReadChar();
                        }
                        else
                        {
                            throw new TimeoutException();
                        }



                    }

                    if (currentCommand == null)
                    {
                        //spawns a new command
                        switch ((char)incomingByte)
                        {
                            case 'r':
                                Debug.Log("setting new command to I AM for sp " + sp.PortName);	
                                currentCommand = new RecieveIAm(sp);
                                break;
                            case 'n':
                                Debug.Log("setting new command to CREATE TRAIN");
                                currentCommand = new CreateTrain();
                                break;
                            case 'o':
                                Debug.Log("setting new command to STOP TRAIN PRESSED");
                                currentCommand = new StopTrainPressed();
                                break;
                            case 'p':
                                Debug.Log("setting new command to ANSWER TRAIN");
                                currentCommand = new AnswerTrain();
                                break;
                            case 'q':
                                Debug.Log("setting new command to SEND VOTE");
                                currentCommand = new SendVote();
                                break;
                            case 's':
                                //Debug.Log("setting new command to DEBUG");
                                currentCommand = new ReadError();
                                ((ReadError)currentCommand).whoAmI = sp.PortName;
                                break;
                            default:
                                Debug.LogError("Unable to figure out command sent to me: " + incomingByte + " " + (char)incomingByte);
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

    public void handleMessages() {

    }
    public IEnumerator ReleaseTrainAfterTime(float time, int trainID)
    {
        yield return new WaitForSeconds(time);
        Debug.Log("Train started again after " + time + " seconds");
        SenderHelper.instance.ReleaseTrainLights(trainID);
    }


    // Update is called once per frame

    //5 seconds for a circle
    //a 4th of the track every seconf. the whole track in 4 seconds
    float speed = (2 * Mathf.PI) / 4;

    float innerSpeed = (2 * Mathf.PI) / (1f/3.6f);
    public List<string> keywords = new List<string> { "z", "z", "z" };
    void Update()
    {
        
        if (Input.GetKeyDown(KeyCode.Equals))
        {
            createTrainWithoutSerialPort();
        }
        if (!waitForAssignmentOnStartUp()) {
            return;
        }

        //maybe put this in a coroutine
        foreach (int trainID in trainDictionary.Keys) {
            if (trainDictionary[trainID].isPaused)
            {
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
                visualTrainDictionary[trainID].transform.Find("trainBack").gameObject.SetActive(false);
            }
            else {
                visualTrainDictionary[trainID].transform.position = new Vector3(
                Mathf.Cos(trainDictionary[trainID].radians) * outerRadius,
                Mathf.Sin(trainDictionary[trainID].radians) * outerRadius,
                0f

                );

            }
            visualTrainDictionary[trainID].transform.Find("trainHead").localRotation =
                Quaternion.Euler(new Vector3(0, 0, 10f));
           
           visualTrainDictionary[trainID].transform.Find("trainBack").localRotation =
                Quaternion.Euler(new Vector3(0, 0, -10f));

           visualTrainDictionary[trainID].transform.rotation = Quaternion.Euler(new Vector3(0, 0,
                    trainDictionary[trainID].radians * Mathf.Rad2Deg + 90f
                ));


        }

    }

    public void createTrainWithoutSerialPort() {

        int fakeTrainID = 0;
        for (int i = 1; i < 6; i++) {
            if (portDictionary.ContainsKey(i)) {
                continue;
            }
            fakeTrainID = i;
            break;
        }
        if (fakeTrainID == 0) {
            Debug.Log("All Users are assigned");
            return;
        }

        SerialPort sp = new SerialPort("fake" + fakeTrainID);
        RecieveIAm fakeReciept = new RecieveIAm(sp);
        fakeReciept.recieverID = fakeTrainID;
        fakeReciept.executePopulatedMessage();
        StartCoroutine(readPort(sp));
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
        answer = "";
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
