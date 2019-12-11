using UnityEngine;
using System.Collections;
using System.IO.Ports;
using System.Collections.Generic;

public class RecieverHelper : MonoBehaviour
{
    public static RecieverHelper instance;

    private void Awake()
    {
        instance = this;
    }

}


//assume it has read the first char so far
interface RecieveCommand {
    // this function determines the behavior when you read in a new character 
    void readNextByte(byte b);
    void executePopulatedMessage();   
}

public class RecieveIAm : RecieveCommand
{
    public int recieverID;
    public SerialPort sp;

    private enum state
    {
        initial,
        reciever,
    }

    state currentState = state.reciever;
    public RecieveIAm(SerialPort _sp)
    {
        sp = _sp;
    }
    public void readNextByte(byte b)
    {
        switch (currentState)
        {
            case state.reciever:
                recieverID += (char)b;

                break;
            default:
                break;
        }
    }

    public void executePopulatedMessage()
    {

        //if I am worried about it handleing messages 1 at a time
        //I should have it push to a queue here
        if (recieverID < 1 || recieverID > 6)
        {
            GameManagerScript.instance.log("invalid responce to who are you: " + recieverID);
            return;
        }

        if (GameManagerScript.instance.portDictionary.ContainsKey(recieverID))
        {
            return;
        }
        //put in dictionary

        Debug.Log("Assigned arduino at " + sp.PortName + " to " + recieverID);
        GameManagerScript.instance.log("assigning " + recieverID + " to sp: " + sp.PortName);

        GameManagerScript.instance.serialPortsAvailable.Remove(sp);
        GameManagerScript.instance.serialPortsInUse.Add(sp);

        GameManagerScript.instance.portDictionary.Add(recieverID, sp);
        GameManagerScript.instance.debugCommandsDictionary.Add(sp, new Queue<byte>());

        if (recieverID != 6)
        {
            PlayerData newPlayer = new PlayerData(recieverID, sp, recieverID * ((2 * Mathf.PI) / 5));
            GameManagerScript.instance.playerInfoDictionary.Add(recieverID, newPlayer);
            GameManagerScript.instance.votingDictionary.Add(recieverID, new Queue<PlayerData>());
            GameManagerScript.instance.addDebugStation(recieverID);
            //send keywords if they are a spy, codewords if they are not.
            //if (sp.IsOpen) {
            //    SenderHelper.instance.SendCodewords(recieverID, GameManagerScript.instance.keywords.Count, GameManagerScript.instance.keywords);
            //}

        }
        else {

            GameManagerScript.instance.pingLEDArduino();
        }




    }
}

/// <summary>
/// this is for the lights to tell you their current speed after they get connected in case it isn't restarted
/// </summary>
public class RecieveSetSpeed : RecieveCommand
{

    private enum state
    {
        speed,
        finished,
    }

    state currentState = state.speed;
    public RecieveSetSpeed()
    {

    }

    public void readNextByte(byte b)
    {
        switch (currentState)
        {
            case state.speed:
                GameManagerScript.instance.currentLightDelay = (char)b;
                Debug.Log("Recieved that light speed is " + GameManagerScript.instance.currentLightDelay);
                currentState = state.finished;
                break;
            default:
                break;
        }
    }

    public void executePopulatedMessage()
    {
        // already handled in the readnextbyte function
        GameManagerScript.instance.SendResync();
    }
}



public class RecieveStartResync : RecieveCommand
{
    public RecieveStartResync() { }

    public void readNextByte(byte b) { }

    public void executePopulatedMessage()
    {
        GameManagerScript.instance.isSyncing = true;
        GameManagerScript.instance.resyncTimer = 0;
    }
}

public class RecieveEndResync : RecieveCommand
{
    public RecieveEndResync() { }
    public void readNextByte(byte b) { }

    public void executePopulatedMessage()
    {
        GameManagerScript.instance.isSyncing = false;
        //Debug.Log("Resynced and got " + GameManagerScript.instance.resyncTimer + " which is actually " + (GameManagerScript.instance.resyncTimer / GameManagerScript.instance.resyncLoops));
        //char speed = (char)((GameManagerScript.instance.currentLightDelay) - ((GameManagerScript.instance.resyncTimer - GameManagerScript.instance.timeForRotation) * 10));
        //Debug.Log("It will timer to be " + (int)speed);
        GameManagerScript.instance.SetSpeedBasedOnResync();
    }
}

