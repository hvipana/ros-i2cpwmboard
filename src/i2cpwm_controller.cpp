/**
 *
   \file
   \brief      controller for I2C interfaced 16 channel PWM boards with PCA9685 chip
   \author     Bradan Lane Studio <info@bradanlane.com>
   \copyright  Copyright (c) 2016, Bradan Lane Studio
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      - Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      - Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      - The name of Bradan Lane, Bradan Lane Studio nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL CLEARPATH ROBOTICS, INC. BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
  Please send comments, questions, or patches to info@bradanlane.com
 
*/

/**
\mainpage
 
 Controller for I2C interfaced 16 channel PWM boards with PCA9685 chip

 Bradan Lane Studio <info@bradanlane.com>

 Copyright (c) 2016, Bradan Lane Studio

 Licensed under GPLv3

--------


 __FILE STRUCTURE AND CODE__
 
  The file is broken into sections:
    - private properties (all private properties have a leading underscore)
    - private methods (all private methods have a leading underscore)
    - public methods subscribers for topics
    - public methods services
 
  The code is currently authored in C and should be rewritten as proper C++.
 
  This documentation refers to 'servo' and 'RC servo' but is equally applicable to any PWM or PPM controlled motor.

  All published services and topics use a one-based count syntax. For example, the first servo is '1' and the default board is '1'. 
  The hardware uses zero-based values. For example the first channel on the 16 channel 12-bit PWM board is '0' and the first I2C board is '0' with address 0x40.
  The switch from one-based to zero-based is done at the lowest level of this code. All public interactions should assume one-based values.
 
  __USING PWM WITH SERVOS__

  While the PCA9685 chip and related boards are called "PWM" for pulse width modulation, there use with servos
  is more accurately PPM for pulse position modulation.

  For standard 180 degree servos with a motion arc of ±90 degrees the pulse moves the servo arm to specifc position and holds it.
  For continuous motion servos, the pulse moves the servo at a specific speed. 

  The documentation will refer to positon, speed or position/speed. 
  These are interchangeable as the two terms are dependent on whether the servo is a fixed rotation servo or a continuous rotation servo and independent of the board itself.
 
  Analog RC servos are most often designed for 20ms pulses. This is achieved with a frequency of 50Hz.
  This software defaults to 50Hz. Use the set_pwm_frequncy() to change this frequency value. 
  It may be necessary or convenient to change the PWM frequency if using DC motors connected to PWM controllers. 
  It may also be convenient if using PWM to control LEDs.

  A commonly available board is the Adafruit 16 channel 12-bit PWM board or the similarly named HAT. There are similar boards as well as clones.
  All of these boards use the PCA9685 chip. Not all boards have been tested.

  The PWM boards drive LED and servos using pulse width modulation. The 12 bit interface means values are in the range of 0..4096.
  The pulse is defined as a start value and end value. When driving servos, the start point is typically 0 and the end point is the duration.

  FYI: If using more than one PWM board or when using a board with an I2C address other than the default 0x40, 
  the set_pwm_active_board() service must be used before using other services or topics from this package.
 

  __CONFIGURING SERVOS__

  The tolerance of the resistors in RC servos means each servo may have a slightly different center point.

  The servos_absolute() topic controls servos with absolute pulse start and stop values.
  This topic subscriber is not generally useful in robot operations, however it is conveninet for testing and for determining configuration values.
  Use this topic to determine the center position for a standard servo or the stop position for a continuous servo.

  Also use this topic to determine the range of each servo - 
  either the ±90 postion for a standard servo or 
  the max forward and reverse speed for a continuous rotation servo.

  __note:__ The centering value and range of servos is dependent on the pulse frequency. 
  If you change the frequency, you will need to determine new center and range values.
  
  Use the config_servos() service to set the center values and range values for servos.
  This enables servo motion to be generalized to a standard proportion ±1000. 
  Servo configuration is a prerequisite to using the servos_proportional() topic subscriber. 
  It is also a requirement before using the config_drive() servie API and its related servos_drive() topic subscriber.
  
  __ROBOT DRIVE MODE__

  In addition to absolute and proportional topics for controling servos, this package provides support for the geometry 'Twist' message.
  The servos_drive() topic handles the calculations to covert linear and angular data to servo drive data.
  This package supports three drive modes:
  
    -# 'ackerman' - single drive with external automobile style steering
    -# 'differential' - skid steer or track steer using a servo (or servos) for each of the left and right.
	A very simple algorythm is used.
	The result is the ability to perform zero-radius turns at a maximum of 50% speed
	and a full speed full turn (controller with max forward/reverse and max left/right is equal to the wheel base.
    -# 'mecanum' - requires a servo (or multiple servos) for each of left-front, right-front, left-rear, and right-rear wheels.
	Similar drive characteristics to differential drive with teh additional of lateral motion.
  
  The servos on the PWM board are assigned to their respective drive positons using the config_drive_mode() service.
  The applicable servos are assigned positions as follows:
  
    positon | ackerman | differential | mecanum
    --------|----------|--------------|--------
    position 1 corresponds to | drive | left | left-front
    position 2 corresponds to | | right | right-front
    position 3 corresponds to | | | left-rear
    position 4 corresponds to | | | right-rear
  
  The drive topic requires that use of both the config_servos() service and config_drive_mode() service before drive mode will result in expected behavior.

  All non-drive servos may still be operated with the proportion or absolute topics.

  The stop_servos() service is provided as convenience to stop all servos and place then is a powered off state.
  The stop service is useful as a safety operation.

  __TESTING__

  Basic testing is available from the command line. Start the I2C PWM node with `roslaunch i2cpwm_board i2cpwm_node.launch` and then proceed with 
  any of the _rostopic example_ and _rosservice example_ commands contained within the documentation for each service and topic subscriber.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>

#include <ros/ros.h>
#include <ros/console.h>

// messages used for any service with no parameters
#include <std_srvs/Empty.h>
// messages used for drive movement topic
#include <geometry_msgs/Twist.h>

// messages used for the absolute and proportional movement topics
#include "i2cpwm_board/Servo.h"
#include "i2cpwm_board/ServoArray.h"
// messages used for the servo setup service
#include "i2cpwm_board/ServoConfig.h"
#include "i2cpwm_board/ServoConfigArray.h"
// request/response of the servo setup service
#include "i2cpwm_board/ServosConfig.h"
// request/response of the drive mode service
#include "i2cpwm_board/DriveMode.h"
// request/response of the integer parameter services
#include "i2cpwm_board/IntValue.h"


/// @cond PRIVATE_NO_PUBLIC DOC

typedef struct _servo_config {
    int center;
    int range;
    int direction;
    int mode_pos;
} servo_config;

typedef struct _drive_mode {
	int mode;
	float scale;
} drive_mode;

enum drive_modes {
    MODE_UNDEFINED      = 0,
    MODE_ACKERMAN       = 1,
    MODE_DIFFERENTIAL   = 2,
    MODE_MECANUM        = 3,
    MODE_INVALID        = 4
};

enum drive_mode_positions {
    POSITION_UNDEFINED  = 0,
    POSITION_LEFTFRONT  = 1,
    POSITION_RIGHTFRONT = 2,
    POSITION_LEFTREAR   = 3,
    POSITION_RIGHTREAR  = 4,
    POSITION_INVALID    = 5
};

#define _BASE_ADDR   0x40
#ifndef _PI
#define _PI 3.14159265358979323846
#endif
#define _CONST(s) ((char*)(s))

enum pwm_regs {
  // Registers/etc.
  __MODE1              = 0x00,
  __MODE2              = 0x01,
  __SUBADR1            = 0x02,      // enable sub address 1 support
  __SUBADR2            = 0x03,      // enable sub address 2 support
  __SUBADR3            = 0x04,      // enable sub address 2 support
  __PRESCALE           = 0xFE,
  __CHANNEL_ON_L       = 0x06,
  __CHANNEL_ON_H       = 0x07,
  __CHANNEL_OFF_L      = 0x08,
  __CHANNEL_OFF_H      = 0x09,
  __ALL_CHANNELS_ON_L  = 0xFA,
  __ALL_CHANNELS_ON_H  = 0xFB,
  __ALL_CHANNELS_OFF_L = 0xFC,
  __ALL_CHANNELS_OFF_H = 0xFD,
  __RESTART            = 0x80,
  __SLEEP              = 0x10,      // enable low power mode
  __ALLCALL            = 0x01,
  __INVRT              = 0x10,      // invert the output control logic
  __OUTDRV             = 0x04
};

servo_config _servo_configs[62][16];        // we can support up to 62 boards (1..62), each with 16 PWM devices (1..16)
int _pwm_boards[62];                        // we can support up to 62 boards (1..62)
int _controller_io_handle;                  // linux file handle for I2C
int _active_board = 0;                      // used to determine if I2C SLAVE change is needed
int _pwm_frequency = 50;                    // frequency determines the size of a pulse width; higher numbers make RC servos buzz
drive_mode _active_drive;					// used when converting Twist geometry to PWM values and which servos are for motion

/// @endcond PRIVATE_NO_PUBLIC DOC




//* ------------------------------------------------------------------------------------------------------------------------------------
// local private methods
//* ------------------------------------------------------------------------------------------------------------------------------------

static int _abs (int v1)
{
	if (v1 < 0)
		return (0 - v1);
	return v1;
}

static int _min (int v1, int v2)
{
	if (v1 > v2)
		return v2;
	return v1;
}

static int _max (int v1, int v2)
{
	if (v1 < v2)
		return v2;
	return v1;
}

static int _absmax (int v1, int v2)
{
	int a1, a2;
	a1 = _abs(v1);
	a2 = _abs(v2);
	if (a1<a2)
		return v2;
	return v1;
}



/**
 \private method to smooth a speed value
 
 we calculate each speed using a cosine 'curve',  this results in the output curve 
 being shallow at 'stop', full forward, and full reverse and becoming 
 more aggressive in the middle or each direction

 @param speed an int value (±1000) indicating original speed
 @returns an integer value (±1000) smoothed for more gentle acceleration
 */
