using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ArduiPin.IO
{
    public class Output
    {
        public const byte MSG_PARAM_LENGTH = 5;
        public const byte CARRIAGE_RETURN = 13;

        public byte Pin { get; set; }
        public byte Command { get; set; }
        public byte[] Param { get; set; }

        /// <summary>
        /// Default constructor
        /// </summary>
        public Output(byte pin, byte command)
        {
            Pin = pin;
            Command = command;
            Param = new byte[MSG_PARAM_LENGTH];
        }
    }
}