//Client -> Hub
//when it recieves the command to create a train
public class CreateTrain : RecieveCommand
{
    public int senderID;
    public int recieverID;
    string leftText;
    string rightText;
    private enum state
    {
        sender,
        reciever,
        leftText,
        rightText,
        end,
    }

    state currentState = state.sender;
    public void executePopulatedMessage()
    {

        //sets the initial radians to whoever sent the trai.
        Debug.Log(senderID);
        float radians = GameManagerScript.instance.playerInfoDictionary[senderID].radians;
        TrainData newTrainData = new TrainData(rightText, leftText, senderID, recieverID, radians);

        //add to the train dictionary here
        GameManagerScript.instance.trainDictionary.Add(GameManagerScript.instance.currentTrainID, newTrainData);
        
        SenderHelper.instance.CreateTrainLights(senderID, recieverID, GameManagerScript.instance.currentTrainID);
        //adds to train dictionary
        
        GameManagerScript.instance.currentTrainID++;
    }

    public void readNextByte(byte b)
    {

        switch (currentState) {
            case state.sender:
                senderID += b;
                currentState = state.reciever;
                break;
            case state.reciever:
                recieverID += b;
                currentState = state.leftText;
                break;
            case state.leftText:
                if ((char)b == '\n')
                {
                    currentState = state.rightText;
                }
                else {
                    leftText += (char)b;
                }
                
                break;
            case state.rightText:
                if ((char)b == '\n')
                {
                    currentState = state.end;
                }
                else
                {
                    rightText += (char)b;
                }
                break;
            case state.end:
                break;
            default:
                break;
        }
    }
}

public class StopTrainPressed : RecieveCommand
{
    public int senderID;

    public void executePopulatedMessage()
    {
        
        float minDist = Mathf.Infinity;
        PlayerData senderInfo = GameManagerScript.instance.playerInfoDictionary[senderID];
        int minID = 0;
        foreach (int trainID in GameManagerScript.instance.trainDictionary.Keys) {

            float trainRadian = GameManagerScript.instance.trainDictionary[trainID].radians;
            float clientRadian = senderInfo.radians;
            trainRadian %= (2*Mathf.PI);

            //i think this is correct
            float diff = Mathf.Min(Mathf.Abs(clientRadian - trainRadian), Mathf.Abs((clientRadian + 2*Mathf.PI) - trainRadian));

            if (diff < minDist)
            {
                minID = trainID;
                minDist = diff;
                
            }
        }
        if (minDist > 1f)
        {
            return;
        }


        //send paused train to hub

        //

        SenderHelper.instance.PauseTrainLights(senderID, minID);
        //display message if it was meant for them and prompt for answer
        if (GameManagerScript.instance.trainDictionary[minID].reciever == senderID)
        {
            
            if (GameManagerScript.instance.trainDictionary[minID].answer.Equals(""))
            {
                SenderHelper.instance.DisplayTrainThatNeedsAnswer(
                senderID,
                minID,
                GameManagerScript.instance.trainDictionary[minID].sender,
                GameManagerScript.instance.trainDictionary[minID].reciever,
                GameManagerScript.instance.trainDictionary[minID].leftText,
                GameManagerScript.instance.trainDictionary[minID].rightText);
                // don't release the train, it'll be released when you answer the message
            }
            else
            {

                
                SenderHelper.instance.DisplayTrainDontAnswer(
                senderID,
                GameManagerScript.instance.trainDictionary[minID].sender,
                GameManagerScript.instance.trainDictionary[minID].reciever,
                GameManagerScript.instance.trainDictionary[minID].leftText,
                GameManagerScript.instance.trainDictionary[minID].rightText,
                GameManagerScript.instance.trainDictionary[minID].answer);
                if (GameManagerScript.instance.trainDictionary[minID].sender != senderID)
                {
                    // if it's not your train resume it, it's already answered
                    // only resume it if you're not going to destroy it
                    GameManagerScript.instance.StartCoroutine(GameManagerScript.instance.ReleaseTrainAfterTime(4f, minID)); // start the train again after a while since it starts again automatically!
                }
            }
            
            //display train you need to answer
        }
        else {
            SenderHelper.instance.DisplayTrainDontAnswer(
                senderID,
                GameManagerScript.instance.trainDictionary[minID].sender,
                GameManagerScript.instance.trainDictionary[minID].reciever,
                GameManagerScript.instance.trainDictionary[minID].leftText,
                GameManagerScript.instance.trainDictionary[minID].rightText,
                GameManagerScript.instance.trainDictionary[minID].answer);
            //display train don't answer
            if (GameManagerScript.instance.trainDictionary[minID].sender != senderID)
            {
                // if it's not your train, you should release it after a bit
                GameManagerScript.instance.StartCoroutine(GameManagerScript.instance.ReleaseTrainAfterTime(4f, minID)); // start the train again after a while since it starts again automatically!
            }
        }

        if (GameManagerScript.instance.trainDictionary[minID].sender == senderID) {

            //display the answer
            SenderHelper.instance.DisplayTrainDontAnswer(
                senderID,
                GameManagerScript.instance.trainDictionary[minID].sender,
                GameManagerScript.instance.trainDictionary[minID].reciever,
                GameManagerScript.instance.trainDictionary[minID].leftText,
                GameManagerScript.instance.trainDictionary[minID].rightText,
                GameManagerScript.instance.trainDictionary[minID].answer);
            GameManagerScript.instance.handleSuccessfulCodewordRecieved(GameManagerScript.instance.trainDictionary[minID].sender,
                GameManagerScript.instance.trainDictionary[minID].reciever,
                GameManagerScript.instance.trainDictionary[minID].leftText,
                GameManagerScript.instance.trainDictionary[minID].rightText
                );
            SenderHelper.instance.DestroyTrainLights(minID);
        }
    }