static int _smoothing (int speed)
{
	return ((cos(_PI*((1000 - speed) / 1000)) + 1) / 2) * 1000;
}
	

/**
 * \private method to set a common value for all PWM channels on the active board
 *
 *The pulse defined by start/stop will be active on all channels until any subsequent call changes it.
 *@param start an int value (0..4096) indicating when the pulse will go high sending power to each channel.
 *@param end an int value (0..4096) indicating when the pulse will go low stoping power to each channel.
 *Example: _set_pwm_interval_all (0, 108)   // set all servos with a pulse width of 105
 */
static void _set_pwm_interval_all (int start, int end)
{
    // the public API is ONE based and hardware is ZERO based
    if ((_active_board<1) || (_active_board>62)) {
        ROS_ERROR("Invalid active board number %d :: PWM board numbers must be between 1 and 62", _active_board);
        return;
    }
    int board = _active_board - 1;

    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __ALL_CHANNELS_ON_L, start & 0xFF))
        ROS_ERROR ("Error setting PWM start low byte for all servos on board %d", _active_board);
    if (0 >  i2c_smbus_write_byte_data (_controller_io_handle, __ALL_CHANNELS_ON_H, start  >> 8))
        ROS_ERROR ("Error setting PWM start high byte for all servos on board %d", _active_board);
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __ALL_CHANNELS_OFF_L, end & 0xFF))
        ROS_ERROR ("Error setting PWM end low byte for all servos on board %d", _active_board);
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __ALL_CHANNELS_OFF_H, end >> 8))
        ROS_ERROR ("Error setting PWM end high byte for all servos on board %d", _active_board);
};


