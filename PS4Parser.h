/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#ifndef _ps4parser_h_
#define _ps4parser_h_

#include "Usb.h"
#include "controllerEnums.h"

/** Buttons on the controller */
const uint8_t PS4_BUTTONS[] PROGMEM = {
        UP, // UP
        RIGHT, // RIGHT
        DOWN, // DOWN
        LEFT, // LEFT

        0x0C, // SHARE
        0x0D, // OPTIONS
        0x0E, // L3
        0x0F, // R3

        0x0A, // L2
        0x0B, // R2
        0x08, // L1
        0x09, // R1

        0x07, // TRIANGLE
        0x06, // CIRCLE
        0x05, // CROSS
        0x04, // SQUARE

        0x10, // PS
        0x11, // TOUCHPAD
};

union PS4Buttons {
        struct {
                uint8_t dpad : 4;
                uint8_t square : 1;
                uint8_t cross : 1;
                uint8_t circle : 1;
                uint8_t triangle : 1;

                uint8_t l1 : 1;
                uint8_t r1 : 1;
                uint8_t l2 : 1;
                uint8_t r2 : 1;
                uint8_t share : 1;
                uint8_t options : 1;
                uint8_t l3 : 1;
                uint8_t r3 : 1;

                uint8_t ps : 1;
                uint8_t touchpad : 1;
                uint8_t reportCounter : 6;
        };
        uint8_t val[3];
};

struct touchpadXY {
        uint8_t dummy; // I can not figure out what this data is for, it seems to change randomly, maybe a timestamp?
        struct {
                uint8_t counter : 7; // Increments every time a finger is touching the touchpad
                uint8_t touching : 1; // The top bit is cleared if the finger is touching the touchpad
                uint16_t x : 12;
                uint16_t y : 12;
        } finger[2]; // 0 = first finger, 1 = second finger
};

struct PS4Data {
        /* Button and joystick values */
        uint8_t hatValue[4];
        PS4Buttons btn;
        uint8_t trigger[2];

        /* Gyro and accelerometer values */
        uint8_t dummy[3]; // First two looks random, while the third one might be some kind of status - it increments once in a while
        int16_t gyroY, gyroZ, gyroX;
        int16_t accX, accZ, accY;

        /* The rest is data for the touchpad */
        uint8_t dummy2[9]; // Byte 5 looks like some kind of status (maybe battery status), bit 1 of byte 8 is set every time a finger is moving around the touchpad
        touchpadXY xy[3]; // It looks like it sends out three coordinates each time, this might be because the microcontroller inside the PS4 controller is much faster than the Bluetooth connection.
                          // The last data is read from the last position in the array while the oldest measurement is from the first position.
                          // The first position will also keep it's value after the finger is released, while the other two will set them to zero.
                          // Note that if you read fast enough from the device, then only the first one will contain any data.

        // The last three bytes are always: 0x00, 0x80, 0x00
};

enum DPADEnum {
        DPAD_UP = 0x0,
        DPAD_UP_RIGHT = 0x1,
        DPAD_RIGHT = 0x2,
        DPAD_RIGHT_DOWN = 0x3,
        DPAD_DOWN = 0x4,
        DPAD_DOWN_LEFT = 0x5,
        DPAD_LEFT = 0x6,
        DPAD_LEFT_UP = 0x7,
        DPAD_OFF = 0x8,
};

/** This class parses all the data sent by the PS4 controller */
class PS4Parser {
public:
        /** Constructor for the PS4Parser class. */
        PS4Parser() {
                Reset();
        };

        /** @name PS4 Controller functions */
        /**
         * getButtonPress(ButtonEnum b) will return true as long as the button is held down.
         *
         * While getButtonClick(ButtonEnum b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(ButtonEnum b),
         * but if you need to drive a robot forward you would use getButtonPress(ButtonEnum b).
         * @param  b          ::ButtonEnum to read.
         * @return            getButtonPress(ButtonEnum b) will return a true as long as a button is held down, while getButtonClick(ButtonEnum b) will return true once for each button press.
         */
        bool getButtonPress(ButtonEnum b);
        bool getButtonClick(ButtonEnum b);
        /**@}*/
        /** @name PS4 Controller functions */
        /**
         * Used to get the analog value from button presses.
         * @param  b The ::ButtonEnum to read.
         * The supported buttons are:
         * ::UP, ::RIGHT, ::DOWN, ::LEFT, ::L1, ::L2, ::R1, ::R2,
         * ::TRIANGLE, ::CIRCLE, ::CROSS, ::SQUARE, and ::T.
         * @return   Analog value in the range of 0-255.
         */
        uint8_t getAnalogButton(ButtonEnum b);

