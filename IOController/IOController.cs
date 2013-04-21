using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading.Tasks;
using ArduiPin.IO;
using vJoyInterfaceWrap;

namespace ArduiPin.IOController
{
    /// <summary>
    /// Class that controls all the INPUT/OUTPUT for the Arduino. It uses tasks for multithreading purpose since
    /// the output load to the Arduino can be pretty coarse.
    /// </summary>
    public class IOController
    {
        private enum cmdEnum {INPUT_CMD = 73, PIN_WRITE_CMD = 80, SHIFT_OUT_CMD = 83, PWM_WRITE_CMD = 87};  //I-P-S-W -> Used for the commands sent/received thru the serial link.

        
        //TODO: Change those arrays for a config file (possibility for 1 config per table + 1 default config)
        // *** I/O CONFIG ****  CHANGE THIS TO REPRESENT PIN USED BY YOUR ARDUNINO. BOTH ARDUINO AND THIS MUST BE THE SAME!!!!! <-------------------------------------------------
        private byte[] _inputDigitalPin = { 52, 50, 48, 46, 44, 42, 40 };   //The array of digital input pin used (buttons).
        private byte[] _inputAnalogPin   = { 6, 7 };                         //The array of analog input pin used (plunger, nudging, etc).
        private byte[] _outputDigitalPin = { 51, 49, 47, 45 };   //The array of digital output pin used.
        private byte[] _outputAnalogPin  = { 9, 10 };                        //The array of analog output pin used (PWM, RGB, etc).

        private List<Input> _input = new List<Input>();
        private List<Output> _outPut = new List<Output>();
        
        //The queues used by the tasks.
        private BlockingCollection<Input> _inputQueue = new BlockingCollection<Input>();
        private BlockingCollection<Output> _outputQueue = new BlockingCollection<Output>();
        Task _inputTask;
        Task _outputTask;

        static public vJoy _joystick;
        static public uint _joystickId = 1;


        //TODO: Change that hardcoding for a config.ini file.
        private static SerialPort _serialPort;
        const int BAUD_RATE = 9600;
        const String PORT = "COM3";
  

        /// <summary>
        /// Default constructor. Initialize the needed objects and tasks.
        /// </summary>
        public IOController()
        {
            //Serial port init.
            initSerialPort(PORT, BAUD_RATE);

            //Tasks init.
            _inputTask = new Task(() => InputProcessing(), TaskCreationOptions.LongRunning);
            _outputTask = new Task(() => OutputProcessing(), TaskCreationOptions.LongRunning);

            //vJoy object init.
            _joystick = new vJoy();
            _joystick.AcquireVJD(_joystickId);

            //Input init.
            InputInit();
        }

        /// <summary>
        /// Initialization of the serial port object.
        /// </summary>
        /// <param name="port">The name of the port: ie "COM3"</param>
        /// <param name="baudrate">The baud rate: ie 9600</param>
        private void initSerialPort(string port, int baudrate)
        {
            System.ComponentModel.IContainer components = new System.ComponentModel.Container();
            _serialPort = new SerialPort(components);
            _serialPort.PortName = port;
            _serialPort.BaudRate = baudrate;
            _serialPort.DataReceived += OnReceived;
        }

        /// <summary>
        /// Default Inialization of the input object array so they are binded to a vJoy button.
        /// TODO: Implement an override method with config file that load the pin used and which button they are binded.
        /// </summary>
        private void InputInit()
        {
            for (int i = 0; i < _inputDigitalPin.Length; i++)
            {
                _input.Add(new Input(_inputDigitalPin[i], (uint)i + 1));
            }
        }


        /// <summary>
        /// Start the controller operations
        /// </summary>
        public void Start()
        {
            OpenSerialPort();
            _inputTask.Start();
            _outputTask.Start();
        }
  
        /// <summary>
        /// Stops the controller.
        /// </summary>
        public void Stop()
        {
            //The CompleteAdding() method tells the attached tasks we are done adding items so they can stop looping for ever.
            _inputQueue.CompleteAdding();
            _outputQueue.CompleteAdding();
            _joystick.RelinquishVJD(_joystickId);
            //TODO: Add code to release all the pins on the arduino (turn off leds, etc.).
            _serialPort.Close();
        }
  
        /// <summary>
        /// Task that process the data sent by the Arduino and translate it into joystick input for Visual Pinball.
        /// Since it's a task, it run in is own thread.
        /// </summary>
        private void InputProcessing()
        {
            //Loops thru each msg in the queue and process/remove that msg. 
            //Blocks when no more msg in the queue and resumes when new msg enters.
            //So it's an infinite loops until the CompleteAdding() is called on the queue object.
            foreach (Input input in _inputQueue.GetConsumingEnumerable())     
            {
                _joystick.SetBtn(input.State, _joystickId, input.VJoyButton);
            }  
        }
  
        /// <summary>
        /// Task that process all the data sent by the Framework from PinMame and sends it as messages for the Arduino.
        /// Since it's a task, it run in is own thread.
        /// </summary>
        private void OutputProcessing()
        {
            //Loops thru each msg in the queue and process/remove that msg. 
            //Blocks when no more msg in the queue and resumes when new msg enters.
            //So it's an infinite loops until the CompleteAdding() is called on the queue object.
            foreach (Output output in _outputQueue.GetConsumingEnumerable())     
            {
                List<byte> data = new List<byte>();
                data.Add(output.Pin);
                data.Add(output.Command);
                data.AddRange(output.Param);
                data.Add(Output.CARRIAGE_RETURN);

                _serialPort.Write(data.ToArray(), 0, data.Count);
            }  
        }
                
        /// <summary>
        /// Method that add a message to the message queue.
        /// </summary>
        /// <param name="msg">The message to add to the queue</param>
        public void AddInputToQueue(Input input)
        {
            _inputQueue.Add(input);
        }

        /// <summary>
        /// Method that add a message to the message queue.
        /// </summary>
        /// <param name="msg">The message to add to the queue</param>
        public void AddOutputToQueue(Output output)
        {
            _outputQueue.Add(output);
        }


        /// <summary>
        /// Open the serial port
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OpenSerialPort()
        {
            if (!_serialPort.IsOpen)
            {
                try
                {
                    _serialPort.Open();
                }
                catch (Exception ex)
                {
                    //TODO: Write errors to a log file or something more intelligent than rethrowing the error.
                    throw (ex);
                }
            }
        }
        /// <summary>
        /// Event that receive and read the data from the Arduino. It runs in it's own thread by default (I think).
        /// For now, all the data the Arduino sends are inputs for the vJoy.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="c"></param>
        private void OnReceived(object sender, SerialDataReceivedEventArgs c)
        {
            byte[] data;
            string dataReceived;

            try
            {
                dataReceived = _serialPort.ReadLine().Trim();
                data = System.Text.Encoding.ASCII.GetBytes(dataReceived); //convert the string into an array of bytes

                //If the message data sent from the Arduino is OK then we send to the inputProcessing task thru it's queue.
                if (data[(int)Input.inputEnum.START_BYTE] == Input.MSG_START && data[(int)Input.inputEnum.CMD_BYTE] == (byte)cmdEnum.INPUT_CMD)
                {
                    Input input = _input.Find(item => item.Pin == data[(int)Input.inputEnum.PIN_BYTE]);
                    input.State = data[(int)Input.inputEnum.STATE_BYTE] == 0;
                    _inputQueue.Add(input);
                }
            }

            catch (Exception ex)
            {
                //TODO: Write errors to a log file or something more intelligent than rethrowing the error.
                throw (ex);
            }

        }
    }
}