/**
 * \private method to set a value for a PWM channel on the active board
 *
 *The pulse defined by start/stop will be active on the specified servo channel until any subsequent call changes it.
 *@param servo an int value (1..16) indicating which channel to change power
 *@param start an int value (0..4096) indicating when the pulse will go high sending power to each channel.
 *@param end an int value (0..4096) indicating when the pulse will go low stoping power to each channel.
 *Example: _set_pwm_interval (3, 0, 350)    // set servo #3 (fourth position on the hardware board) with a pulse of 350
 */
static void _set_pwm_interval (int servo, int start, int end)
{
	ROS_DEBUG("_set_pwm_interval enter");
    // the public API is ONE based and hardware is ZERO based
    if ((_active_board<1) || (_active_board>62)) {
        ROS_ERROR("Invalid active board number %d :: PWM board numbers must be between 1 and 62", _active_board);
        return;
    }
    int board = _active_board - 1;

	ROS_DEBUG("_set_pwm_interval board=%d", board);
    // the public API is ONE based and hardware is ZERO based
    if ((servo<1) || (servo>16)) {
        ROS_ERROR("Invalid servo number %d :: servo numbers must be between 1 and 16", servo);
        return;
    }
    int channel = servo - 1;
    
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __CHANNEL_ON_L+4*channel, start & 0xFF))
        ROS_ERROR ("Error setting PWM start low byte on servo %d on board %d", servo, _active_board);
    if (0 >  i2c_smbus_write_byte_data (_controller_io_handle, __CHANNEL_ON_H+4*channel, start  >> 8))
        ROS_ERROR ("Error setting PWM start high byte on servo %d on board %d", servo, _active_board);
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __CHANNEL_OFF_L+4*channel, end & 0xFF))
        ROS_ERROR ("Error setting PWM end low byte on servo %d on board %d", servo, _active_board);
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __CHANNEL_OFF_H+4*channel, end >> 8))
        ROS_ERROR ("Error setting PWM end high byte on servo %d on board %d", servo, _active_board);
};



/**
 * \private method to set a value for a PWM channel, based on a range of ±1000, on the active board
 *
 *The pulse defined by start/stop will be active on the specified servo channel until any subsequent call changes it.
 *@param servo an int value (1..16) indicating which channel to change power
 *@param value an int value (±1000) indicating when the size of the pulse for the channel.
 *Example: _set_pwm_interval (3, 0, 350)    // set servo #3 (fourth position on the hardware board) with a pulse of 350
 */
static void _set_pwm_interval_proportional (int servo, int value)
{
	if ((servo < 1) && (servo > 16)) {
		ROS_ERROR("Invalid servo number %d :: servo numbers must be between 1 and 16", servo);
		return;
	}
	if ((value < -1000) || (value > 1000)) {
		ROS_ERROR("Invalid proportion value %d :: proportion values must be between -1000 and 1000", value);
		return;
	}

	servo_config* configp = &(_servo_configs[_active_board-1][servo-1]);
	
	if ((configp->center < 0) ||(configp->range < 0)) {
		ROS_ERROR("Missing servo configuration for servo[%d]", servo);
		return;
	}

	int pos = (configp->direction * (((configp->range / 2) * value) / 1000)) + configp->center;
        
	if ((pos < 0) || (pos > 4096)) {
		ROS_ERROR("Invalid computed position servo[%d] = (direction(%d) * (((range(%d) / 2) * value(%d)) / 1000)) + %d = %d", servo, configp->direction, configp->range, value, configp->center, pos);
		return;
	}
	_set_pwm_interval (servo, 0, pos);
	ROS_INFO("servo[%d] = (direction(%d) * (((range(%d) / 2) * value(%d)) / 1000)) + %d = %d", servo, configp->direction, configp->range, value, configp->center, pos);
}


