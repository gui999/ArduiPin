using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ArduiPin.IO
{
    public class Input
    {
        public const byte MSG_START = 2;   //The byte that represents the ASCII "Start of text" char. Used as a "new message" delimiter.. since we start a new text! (there's a concept there)
        public enum inputEnum { START_BYTE, CMD_BYTE, PIN_BYTE, STATE_BYTE };   //Indexes for the input message structure.
        public byte Pin { get; private set; }
        public uint VJoyButton { get; private set; }
        public bool State { get; set; }

        /// <summary>
        /// Default constructor
        /// </summary>
        public Input(byte pin, uint vJoyButton)
        {
            Pin = pin;
            VJoyButton = vJoyButton;
            State = false;
        }
    }
}
