using UnityEngine;
using System.Collections;
using System.IO.Ports;
using System.Collections.Generic;
using System;

//this contains all of the functions for sending and recieving messages
public class SenderHelper : MonoBehaviour
{

    public static SenderHelper instance;

    private void Awake()
    {
        instance = this;
    }

    public byte getNumberAsByte(int number)
    {
        return (byte)number;
        // return (byte)((char)number - (char)'0');
    }

    public byte[] generateCharArray(string message)
    {

        List<byte> returnList = new List<byte>();
        foreach (char c in message)
        {
            returnList.Add((byte)c);
            if (c == '\0')
            {
                Debug.LogError("Had null character in string");
            }
        }
        returnList.Add(0);

        return returnList.ToArray();

    }
    public bool WhoAreYou(SerialPort dest)
    {
        try
        {
            GameManagerScript.instance.log("sending who are you to port: " + dest.PortName);
            List<byte> message = new List<byte> { (byte)'a', 0 };
            //foreach (byte b in message) {
            //    Debug.Log("ascii: " + b + ", char: " + (char)b);
            //}
            dest.Write(message.ToArray(), 0, message.Count);
            return true;
        } catch (System.TimeoutException)
        {
            // the port timed out and probably shouldn't be used...
            return false;
        }
    }

    //send to LED arduino
    public void CreateTrainLights(int senderID, int recieverID, int trainID)
    {
        //example of how i'm sending it to the strip
        byte[] message = { (byte)'b', (byte)senderID, (byte)recieverID, (byte)trainID, 0 };

        GameManagerScript.instance.addDebugTrain(trainID);
        GameManagerScript.instance.log("Creating train on arduino from: " + senderID +
            " to " + recieverID + " with id: " + trainID);
        if (GameManagerScript.instance.portDictionary.ContainsKey(6))
        {
            GameManagerScript.instance.portDictionary[6].Write(message, 0, (char)message.Length);
        }
        else
        {
            GameManagerScript.instance.log("LED arduino not set");
        }


    }
    public void PauseTrainLights(int stopperID, int trainID)
    {

        GameManagerScript.instance.trainDictionary[trainID].isPaused = true;
        GameManagerScript.instance.log("player " + stopperID + " is pausing trainID: " + trainID);

    }
    public void ReleaseTrainLights(int trainID)
    {
        GameManagerScript.instance.trainDictionary[trainID].isPaused = false;
        GameManagerScript.instance.log("trainID: " + trainID + " is being released");

    }

    public void MoveTrainToAnswerTrack(int trainID)
    {
        GameManagerScript.instance.trainDictionary[trainID].isOnAnswerStrip = true;

        GameManagerScript.instance.log("moving train: " + trainID + " to the answer track");
    }
    public void DestroyTrainLights(int trainID)
    {
        Destroy(GameManagerScript.instance.visualTrainDictionary[trainID]);
        GameManagerScript.instance.visualTrainDictionary.Remove(trainID);

        GameManagerScript.instance.trainDictionary.Remove(trainID);
        GameManagerScript.instance.log("Destroying trainID: " + trainID);

    }

    public void SetSpeedOfLights(int speed, bool isAnswerTrack)
    {

    }
    public void SendVoteLights(int senderID, int candidateAID, int candidateBID)
    {

    }
    public void DisplayTrainDontAnswer(int stopper, int senderID, int recieverID, string optionA, string optionB, string answer)
    {
        GameManagerScript.instance.log("displaying train that DOESN'T need to be answered\n" +
                "sender id: " + senderID + ", recieverID: " + recieverID + "\n" +
                 "optionA" + optionA + ", optionB" + optionB + "\n" +
                 "answer: " + answer
                );

        List<byte> command = new List<byte> { (byte)'g', getNumberAsByte(senderID), getNumberAsByte(recieverID) };
        foreach (char c in optionA)
        {
            command.Add((byte)c);
        }
        command.Add((byte)'\n');
        foreach (char c in optionB)
        {
            command.Add((byte)c);
        }
        command.Add((byte)'\n');
        foreach (char c in answer)
        {
            command.Add((byte)c);
        }
        command.Add((byte)'\n');
        command.Add(0);

        GameManagerScript.instance.portDictionary[stopper].Write(command.ToArray(), 0, (char)command.Count);


    }
    public void DisplayTrainThatNeedsAnswer(int stopper, int trainID, int senderID, int recieverID, string optionA, string optionB)
    {
        GameManagerScript.instance.log("displaying train that NEEDS to be answered\n" +
                "sender id: " + senderID + ", recieverID: " + recieverID + "\n" +
                 "optionA" + optionA + ", optionB" + optionB + "\n" +
                 "trainID: " + trainID
                );


        List<byte> command = new List<byte> { (byte)'h', getNumberAsByte(trainID), getNumberAsByte(senderID), getNumberAsByte(recieverID) };
        foreach (char c in optionA)
        {
            command.Add((byte)c);
        }
        command.Add((byte)'\n');
        foreach (char c in optionB)
        {
            command.Add((byte)c);
        }
        command.Add((byte)'\n');
        command.Add(0);
        GameManagerScript.instance.portDictionary[stopper].Write(command.ToArray(), 0, (char)command.Count);




    }
    public void SendKeywords(int stopper, int numKeywords, List<string> keywords)
    {
        List<byte> command = new List<byte> { (byte)'l', getNumberAsByte(numKeywords) };
        foreach (string s in keywords)
        {
            foreach (char c in s)
            {
                command.Add((byte)c);
            }
            command.Add((byte)'\n');
        }
        command.Add(0);
        GameManagerScript.instance.portDictionary[stopper].Write(command.ToArray(), 0, (char)command.Count);
    }
    public void SendCodewords(int stopper, int numCodewords, List<string> codewords)
    {
        List<byte> command = new List<byte> { (byte)'m', (byte)codewords.Count };
        foreach (string s in codewords)
        {
            foreach (char c in s)
            {
                command.Add((byte)c);
            }
            command.Add((byte)'\n');
        }
        command.Add(0);
        GameManagerScript.instance.portDictionary[stopper].Write(command.ToArray(), 0, (char)command.Count);
        //foreach (byte b in command)
        //{
        //    Debug.Log("ascii: " + b + ", char: " + (char)b);
        //}
    }

    public void NewGame()
    {

    }
}


 