/**
 * \private method to set a pulse frequency
 *
 *The pulse defined by start/stop will be active on all channels until any subsequent call changes it.
 *@param frequency an int value (1..15000) indicating the pulse frequency where 50 is typical for RC servos
 *Example: _set_frequency (68)  // set the pulse frequency to 68Hz
 */
static void _set_pwm_frequency (int freq)
{
	ROS_DEBUG("_set_pwm_frequency enter");
    int prescale;
    char oldmode, newmode;
    int res;

	ROS_DEBUG("_set_pwm_frequency store freq=%d", freq);
    _pwm_frequency = freq;   // save to global
    
	ROS_DEBUG("_set_pwm_frequency prescale");
    float prescaleval = 25000000.0; // 25MHz
    prescaleval /= 4096.0;
    prescaleval /= (float)freq;
    prescaleval -= 1.0;
    //ROS_INFO("Estimated pre-scale: %f", prescaleval);
    prescale = floor(prescaleval + 0.5);
    // ROS_INFO("Final pre-scale: %d", prescale);


	ROS_INFO("Setting PWM frequency to %d Hz", freq);

    nanosleep ((const struct timespec[]){{1, 000000L}}, NULL); 


	ROS_DEBUG("_set_pwm_frequency read old mode");
    oldmode = i2c_smbus_read_byte_data (_controller_io_handle, __MODE1);
    newmode = (oldmode & 0x7F) | 0x10; // sleep

	ROS_DEBUG("_set_pwm_frequency power down to sleep");
    if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __MODE1, newmode)) // go to sleep
        ROS_ERROR("Unable to set PWM controller to sleep mode"); 

	ROS_DEBUG("_set_pwm_frequency write prescale=%d", prescale);
    if (0 >  i2c_smbus_write_byte_data(_controller_io_handle, __PRESCALE, (int)(floor(prescale))))
        ROS_ERROR("Unable to set PWM controller prescale"); 

	ROS_DEBUG("_set_pwm_frequency write old mode");
    if (0 > i2c_smbus_write_byte_data(_controller_io_handle, __MODE1, oldmode))
        ROS_ERROR("Unable to set PWM controller to active mode"); 

    nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);   //sleep 5microsec,

	ROS_DEBUG("_set_pwm_frequency write old power setting");
    if (0 > i2c_smbus_write_byte_data(_controller_io_handle, __MODE1, oldmode | 0x80))
        ROS_ERROR("Unable to restore PWM controller to active mode");

	ROS_DEBUG("_set_pwm_frequency exit");
};


/**
 * \private method to set the active board
 *
 *@param board an int value (1..62) indicating which board to activate for subsequent service and topic subscription activity where 1 coresponds to the default board address of 0x40 and value increment up
 *Example: _set_active_board (68)   // set the pulse frequency to 68Hz
 */
static void _set_active_board (int board)
{
	char mode1res;

	ROS_DEBUG("_set_active_board enter");
	if ((board<1) || (board>62)) {
        ROS_ERROR("Invalid board number %d :: board numbers must be between 1 and 62", board);
        return;
    }
    if (_active_board != board) {
        _active_board = board;   // save to global
        
        // the public API is ONE based and hardware is ZERO based
        board--;
        
		ROS_DEBUG("_set_active_board active=%d board=%d addr=0x%X", _active_board, board, (_BASE_ADDR+(board)));
        if (0 > ioctl (_controller_io_handle, I2C_SLAVE, (_BASE_ADDR+(board)))) {
            ROS_FATAL ("Failed to acquire bus access and/or talk to I2C slave at address 0x%02X", (_BASE_ADDR+board));
            return; /* exit(1) */   /* additional ERROR HANDLING information is available with 'errno' */
        }

        if (_pwm_boards[board]<0) {
            _pwm_boards[board] = 1;

			ROS_DEBUG("_set_active_board new board");
            /* this is guess but I believe the following needs to be done on each board only once */

            if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __MODE2, __OUTDRV))
                ROS_ERROR ("Failed to enable PWM outputs for totem-pole structure");

            if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __MODE1, __ALLCALL))
                ROS_ERROR ("Failed to enable ALLCALL for PWM channels");

            nanosleep ((const struct timespec[]){{0, 5000000L}}, NULL);   //sleep 5microsec, wait for osci


            mode1res = i2c_smbus_read_byte_data (_controller_io_handle, __MODE1);
            mode1res = mode1res & ~__SLEEP; //                 # wake up (reset sleep)

            if (0 > i2c_smbus_write_byte_data (_controller_io_handle, __MODE1, mode1res))
                ROS_ERROR ("Failed to recover from low power mode");

            nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);   //sleep 5microsec, wait for osci

            // the first time we activate a board, we mark it and set all of its servo channels to 0
            _set_pwm_interval_all (0, 0);
        }
    }
}