    public void readNextByte(byte b)
    {
        senderID += (char)b;
    }
}


//user recieves a message that they need to send a train answer
//Answer train gets fired after the timeout of stopping a train
//OR it gets fired when you send an answer train

//example if someone stops a train that is not intended for them, it gets printed in th
//DisplayTrainDontAnswer message, then, after 3 seconds, this message is sent from the
//client arduino to resume the train if it wasn't meant for them
// isRight integer --> 1 = left, 2 = right
public class AnswerTrain : RecieveCommand
{
    int senderID;
    int trainID;
    int isRight;

    private enum state
    {
        sender,
        trainID,
        answer
    }
    state currentState = state.sender;
    public void executePopulatedMessage()
    {


        if (GameManagerScript.instance.trainDictionary.ContainsKey(trainID))
        {
            //train indended for them
            if (senderID == GameManagerScript.instance.trainDictionary[trainID].reciever)
            {
                //spawn train on answer track
                if (isRight == 2)
                {
                    GameManagerScript.instance.trainDictionary[trainID].answer = GameManagerScript.instance.trainDictionary[trainID].rightText;
                }
                else
                {
                    GameManagerScript.instance.trainDictionary[trainID].answer = GameManagerScript.instance.trainDictionary[trainID].leftText;
                }
                SenderHelper.instance.MoveTrainToAnswerTrack(trainID);
                SenderHelper.instance.ReleaseTrainLights(trainID);

            }
            else
            {
                if (GameManagerScript.instance.trainDictionary[trainID].answer == null)
                {
                    //unpause train on not answertrack
                    SenderHelper.instance.ReleaseTrainLights(trainID);
                }
                else
                {
                    SenderHelper.instance.ReleaseTrainLights(trainID);
                    //unpause train on answertrack
                }
            }
        }
    }

    public void readNextByte(byte b)
    {

        switch (currentState)
        {
            case state.sender:
                senderID += b;
                currentState = state.trainID;
                break;
            case state.trainID:
                trainID += b;
                currentState = state.answer;
                break;
            case state.answer:
                //maybe add some case handleing to make sure this number is 0 or 1
                isRight += b;
                break;
            default:
                break;
        }
    }
}

public class SendVote : RecieveCommand
{
    public int senderID;
    public int playerVoted;