        /**
         * Used to read the analog joystick.
         * @param  a ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return   Return the analog value in the range of 0-255.
         */
        uint8_t getAnalogHat(AnalogHatEnum a);

        /**
         * Get the x-coordinate of the touchpad. Position 0 is in the top left.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Returns the x-coordinate of the finger.
         */
        uint16_t getX(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].x;
        };

        /**
         * Get the y-coordinate of the touchpad. Position 0 is in the top left.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Returns the y-coordinate of the finger.
         */
        uint16_t getY(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].y;
        };

        /**
         * Returns whenever the user is toucing the touchpad.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Returns true if the specific finger is touching the touchpad.
         */
        bool isTouching(uint8_t finger = 0, uint8_t xyId = 0) {
                return !(ps4Data.xy[xyId].finger[finger].touching); // The bit is cleared when a finger is touching the touchpad
        };

        /**
         * This counter increments every time a finger touches the touchpad.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Return the value of the counter, note that it is only a 7-bit value.
         */
        uint8_t getTouchCounter(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].counter;
        };

        /**
         * Get the angle of the controller calculated using the accelerometer.
         * @param  a Either ::Pitch or ::Roll.
         * @return   Return the angle in the range of 0-360.
         */
        double getAngle(AngleEnum a) {
                if(a == Pitch)
                        return (atan2(ps4Data.accY, ps4Data.accZ) + PI) * RAD_TO_DEG;
                else
                        return (atan2(ps4Data.accX, ps4Data.accZ) + PI) * RAD_TO_DEG;
        };

        /**
         * Used to get the raw values from the 3-axis gyroscope and 3-axis accelerometer inside the PS4 controller.
         * @param  s The sensor to read.
         * @return   Returns the raw sensor reading.
         */
        int16_t getSensor(SensorEnum s) {
                switch(s) {
                        case gX:
                                return ps4Data.gyroX;
                        case gY:
                                return ps4Data.gyroY;
                        case gZ:
                                return ps4Data.gyroZ;
                        case aX:
                                return ps4Data.accX;
                        case aY:
                                return ps4Data.accY;
                        case aZ:
                                return ps4Data.accZ;
                        default:
                                return 0;
                }
        };
        /**@}*/

protected:
        /**
         * Used to parse data sent from the PS4 controller.
         * @param len Length of the data.
         * @param buf Pointer to the data buffer.
         */
        void Parse(uint8_t len, uint8_t *buf);

        /** Used to reset the different buffers to their default values */
        void Reset() {
                uint8_t i;
                for (i = 0; i < sizeof(ps4Data.hatValue); i++)
                        ps4Data.hatValue[i] = 127;
                for (i = 0; i < sizeof(PS4Buttons); i++) {
                        ps4Data.btn.val[i] = 0;
                        oldButtonState.val[i] = 0;
                }
                for (i = 0; i < sizeof(ps4Data.trigger); i++)
                        ps4Data.trigger[i] = 0;
                for (i = 0; i < sizeof(ps4Data.xy)/sizeof(ps4Data.xy[0]); i++) {
                        for (uint8_t j = 0; j < sizeof(ps4Data.xy[0].finger)/sizeof(ps4Data.xy[0].finger[0]); j++)
                                ps4Data.xy[i].finger[j].touching = 1; // The bit is cleared if the finger is touching the touchpad
                }

                ps4Data.btn.dpad = DPAD_OFF;
                oldButtonState.dpad = DPAD_OFF;
                buttonClickState.dpad = 0;
                oldDpad = 0;
        };

private:
        bool checkDpad(ButtonEnum b); // Used to check PS4 DPAD buttons

        PS4Data ps4Data;
        PS4Buttons oldButtonState, buttonClickState;
        uint8_t oldDpad;
};
#endif