/**
 \private method to initialize private internal data structures at startup

@param devicename a string value indicating the linux I2C device

Example: _init ("/dev/i2c-1");  // default I2C device on RPi2 and RPi3 = "/dev/i2c-1"
 */
static void _init (char* filename)
{
    int res;
    char mode1res;
    int i,j;

    /* initialize all of the global data objects */
    
    for (j=0; j<64;j++) {
        for (i=0; i<16;i++) {
            // these values have not useful meaning
            _servo_configs[j][i].center = -1;
            _servo_configs[j][i].range = -1;
            _servo_configs[j][i].direction = 1;
            _servo_configs[j][i].mode_pos = -1;
        }
    }
    for (j=0; j<64;j++)
        _pwm_boards[j] = -1;
    _active_board = -1;

	_active_drive.mode = MODE_UNDEFINED;
	_active_drive.scale = 1.0;
	
	
    if ((_controller_io_handle = open (filename, O_RDWR)) < 0) {
        ROS_FATAL ("Failed to open I2C bus %s", filename);
        return; /* exit(1) */   /* additional ERROR HANDLING information is available with 'errno' */
    }
}




// ------------------------------------------------------------------------------------------------------------------------------------
/**
\defgroup Topics Topics with subscribers provided by this package
@{ */
// ------------------------------------------------------------------------------------------------------------------------------------

/**
\brief subscriber topic to move servos in a physical position

subscriber for the servos_absolute topic which processes one or more servos and sets their physical pulse value.

@param msg  a 'ServoArray' message (array of one or more 'Servo') where the servo:value is the pulse position/speed

__rostopic example:__ `rostopic pub -1 /servos_absolute i2cpwm_board/ServoArray "{servos:[{servo: 1, value: 200}, {servo: 2, value: 200}]}"`

__i2cpwm_board::ServoArray__
\include "ServoArray.msg"
__i2cpwm_board::Servo__
\include "Servo.msg"
 */
void servos_absolute (const i2cpwm_board::ServoArray::ConstPtr& msg)
{
    /* this subscription works on the active_board */
    
    for(std::vector<i2cpwm_board::Servo>::const_iterator sp = msg->servos.begin(); sp != msg->servos.end(); ++sp) {
        int servo = sp->servo;
        int value = sp->value;
		ROS_DEBUG("servos_absolute processing servo=%d value=%d", servo, value);
        if ((servo < 1) && (servo > 16)) {
            ROS_ERROR("Invalid servo number %d :: servo numbers must be between 1 and 16", servo);
            continue;
        }
        if ((value < 0) || (value > 4096)) {
            ROS_ERROR("Invalid PWM value %d :: PWM values must be between 0 and 4096", value);
            continue;
        }
        _set_pwm_interval (servo, 0, value);
        ROS_INFO("servo[%d] = %d", servo, value);
    }
}


/**
\brief subscriber topic to move servos in the range of ±1000

A subscriber for the controling servos using proportional valued. 
This topic processes one or more servos and sets their physical pulse value based on
each servos physical range proportional to a range of ±1000. 

This topic requires the use of the config_servos() service.
The center, direction, and range are used to convert the proportional to a physical position value.

@param msg  a 'ServoArray' message (array of one or more 'Servo') where the servo:value is a relative position/speed

__rostopic example:__ `rostopic pub -1 /servos_proportion i2cpwm_board/ServoArray "{servos:[{servo: 1, value: -100}]}"`

__i2cpwm_board::ServoArray Message__
\include "ServoArray.msg"
__i2cpwm_board::Servo Message__
\include "Servo.msg"
 */
void servos_proportional (const i2cpwm_board::ServoArray::ConstPtr& msg)
{
    /* this subscription works on the active_board */

    for(std::vector<i2cpwm_board::Servo>::const_iterator sp = msg->servos.begin(); sp != msg->servos.end(); ++sp) {
        int servo = sp->servo;
        int value = sp->value;
		_set_pwm_interval_proportional (servo, value);
    }
}




/**
\brief subscriber topic to move servos based on a drive mode

A subscriber for the servos_drive topic which processes a geometry Twist message and translates it to 
proportional pulse values for the pre-specified servos. This requires the use of both the drive_mode() and servos_config() services.


The center, direction, and range are used to convert the proportional to a physical position value.

@param msg  a geometry Twist message

__geometry_msgs::Twist__
\include "Twist.msg"
__geometry_msgs::Vector3__
\include "Vector3.msg"
 */
