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
    public Dictionary<int, GameObject> visualStationDictionary = new Dictionary<int, GameObject>();

    public GameObject trainPrefab;
    public GameObject stationPrefab;
    float outerRadius = 4f;
    float innerRadius = 2f;
    public Sprite trainHeadSprite;
    public Sprite trainTailSprite;
    public Color[] COLORS = { Color.red, Color.blue, Color.green, Color.magenta, Color.yellow };
    public TMP_Text timerText;
    //dictionary of int to the port that it controls
    //1-5 are players
    //6 is the lights
    [Header("Game Logic Variables")]
    public Dictionary<int, SerialPort> portDictionary = new Dictionary<int, SerialPort>();

    public Dictionary<SerialPort, Queue<byte>> debugCommandsDictionary = new Dictionary<SerialPort, Queue<byte>>();

    public Dictionary<int, TrainData> trainDictionary = new Dictionary<int, TrainData>();
    public Dictionary<int, PlayerData> playerInfoDictionary = new Dictionary<int, PlayerData>();

    public Dictionary<int, Queue<PlayerData>> votingDictionary = new Dictionary<int, Queue<PlayerData>>();
    public int currentTrainID = 1;
    public float timeForRotation = 4f;
    public bool gameStarted = false;
    public float timePerRound = 60f;
    public float currentTime;
    public bool gamePaused= false;

    public List<string> keywords = new List<string>();
    public List<string> codewords = new List<string>();


    public static GameManagerScript instance;


    private void Awake()
    {
        instance = this;
    }
    void Start()
    {
        //ex 1 and 2 are players
        keywords = new List<string> { "DeathStar" };
        codewords = new List<string> { "Space", "Sphere", "Wow" };
        currentTime = timePerRound;
        TryAttachingToAllPorts();

    }

    public void handleDebugFrom(int input) {
        debugFromPlayer = input+1;
        
    }

    public void SendNewGameAll() {
        gameStarted = true;
        byte[] message = { (byte)'k', 0 };

        foreach (SerialPort sp in portDictionary.Values) {
            if (sp.IsOpen) {
                sp.Write(message, 0, message.Length);
            }
            
        }

        foreach (GameObject go in visualTrainDictionary.Values) {
            Destroy(go);
        }

        assignRoles();
        foreach (int id in playerInfoDictionary.Keys) {
            votingDictionary[id].Clear();
            visualStationDictionary[id].transform.Find("vote1")
            .gameObject.GetComponent<SpriteRenderer>().color = Color.white;
            visualStationDictionary[id].transform.Find("vote2")
            .gameObject.GetComponent<SpriteRenderer>().color = Color.white;
            if (playerInfoDictionary[id].isSpy)
            {
                SenderHelper.instance.SendCodewords(id, codewords.Count, codewords);
            }
            else {
                SenderHelper.instance.SendKeywords(id, keywords.Count, keywords);

            }
        }
        
        trainDictionary.Clear();
        visualTrainDictionary.Clear();
        //votingDictionary.Clear();
        
    }

    public void assignRoles() {
        if (playerInfoDictionary.Count == 0 || playerInfoDictionary.Count == 1) {
            return;
        }
        int spy1Index = (int)Mathf.Floor(UnityEngine.Random.value * playerInfoDictionary.Count)+1;
        int spy2Index = spy1Index;
        while (spy2Index == spy1Index)
        {
            spy2Index = (int)Mathf.Floor(UnityEngine.Random.value * playerInfoDictionary.Count) + 1;
        }
        foreach (int id in playerInfoDictionary.Keys) {
            playerInfoDictionary[id].isSpy = false;
            visualStationDictionary[id].transform.Find("isSpySprite").gameObject.SetActive(false);
        }
        Debug.Log("spy index: " + spy1Index);
        Debug.Log("spy index: " + spy2Index);
        Debug.Log("number of players in player dictionary: " + playerInfoDictionary.Count);
        playerInfoDictionary[spy1Index].isSpy = true;
        visualStationDictionary[spy1Index].transform.Find("isSpySprite").gameObject.SetActive(true);
        playerInfoDictionary[spy2Index].isSpy = true;
        visualStationDictionary[spy2Index].transform.Find("isSpySprite").gameObject.SetActive(true);


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
        newStation.GetComponent<SpriteRenderer>().color = COLORS[id-1];
        visualStationDictionary.Add(id, newStation);
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

        if (command.Equals("PAUSE")) {
            gamePaused = true;
            SenderHelper.instance.PauseAllTrainLights();
            return;
        }
        if (command.Equals("RESUME"))
        {
            gamePaused = false;
            SenderHelper.instance.ResumeAllTrainLights();
            return;
        }
        int fromPlayer;
        if (command.Length >= 2)
        {
            if (!char.IsNumber(command[1]))
            {
                Debug.LogError("ERROR: second number was not a number");
                return;
            }
            fromPlayer = command[1] - '0';
            if (fromPlayer > 6 || fromPlayer < 1)
            {
                Debug.LogError("ERROR: sender must be between 1 and 6");
                return;
            }
        }
        else {
            Debug.LogError("ERROR: command must be at least 2 characters");
            return;
        }
        
        Debug.Log("from player: " + fromPlayer);
        SerialPort originPort = portDictionary[fromPlayer];
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

            //if (debugInitializedAndPopulated) {
            //    Debug.Log(debugCommandsDictionary[sp].Dequeue());
            //}

            

            while ((debugInitializedAndPopulated || (sp.IsOpen && sp.BytesToRead > 0)))
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
                    //reject all commands if the game is paused except for vote:)
                    if (currentCommand == null)
                    {
                        //spawns a new command
                        switch ((char)incomingByte)
                        {
                            case 'r':
                                if (gamePaused) {
                                    break;
                                }
                                Debug.Log("setting new command to I AM for sp " + sp.PortName);	
                                currentCommand = new RecieveIAm(sp);
                                break;
                            case 'n':
                                if (gamePaused || !gameStarted)
                                {
                                    break;
                                }
                                Debug.Log("setting new command to CREATE TRAIN");
                                currentCommand = new CreateTrain();
                                break;
                            case 'o':
                                if (gamePaused || !gameStarted)
                                {
                                    break;
                                }
                                Debug.Log("setting new command to STOP TRAIN PRESSED");
                                currentCommand = new StopTrainPressed();
                                break;
                            case 'p':
                                if (gamePaused || !gameStarted)
                                {
                                    break;
                                }
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
    
    public void handleTime() {
        timerText.text = currentTime.ToString();
        if (!gameStarted || gamePaused) {
            return;
        }
        currentTime -= Time.deltaTime;
        if (currentTime <= 0) {
            Debug.Log("game over");
            //just restarts the game once it ends.
            SendNewGameAll();
        }

    }
    void Update()
    {
        
        
        //adding fake players
        if (Input.GetKeyDown(KeyCode.Equals))
        {
            createTrainWithoutSerialPort();
        }
        if (!waitForAssignmentOnStartUp()) {
            return;
        }
        handleTime();
        if (!gameStarted)
        {
            return;
        }
        //maybe put this in a coroutine
        foreach (int trainID in trainDictionary.Keys) {
            //the train is repositioned in the 
            if (!trainDictionary[trainID].isPaused)
            {
                trainDictionary[trainID].radians += speed * Time.deltaTime;
            }


           
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
public class PlayerData {
    public int id;
    public SerialPort sp;
    public float radians;
    public bool isSpy = false;
    public PlayerData(int _id, SerialPort _sp, float _radians) {
        id = _id;
        sp = _sp;
        radians = _radians;
    }
}