    private enum state
    {
        sender,
        playerVoted,
        end
    }
    state currentState = state.sender;
    public void executePopulatedMessage()
    {


        //always will be at least of size 1
        if (GameManagerScript.instance.votingDictionary[senderID].Contains(
            GameManagerScript.instance.playerInfoDictionary[playerVoted])) {
            Debug.Log("player already voted for");
            return;
        }
        if (senderID == playerVoted)
        {
            Debug.Log("You cannot vote for yourself");
            return;
        }
        GameManagerScript.instance.votingDictionary[senderID].Enqueue(
            GameManagerScript.instance.playerInfoDictionary[playerVoted]);

        int firstPlayer;
        int secondPlayer = 6;
        if (GameManagerScript.instance.votingDictionary[senderID].Count >= 2)
        {
            if (GameManagerScript.instance.votingDictionary[senderID].Count > 2) {
                GameManagerScript.instance.votingDictionary[senderID].Dequeue();
            }
            Debug.Log("List is now:");
            foreach (PlayerData p in GameManagerScript.instance.votingDictionary[senderID])
            {
                Debug.Log("\tvote: " + p.id);
            }
            PlayerData[] voteArray = GameManagerScript.instance.votingDictionary[senderID].ToArray();
            firstPlayer = voteArray[0].id;
            secondPlayer = voteArray[1].id;
        }
        else {
            //there is only 1
            firstPlayer = GameManagerScript.instance.votingDictionary[senderID].Peek().id;
        }


        
        SenderHelper.instance.SendVoteLights(senderID, firstPlayer, secondPlayer);


    }

    public void readNextByte(byte b)
    {

        switch (currentState) {
            case state.sender:
                senderID += b;
                currentState = state.playerVoted;
                break;
            case state.playerVoted:
                playerVoted += b;
                currentState = state.end;
                break;
            case state.end:
                break;
            default:
                break;
        }
        
    }
}

public class ReadError : RecieveCommand
{
    public string debugMessage = "";
    public string whoAmI = "no port set";

    private enum state
    {
        debug,
        end
    }
    state currentState = state.debug;
    public void executePopulatedMessage()
    {

        GameManagerScript.instance.log(debugMessage);
        if (debugMessage.Length != 0)
        {
            Debug.LogWarning("debug message from: " + whoAmI + ": ascii: " + (byte)debugMessage[0] + ", char: " + debugMessage);

        }
        else
        {
            Debug.LogWarning("Debug message from: " + whoAmI + " length of debug message is 0");

        }

    }

    public void readNextByte(byte b)
    {

        switch (currentState)
        {
            case state.debug:
                debugMessage += (char)b;
                break;
            case state.end:
                break;
            default:
                break;
        }

    }


}


public class RecievePongFromLEDArduino : RecieveCommand
{

    public void readNextByte(byte b)
    {
        
    }

    public void executePopulatedMessage()
    {
        // already handled in the readnextbyte function
        GameManagerScript.instance.isPinging = false;
        Debug.Log("this in the ping from the hub to the LED: " + GameManagerScript.instance.pingTimer / 2f);
        GameManagerScript.instance.pingTime = GameManagerScript.instance.pingTimer / 2f;
    }
}

public class RecieveTrainPing : RecieveCommand
{
    public int trainID;

    private enum state
    {
        trainID,
        end
    }
    state currentState = state.trainID;
    public void executePopulatedMessage()
    {

        //I need to see what radians the train id is at ping time seconds ago
        //if it is not directly in front of the sender's station, as the message suggested
        //that it be, I need to move it there

        float radiansBehind = GameManagerScript.instance.pingTime * (Mathf.PI * 2 / GameManagerScript.instance.timeForRotation);

        //float posAtPingTime = GameManagerScript.instance.trainDictionary[trainID].radians - radiansBehind;
        //if (posAtPingTime < 0) {
        //    posAtPingTime += Mathf.PI * 2;
        //}
        int sender = GameManagerScript.instance.trainDictionary[trainID].sender;
        float radianDifference = radiansBehind + GameManagerScript.instance.playerInfoDictionary[sender].radians;
        Debug.Log("DIFFERENCE BETWEEN UNITY AND LED STRIP IN RADIANS: " + radianDifference);
        GameManagerScript.instance.trainDictionary[trainID].radians += radianDifference;

    }

    public void readNextByte(byte b)
    {

        switch (currentState)
        {
            case state.trainID:
                trainID += b;
                break;
            case state.end:
                break;
            default:
                break;
        }

    }
}