void servos_drive (const geometry_msgs::Twist::ConstPtr& msg)
{
	/* this subscription works on the active_board */

	int i;
	int swap;
	int temp_x, temp_y, temp_r, dir_x, dir_y, dir_r;
	int speed[4];
	
	/* msg is a pointer to a Twist message: msg->linear and msg->angular each of which have members .x .y .z */
	/* the subscriber uses the maths from: http://robotsforroboticists.com/drive-kinematics/ */	

	ROS_INFO("[%5.2f %5.2f %5.2f] [%5.2f %5.2f %5.2f]", 
			 msg->linear.x, msg->linear.y, msg->linear.z, msg->angular.x, msg->angular.y, msg->angular.z);

	if (_active_drive.mode == MODE_UNDEFINED) {
		ROS_ERROR("drive mode not set");
		return;
	}
	if ((_active_drive.mode < MODE_UNDEFINED) || (_active_drive.mode >= MODE_INVALID)) {
		ROS_ERROR("unrecognized drive mode set %d", _active_drive.mode);
		return;
	}

	dir_x = ((msg->linear.x > 0)  ? 1 : -1);
	dir_y = ((msg->linear.y > 0)  ? 1 : -1);
	dir_r = ((msg->angular.z > 0) ? 1 : -1);

	temp_x = _abs(msg->linear.x);
	temp_y = _abs(msg->linear.x);
	temp_r = _abs(msg->angular.z);
		
	temp_x = _smoothing (temp_x);
	temp_y = _smoothing (temp_y);
	temp_r = _smoothing (temp_r) / 2;


	switch (_active_drive.mode) {
	case MODE_ACKERMAN:
		/*
		  with ackerman drive, steering is handled by a separate servo
		  we drive assigned servos exclusively by the linear.x
		*/
		speed[0] = temp_x * dir_x;
		ROS_INFO("ackerman drive mode speed=%d", speed[0]);
		break;
	case MODE_DIFFERENTIAL:
		/*
		  with differential drive, steering is handled by the relative speed of left and right servos
		  we drive assigned servos by mixing linear.x and angular.z with a very simple formula
		*/
		speed[0] = temp_x * dir_x;
		speed[1] = (temp_x - temp_r) * dir_x;
		if (dir_r < 0) {
			swap = speed[0];
			speed[0] = speed[1];
			speed[1] - swap;
		}
		ROS_INFO("differential drive mode speed left=%d right=%d", speed[0], speed[1]);
		break;
	case MODE_MECANUM:
		/*
		  with mecanum drive, steering is handled by the relative speed of left and right servos
		  with mecanum drive, lateral motion is handled by the rotation of front and rear servos
		  we drive assigned servos by mixing linear.x and angular.z  and linear.y
		*/
		speed[0] = speed[2] = temp_x * dir_x;
		speed[1] = speed[3] = (temp_x - temp_r) * dir_x;
		speed[0] -= temp_y * dir_y;
		speed[3] -= temp_y * dir_y;
		speed[1] += temp_y * dir_y;
		speed[2] += temp_y * dir_y;
		speed[0] = _absmax (speed[0], 1000);
		speed[1] = _absmax (speed[1], 1000);
		speed[2] = _absmax (speed[2], 1000);
		speed[3] = _absmax (speed[3], 1000);
		ROS_INFO("mecanum drive mode speed leftfront=%d rightfront=%d leftrear=%d rightreer=%d", speed[0], speed[1], speed[2], speed[3]);
		break;
	default:
		break;
	}
		
	/* find all drive servos and set their new speed */
	for (i=0; i<16; i++) {
		if (_servo_configs[_active_board-1][i].mode_pos == POSITION_LEFTFRONT)
			_set_pwm_interval_proportional (i+1, speed[0]);
		if (_servo_configs[_active_board-1][i].mode_pos == POSITION_RIGHTFRONT)
			_set_pwm_interval_proportional (i+1, speed[1]);
		if (_servo_configs[_active_board-1][i].mode_pos == POSITION_LEFTREAR)
			_set_pwm_interval_proportional (i+1, speed[3]);
		if (_servo_configs[_active_board-1][i].mode_pos == POSITION_RIGHTREAR)
			_set_pwm_interval_proportional (i+1, speed[2]);
	}
}


// ------------------------------------------------------------------------------------------------------------------------------------
/**@}*/
/**
\defgroup Services Services interfaces provided by this package
@{ */
// services
// ------------------------------------------------------------------------------------------------------------------------------------

/**
   \brief service to set set PWM frequency

   The PWM boards drive LED and servos using pulse width modulation. The 12 bit interface means values are 0..4096.
   The size of the minimum width is determined by the frequency. is service is needed when using a board configured other than with the default I2C address and when using multiple boards.

   If using the pwm_active_board service, it must be used before using other services or topics from this package.

   @param req an Int16 value for which board should be active(1..62).
   @param res the return value
   @returns an integer non-zero if an error occured

   Example request: "{}"

__i2cpwm_board::IntValue__
\include "IntValue.srv"
 */
bool set_pwm_frequency (i2cpwm_board::IntValue::Request &req, i2cpwm_board::IntValue::Response &res)
{
	int freq;
	freq = req.value;
	if ((freq<12) || (freq>1024)) {
		ROS_ERROR("Invalid PWM frequency %d :: PWM frequencies should be between 12 and 1024", freq);
		freq = 50;	// most analog RC servos are designed for 20ms pulses.
		res.error = freq;
	}
	_set_pwm_frequency (freq);	// I think we must reset frequency when we change boards
	res.error = freq;
	return true;
}

/**
   \brief service to set active PWM board

   A service to set or switch which PWM board is active. By default, board #0 is active and uses I2C 0x40 address.
   the PWM boards which use the PCA9685 chip support addresses in the range 0x40 to 0x7E.

   This service is needed when using a board configured other than with the default I2C address and when using multiple boards.
   If using the pwm_active_board service, it must be used before using other services or topics from this package.

   @param req an Int16 value for which board should be active(1..62).
   @param res the return value
   @returns an integer non-zero if an error occured

   Example request: "{}"

__i2cpwm_board::IntValue__
\include "IntValue.srv"
 */
bool set_pwm_active_board (i2cpwm_board::IntValue::Request &req, i2cpwm_board::IntValue::Response &res)
{
	int board;
	res.error = 0;
	board = req.value;
	if ((board<1) || (board>62)) {
		ROS_ERROR("Invalid board number %d :: PWM board numbers must be between 1 and 62", board);
		board = 1;
	}
	_set_active_board (board);
	_set_pwm_frequency (_pwm_frequency);	// I think we must reset frequency when we change boards
	res.error = board;
	return true;
}

/**
   \brief store configuration data for servos on the active board

   A service to set each servo's center value, direction of rotation (1 for forward and -1 for reverse motion), 
   the center or nul value, range, and direction of one or more servos. 
   and range between full left and right or maximun forward and backward speed.

   Setting these data are required before sending messages to the servos_proportional() topic as well as the servos_drive() topic. 

   If more than one PWM board is present, the set_pwm_active_board() service is used to switch between boards prior to configuring servos.

   @param req an array of 'ServoConfig' which consists of a servo number (one based), center(0..4096), range(0..4096), and direction (±1).
   @param res the return value
   @returns an integer non-zero if an error occured

__rosservice example:__ `rosservice call /config_servos "servos: [{servo: 1, center: 336, range: 96, direction: 1}]"`

__i2cpwm_board::ServosConfig___
\include "ServosConfig.srv"

 */
bool config_servos (i2cpwm_board::ServosConfig::Request &req, i2cpwm_board::ServosConfig::Response &res)
{
	/* this service works on the active_board */
	int i;
	
	res.error = 0;

	if ((_active_board<1) || (_active_board>62)) {
		ROS_ERROR("Invalid board number %d :: PWM board numbers must be between 1 and 62", _active_board);
		res.error = -1;
		return true;
	}

	for (i=0;i<req.servos.size();i++) {
		int servo = req.servos[i].servo;
		int center = req.servos[i].center;
		int range = req.servos[i].range;
		int direction = req.servos[i].direction;

		if ((servo < 1) || (servo > 16)) {
			ROS_ERROR("Invalid servo number %d :: servo numbers must be between 1 and 16", servo);
			res.error = servo; /* this needs to be more specific and indicate a bad server ID was provided */
			continue;
		}
		if ((center < 0) || (center > 4096)) {
			ROS_ERROR("Invalid center value %d :: center values must be between 0 and 4096", center);
			continue;
		}
		if ((center < 0) || (center > 4096)) {
			ROS_ERROR("Invalid range value %d :: range values must be between 0 and 4096", range);
			continue;
		}
		if (((center - (range/2)) < 0) || (((range/2) + center) > 4096)) {
			ROS_ERROR("Invalid range center combination %d ± %d :: range/2 ± center must be between 0 and 4096", center, (range/2));
			continue;
		}

		_servo_configs[_active_board-1][servo-1].center = center;
		_servo_configs[_active_board-1][servo-1].range = range;
		_servo_configs[_active_board-1][servo-1].direction = direction;
		_servo_configs[_active_board-1][servo-1].mode_pos = POSITION_UNDEFINED;
	}
	return true;
}


/**
   \brief set drive mode and drive servos

   A service to set the desired drive mode. It must be called  before messages are handled by the servos_drive() topic.
   Setting these data are required before sending messages to the servos_proportional() topic. 

   The drive mode consists of a string value for the type of drive desired: ackerman, differential, or mecanum.
   For each mode, the drive servos must be specified.

   @param req is DriveMode with a string and a ServoArray' which consists of a servo number (1..16) and a value (1..4) indicating the drive sequence
   @param res the return value
   @returns an integer non-zero if an error occured

   The mode string is one of the following drive systems:
    -# 'ackerman' - (automobile steering) requires minimum of one servo for drive and uses some other servo for stearing..
	-# 'differential' - requires multiples of two servos, designated as left and right.
	-# 'mecanum' - requires multiples of four servos, designated as left-front, right-front, left-rear, and right-rear.

	The servo message is used for indicating which servos are used for the drive system.
	The message consists of 'servo' number, and data 'value'.

	The 'value' field indicates the positon the corresponding servo within the drive system.
	The applicable servos are assigned positions as follows:
  
    positon | ackerman | differential | mecanum
    --------|----------|--------------|--------
    position 1 corresponds to | drive | left | left-front
    position 2 corresponds to | | right | right-front
    position 3 corresponds to | | | left-rear
    position 4 corresponds to | | | right-rear
  
	Example request: "{}"

	__i2cpwm_board::DriveMode__
	\include "DriveMode.srv"
 */
bool config_drive_mode (i2cpwm_board::DriveMode::Request &req, i2cpwm_board::DriveMode::Response &res)
{
	res.error = 0;

	int i;
	int mode = MODE_UNDEFINED;
	float scale;

	// assumes the parameter was provided in the proper case
	if 		(0 == strcmp (req.mode.c_str(), _CONST("ackerman")))
		mode = MODE_ACKERMAN;
	else if (0 == strcmp (req.mode.c_str(), _CONST("differential")))
		mode = MODE_DIFFERENTIAL;
	else if (0 == strcmp (req.mode.c_str(), _CONST("mecanum")))
		mode = MODE_MECANUM;
	else {
		mode = MODE_INVALID;
		ROS_ERROR("Invalid drive mode %s :: drive mode must be one of ackerman, differential, or mecanum", req.mode.c_str());
		res.error = -1;
		return true;
	}

	_active_drive.mode = mode;
	scale = req.scale;
	if (scale <= 0.0) {
		ROS_ERROR("Invalid scale %f :: the scalar for Twist messages must be greater than 0.0", scale);
		res.error = -1;
		return true;
	}

	_active_drive.mode = mode;
	_active_drive.scale = scale;

	for (i=0;i<req.servos.size();i++) {
		int servo = req.servos[i].servo;
		int position = req.servos[i].value;

		if ((_active_board < 1) || (_active_board > 62)) {
			ROS_ERROR("Invalid board number %d :: board numbers must be between 1 and 62", _active_board);
			res.error = servo; /* this needs to be more specific and indicate a bad server ID was provided */
			continue;
		}
		if ((servo < 1) || (servo > 16)) {
			ROS_ERROR("Invalid servo number %d :: servo numbers must be between 1 and 16", servo);
			res.error = servo; /* this needs to be more specific and indicate a bad server ID was provided */
			continue;
		}
		if ((position < POSITION_UNDEFINED) || (position > POSITION_LEFTREAR)) {
			ROS_ERROR("Invalid drive mode position %d :: positions are 0 = non-drive, 1 = left front, 2 = right front, 3 = left rear, and 4 = right rear", position);
			continue;
		}

		_servo_configs[_active_board-1][servo-1].mode_pos = position;
	}

	return true;
}


/**
   \brief service to stop all servos on all boards

   A service to stop all of the servos on all of the PWM boards and set their power state to off / coast.

   This is different from setting each servo to its center value. A centered servo is still under power and it's in a brake state.

   @param req is empty
   @param res is empty
   @returns nothing

   __rosservice example:__ `rosservice call /stop_servos`

 */
bool stop_servos (std_srvs::Empty::Request& req, std_srvs::Empty::Response& res)
{
	int save_active = _active_board;
	int i;

	for (i=0; i<62; i++) {
		if (_pwm_boards[i] > 0) {
			_set_active_board (i+1);	// API is ONE based
			_set_pwm_interval_all (0, 0);
		}
	}
	_active_board = save_active;
	return true;
}





// ------------------------------------------------------------------------------------------------------------------------------------
/**@}*/
// main
// ------------------------------------------------------------------------------------------------------------------------------------

int main (int argc, char **argv)
{

	_controller_io_handle = 0;
	_pwm_frequency = 50;		// set the initial pulse frequency to 50 Hz which is standard for RC servos

	
	_init (_CONST("/dev/i2c-1"));	// default I2C device on RPi2 and RPi3 = "/dev/i2c-1";

	_set_active_board (1);

	_set_pwm_frequency (50);


	ros::init (argc, argv, "i2cpwm_controller");

	ros::NodeHandle n;


	ros::ServiceServer board_srv =		n.advertiseService 	("set_pwm_active_board",		set_pwm_active_board);
	ros::ServiceServer freq_srv =		n.advertiseService 	("set_pwm_frequency", 			set_pwm_frequency);
	ros::ServiceServer stop_srv =		n.advertiseService 	("stop_servos", 				stop_servos);		// the 'stop' service can be used at any time
	ros::ServiceServer config_srv =		n.advertiseService 	("config_servos", 				config_servos);		// 'config' will setup the necessary properties of continuous servos and is helpful for standard servos
	ros::ServiceServer mode_srv =		n.advertiseService 	("config_drive_mode",			config_drive_mode);	// 'mode' specifies which servos are used for motion and which behavior will be applied when driving

	ros::Subscriber abs_sub = 			n.subscribe 		("servos_absolute", 1000, 		servos_absolute);		// the 'absolute' topic will be used for standard servo motion and testing of continuous servos
	ros::Subscriber rel_sub = 			n.subscribe 		("servos_proportional", 1000, 	servos_proportional);	// the 'proportion' topic will be used for standard servos and continuous rotation aka drive servos
	ros::Subscriber drive_sub = 		n.subscribe 		("servos_drive", 1000, 			servos_drive);			// the 'drive' topic will be used for continuous rotation aka drive servos controlled by Twist messages
	
	ros::spin();

	close(_controller_io_handle);

  return 0;
}

