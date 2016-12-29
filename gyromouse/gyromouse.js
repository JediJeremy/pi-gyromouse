require('es6-shim');
//var i2c = require('i2c');

// useful math functions
function vec2_add(a,b) { return [a[0]+b[0], a[1]+b[1]] }
function vec3_add(a,b) { return [a[0]+b[0], a[1]+b[1], a[2]+b[2]] }
function vec2_sub(a,b) { return [a[0]-b[0], a[1]-b[1]] }
function vec3_sub(a,b) { return [a[0]-b[0], a[1]-b[1], a[2]-b[2]] }
function vec2_mul(a,b) { return [a[0]*b[0], a[1]*b[1]] }
function vec3_mul(a,b) { return [a[0]*b[0], a[1]*b[1], a[2]*b[2]] }
function vec3_length(a) { return Math.sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2]) }


var uinput = require('uinput');

var mouse_message, mouse_click; // defined shortly

uinput.setup({
	EV_KEY : [ uinput.BTN_LEFT  ],
	EV_REL : [ uinput.REL_X, uinput.REL_Y, uinput.REL_WHEEL ]
}, function(err, stream) {
	if (err) { throw(err); }
	
	var absmax = new Array(uinput.ABS_CNT).fill(0);
    absmax[uinput.ABS_X] = 1024;
    absmax[uinput.ABS_Y] = 1024;

	var create_options = {
		name : 'gyromouse',
		id : {
			bustype : uinput.BUS_VIRTUAL,
			vendor : 0x1,
			product : 0x1,
			version : 1
		},
        absmax : absmax
	};
	
	var uinput_inflight = false;
	
	uinput.create(stream, create_options, function(err) {
		if (err) { throw(err); }
		// now we can publish the messager functions
		mouse_message = function(v) {
			if(uinput_inflight) return;
			uinput_inflight = true;
			uinput.send_event(stream, uinput.EV_REL, uinput.REL_X, v[0], function() {
				uinput.send_event(stream, uinput.EV_REL, uinput.REL_Y, v[1], function() {
					// complete.
					uinput_inflight = false;
				} );				
			});
		}
		
		mouse_click = function(button,msg) {
			// console.log('click',button,msg);
			var btn_code;
			switch(button) {
				case 1: case 'right': btn_code = uinput.BTN_RIGHT; break;
				case 2: case 'middle': btn_code = uinput.BTN_MIDDLE; break;
				case 0: case 'left': default: btn_code = uinput.BTN_LEFT;
			}
			if(uinput_inflight) return;
			uinput_inflight = true;
			uinput.key_event(stream, uinput.BTN_LEFT, function(err) {
				uinput_inflight = false;
				if(err) throw(err);
			});
		}
	});
});

/*
var robot = require("robotjs");
//Speed up the mouse.
robot.setMouseDelay(1);

mouse_message = function(v) {
	var p = robot.getMousePos();
	robot.moveMouse(mouse.x + v[0], mouse.y + v[1]);
}

mouse_click = function() {
	robot.mouseClick();
}
*/

// GPIO
var Gpio = require('pigpio').Gpio;

function debounce(button,time) {
	var bounce_time = 0;
	var last_level = 0;
	// return an interrupt handler
	return function(level) {
		var now = (+ new Date);
		// console.log(level,now,bounce_time);
		if(level!==last_level) {
			if( (level==0) && (bounce_time < now)) mouse_click(button, level);
			bounce_time = now + time;
			last_level = level;
		}
	}
}
// left button on pin 32 - gpio 12
var button0 = new Gpio(12, {
    mode: Gpio.INPUT,
    pullUpDown: Gpio.PUD_UP,
    edge: Gpio.EITHER_EDGE
});
button0.on('interrupt', debounce(0, 100) );

// right button on pin 36 - gpio 16
var button1 = new Gpio(16, {
    mode: Gpio.INPUT,
    pullUpDown: Gpio.PUD_UP,
    edge: Gpio.EITHER_EDGE
});
button1.on('interrupt', debounce(1, 100));


var i2c_address = 0x68;

// var wire = new i2c(i2c_address, {device: '/dev/i2c-1'}); // point to your i2c address, debug provides REPL interface 

/* wire.scan(function(err, data) {
  // result contains an array of addresses 
	console.log(data);
}); */

// MP6050 i2c constants
// from: https://github.com/jrowberg/i2cdevlib/blob/master/MSP430/MPU6050/MPU6050.h

var MPU6040_ADDRESS_AD0_LOW     = 0x68; // address pin low (GND); default for InvenSense evaluation board
var MPU6040_ADDRESS_AD0_HIGH    = 0x69; // address pin high (VCC)
var MPU6040_DEFAULT_ADDRESS     = 0x68; // ADDRESS_AD0_LOW

var MPU6040_ADDRESS_COMPASS		= 0x0c;

var MPU6040_RA_XG_OFFS_TC       = 0x00; //[7] PWR_MODE; [6=1] XG_OFFS_TC; [0] OTP_BNK_VLD
var MPU6040_RA_YG_OFFS_TC       = 0x01; //[7] PWR_MODE; [6=1] YG_OFFS_TC; [0] OTP_BNK_VLD
var MPU6040_RA_ZG_OFFS_TC       = 0x02; //[7] PWR_MODE; [6=1] ZG_OFFS_TC; [0] OTP_BNK_VLD
var MPU6040_RA_X_FINE_GAIN      = 0x03; //[7=0] X_FINE_GAIN
var MPU6040_RA_Y_FINE_GAIN      = 0x04; //[7=0] Y_FINE_GAIN
var MPU6040_RA_Z_FINE_GAIN      = 0x05; //[7=0] Z_FINE_GAIN
var MPU6040_RA_XA_OFFS_H        = 0x06; //[15=0] XA_OFFS
var MPU6040_RA_XA_OFFS_L_TC     = 0x07;
var MPU6040_RA_YA_OFFS_H        = 0x08; //[15=0] YA_OFFS
var MPU6040_RA_YA_OFFS_L_TC     = 0x09;
var MPU6040_RA_ZA_OFFS_H        = 0x0A; //[15=0] ZA_OFFS
var MPU6040_RA_ZA_OFFS_L_TC     = 0x0B;
var MPU6040_RA_XG_OFFS_USRH     = 0x13; //[15=0] XG_OFFS_USR
var MPU6040_RA_XG_OFFS_USRL     = 0x14;
var MPU6040_RA_YG_OFFS_USRH     = 0x15; //[15=0] YG_OFFS_USR
var MPU6040_RA_YG_OFFS_USRL     = 0x16;
var MPU6040_RA_ZG_OFFS_USRH     = 0x17; //[15=0] ZG_OFFS_USR
var MPU6040_RA_ZG_OFFS_USRL     = 0x18;
var MPU6040_RA_SMPLRT_DIV       = 0x19;
var MPU6040_RA_CONFIG           = 0x1A;
var MPU6040_RA_GYRO_CONFIG      = 0x1B;
var MPU6040_RA_ACCEL_CONFIG     = 0x1C;
var MPU6040_RA_FF_THR           = 0x1D;
var MPU6040_RA_FF_DUR           = 0x1E;
var MPU6040_RA_MOT_THR          = 0x1F;
var MPU6040_RA_MOT_DUR          = 0x20;
var MPU6040_RA_ZRMOT_THR        = 0x21;
var MPU6040_RA_ZRMOT_DUR        = 0x22;
var MPU6040_RA_FIFO_EN          = 0x23;
var MPU6040_RA_I2C_MST_CTRL     = 0x24;
var MPU6040_RA_I2C_SLV0_ADDR    = 0x25;
var MPU6040_RA_I2C_SLV0_REG     = 0x26;
var MPU6040_RA_I2C_SLV0_CTRL    = 0x27;
var MPU6040_RA_I2C_SLV1_ADDR    = 0x28;
var MPU6040_RA_I2C_SLV1_REG     = 0x29;
var MPU6040_RA_I2C_SLV1_CTRL    = 0x2A;
var MPU6040_RA_I2C_SLV2_ADDR    = 0x2B;
var MPU6040_RA_I2C_SLV2_REG     = 0x2C;
var MPU6040_RA_I2C_SLV2_CTRL    = 0x2D;
var MPU6040_RA_I2C_SLV3_ADDR    = 0x2E;
var MPU6040_RA_I2C_SLV3_REG     = 0x2F;
var MPU6040_RA_I2C_SLV3_CTRL    = 0x30;
var MPU6040_RA_I2C_SLV4_ADDR    = 0x31;
var MPU6040_RA_I2C_SLV4_REG     = 0x32;
var MPU6040_RA_I2C_SLV4_DO      = 0x33;
var MPU6040_RA_I2C_SLV4_CTRL    = 0x34;
var MPU6040_RA_I2C_SLV4_DI      = 0x35;
var MPU6040_RA_I2C_MST_STATUS   = 0x36;
var MPU6040_RA_INT_PIN_CFG      = 0x37;
var MPU6040_RA_INT_ENABLE       = 0x38;
var MPU6040_RA_DMP_INT_STATUS   = 0x39;
var MPU6040_RA_INT_STATUS       = 0x3A;
var MPU6040_RA_ACCEL_XOUT_H     = 0x3B;
var MPU6040_RA_ACCEL_XOUT_L     = 0x3C;
var MPU6040_RA_ACCEL_YOUT_H     = 0x3D;
var MPU6040_RA_ACCEL_YOUT_L     = 0x3E;
var MPU6040_RA_ACCEL_ZOUT_H     = 0x3F;
var MPU6040_RA_ACCEL_ZOUT_L     = 0x40;
var MPU6040_RA_TEMP_OUT_H       = 0x41;
var MPU6040_RA_TEMP_OUT_L       = 0x42;
var MPU6040_RA_GYRO_XOUT_H      = 0x43;
var MPU6040_RA_GYRO_XOUT_L      = 0x44;
var MPU6040_RA_GYRO_YOUT_H      = 0x45;
var MPU6040_RA_GYRO_YOUT_L      = 0x46;
var MPU6040_RA_GYRO_ZOUT_H      = 0x47;
var MPU6040_RA_GYRO_ZOUT_L      = 0x48;
var MPU6040_RA_EXT_SENS_DATA_00 = 0x49;
var MPU6040_RA_EXT_SENS_DATA_01 = 0x4A;
var MPU6040_RA_EXT_SENS_DATA_02 = 0x4B;
var MPU6040_RA_EXT_SENS_DATA_03 = 0x4C;
var MPU6040_RA_EXT_SENS_DATA_04 = 0x4D;
var MPU6040_RA_EXT_SENS_DATA_05 = 0x4E;
var MPU6040_RA_EXT_SENS_DATA_06 = 0x4F;
var MPU6040_RA_EXT_SENS_DATA_07 = 0x50;
var MPU6040_RA_EXT_SENS_DATA_08 = 0x51;
var MPU6040_RA_EXT_SENS_DATA_09 = 0x52;
var MPU6040_RA_EXT_SENS_DATA_10 = 0x53;
var MPU6040_RA_EXT_SENS_DATA_11 = 0x54;
var MPU6040_RA_EXT_SENS_DATA_12 = 0x55;
var MPU6040_RA_EXT_SENS_DATA_13 = 0x56;
var MPU6040_RA_EXT_SENS_DATA_14 = 0x57;
var MPU6040_RA_EXT_SENS_DATA_15 = 0x58;
var MPU6040_RA_EXT_SENS_DATA_16 = 0x59;
var MPU6040_RA_EXT_SENS_DATA_17 = 0x5A;
var MPU6040_RA_EXT_SENS_DATA_18 = 0x5B;
var MPU6040_RA_EXT_SENS_DATA_19 = 0x5C;
var MPU6040_RA_EXT_SENS_DATA_20 = 0x5D;
var MPU6040_RA_EXT_SENS_DATA_21 = 0x5E;
var MPU6040_RA_EXT_SENS_DATA_22 = 0x5F;
var MPU6040_RA_EXT_SENS_DATA_23 = 0x60;
var MPU6040_RA_MOT_DETECT_STATUS    = 0x61;
var MPU6040_RA_I2C_SLV0_DO      = 0x63;
var MPU6040_RA_I2C_SLV1_DO      = 0x64;
var MPU6040_RA_I2C_SLV2_DO      = 0x65;
var MPU6040_RA_I2C_SLV3_DO      = 0x66;
var MPU6040_RA_I2C_MST_DELAY_CTRL   = 0x67;
var MPU6040_RA_SIGNAL_PATH_RESET    = 0x68;
var MPU6040_RA_MOT_DETECT_CTRL      = 0x69;
var MPU6040_RA_USER_CTRL        = 0x6A;
var MPU6040_RA_PWR_MGMT_1       = 0x6B;
var MPU6040_RA_PWR_MGMT_2       = 0x6C;
var MPU6040_RA_BANK_SEL         = 0x6D;
var MPU6040_RA_MEM_START_ADDR   = 0x6E;
var MPU6040_RA_MEM_R_W          = 0x6F;
var MPU6040_RA_DMP_CFG_1        = 0x70;
var MPU6040_RA_DMP_CFG_2        = 0x71;
var MPU6040_RA_FIFO_COUNTH      = 0x72;
var MPU6040_RA_FIFO_COUNTL      = 0x73;
var MPU6040_RA_FIFO_R_W         = 0x74;
var MPU6040_RA_WHO_AM_I         = 0x75;

var MPU6040_TC_PWR_MODE_BIT     = 7;
var MPU6040_TC_OFFSET_BIT       = 6;
var MPU6040_TC_OFFSET_LENGTH    = 6;
var MPU6040_TC_OTP_BNK_VLD_BIT  = 0;

var MPU6040_VDDIO_LEVEL_VLOGIC  = 0;
var MPU6040_VDDIO_LEVEL_VDD     = 1;

var MPU6040_CFG_EXT_SYNC_SET_BIT    = 5;
var MPU6040_CFG_EXT_SYNC_SET_LENGTH = 3;
var MPU6040_CFG_DLPF_CFG_BIT    = 2;
var MPU6040_CFG_DLPF_CFG_LENGTH = 3;

var MPU6040_EXT_SYNC_DISABLED       = 0x0;
var MPU6040_EXT_SYNC_TEMP_OUT_L     = 0x1;
var MPU6040_EXT_SYNC_GYRO_XOUT_L    = 0x2;
var MPU6040_EXT_SYNC_GYRO_YOUT_L    = 0x3;
var MPU6040_EXT_SYNC_GYRO_ZOUT_L    = 0x4;
var MPU6040_EXT_SYNC_ACCEL_XOUT_L   = 0x5;
var MPU6040_EXT_SYNC_ACCEL_YOUT_L   = 0x6;
var MPU6040_EXT_SYNC_ACCEL_ZOUT_L   = 0x7;

var MPU6040_DLPF_BW_256         = 0x00;
var MPU6040_DLPF_BW_188         = 0x01;
var MPU6040_DLPF_BW_98          = 0x02;
var MPU6040_DLPF_BW_42          = 0x03;
var MPU6040_DLPF_BW_20          = 0x04;
var MPU6040_DLPF_BW_10          = 0x05;
var MPU6040_DLPF_BW_5           = 0x06;

var MPU6040_GCONFIG_FS_SEL_BIT      = 4;
var MPU6040_GCONFIG_FS_SEL_LENGTH   = 2;

var MPU6040_GYRO_FS_250         = 0x00;
var MPU6040_GYRO_FS_500         = 0x01;
var MPU6040_GYRO_FS_1000        = 0x02;
var MPU6040_GYRO_FS_2000        = 0x03;

var MPU6040_ACONFIG_XA_ST_BIT           = 7;
var MPU6040_ACONFIG_YA_ST_BIT           = 6;
var MPU6040_ACONFIG_ZA_ST_BIT           = 5;
var MPU6040_ACONFIG_AFS_SEL_BIT         = 4;
var MPU6040_ACONFIG_AFS_SEL_LENGTH      = 2;
var MPU6040_ACONFIG_ACCEL_HPF_BIT       = 2;
var MPU6040_ACONFIG_ACCEL_HPF_LENGTH    = 3;

var MPU6040_ACCEL_FS_2          = 0x00;
var MPU6040_ACCEL_FS_4          = 0x01;
var MPU6040_ACCEL_FS_8          = 0x02;
var MPU6040_ACCEL_FS_16         = 0x03;

var MPU6040_DHPF_RESET          = 0x00;
var MPU6040_DHPF_5              = 0x01;
var MPU6040_DHPF_2P5            = 0x02;
var MPU6040_DHPF_1P25           = 0x03;
var MPU6040_DHPF_0P63           = 0x04;
var MPU6040_DHPF_HOLD           = 0x07;

var MPU6040_TEMP_FIFO_EN_BIT    = 7;
var MPU6040_XG_FIFO_EN_BIT      = 6;
var MPU6040_YG_FIFO_EN_BIT      = 5;
var MPU6040_ZG_FIFO_EN_BIT      = 4;
var MPU6040_ACCEL_FIFO_EN_BIT   = 3;
var MPU6040_SLV2_FIFO_EN_BIT    = 2;
var MPU6040_SLV1_FIFO_EN_BIT    = 1;
var MPU6040_SLV0_FIFO_EN_BIT    = 0;

var MPU6040_MULT_MST_EN_BIT     = 7;
var MPU6040_WAIT_FOR_ES_BIT     = 6;
var MPU6040_SLV_3_FIFO_EN_BIT   = 5;
var MPU6040_I2C_MST_P_NSR_BIT   = 4;
var MPU6040_I2C_MST_CLK_BIT     = 3;
var MPU6040_I2C_MST_CLK_LENGTH  = 4;

var MPU6040_CLOCK_DIV_348       = 0x0;
var MPU6040_CLOCK_DIV_333       = 0x1;
var MPU6040_CLOCK_DIV_320       = 0x2;
var MPU6040_CLOCK_DIV_308       = 0x3;
var MPU6040_CLOCK_DIV_296       = 0x4;
var MPU6040_CLOCK_DIV_286       = 0x5;
var MPU6040_CLOCK_DIV_276       = 0x6;
var MPU6040_CLOCK_DIV_267       = 0x7;
var MPU6040_CLOCK_DIV_258       = 0x8;
var MPU6040_CLOCK_DIV_500       = 0x9;
var MPU6040_CLOCK_DIV_471       = 0xA;
var MPU6040_CLOCK_DIV_444       = 0xB;
var MPU6040_CLOCK_DIV_421       = 0xC;
var MPU6040_CLOCK_DIV_400       = 0xD;
var MPU6040_CLOCK_DIV_381       = 0xE;
var MPU6040_CLOCK_DIV_364       = 0xF;

var MPU6040_I2C_SLV_RW_BIT      = 7;
var MPU6040_I2C_SLV_ADDR_BIT    = 6;
var MPU6040_I2C_SLV_ADDR_LENGTH = 7;
var MPU6040_I2C_SLV_EN_BIT      = 7;
var MPU6040_I2C_SLV_BYTE_SW_BIT = 6;
var MPU6040_I2C_SLV_REG_DIS_BIT = 5;
var MPU6040_I2C_SLV_GRP_BIT     = 4;
var MPU6040_I2C_SLV_LEN_BIT     = 3;
var MPU6040_I2C_SLV_LEN_LENGTH  = 4;

var MPU6040_I2C_SLV4_RW_BIT         = 7;
var MPU6040_I2C_SLV4_ADDR_BIT       = 6;
var MPU6040_I2C_SLV4_ADDR_LENGTH    = 7;
var MPU6040_I2C_SLV4_EN_BIT         = 7;
var MPU6040_I2C_SLV4_INT_EN_BIT     = 6;
var MPU6040_I2C_SLV4_REG_DIS_BIT    = 5;
var MPU6040_I2C_SLV4_MST_DLY_BIT    = 4;
var MPU6040_I2C_SLV4_MST_DLY_LENGTH = 5;

var MPU6040_MST_PASS_THROUGH_BIT    = 7;
var MPU6040_MST_I2C_SLV4_DONE_BIT   = 6;
var MPU6040_MST_I2C_LOST_ARB_BIT    = 5;
var MPU6040_MST_I2C_SLV4_NACK_BIT   = 4;
var MPU6040_MST_I2C_SLV3_NACK_BIT   = 3;
var MPU6040_MST_I2C_SLV2_NACK_BIT   = 2;
var MPU6040_MST_I2C_SLV1_NACK_BIT   = 1;
var MPU6040_MST_I2C_SLV0_NACK_BIT   = 0;

var MPU6040_INTCFG_INT_LEVEL_BIT        = 7;
var MPU6040_INTCFG_INT_OPEN_BIT         = 6;
var MPU6040_INTCFG_LATCH_INT_EN_BIT     = 5;
var MPU6040_INTCFG_INT_RD_CLEAR_BIT     = 4;
var MPU6040_INTCFG_FSYNC_INT_LEVEL_BIT  = 3;
var MPU6040_INTCFG_FSYNC_INT_EN_BIT     = 2;
var MPU6040_INTCFG_I2C_BYPASS_EN_BIT    = 1;
var MPU6040_INTCFG_CLKOUT_EN_BIT        = 0;

var MPU6040_INTMODE_ACTIVEHIGH  = 0x00;
var MPU6040_INTMODE_ACTIVELOW   = 0x01;

var MPU6040_INTDRV_PUSHPULL     = 0x00;
var MPU6040_INTDRV_OPENDRAIN    = 0x01;

var MPU6040_INTLATCH_50USPULSE  = 0x00;
var MPU6040_INTLATCH_WAITCLEAR  = 0x01;

var MPU6040_INTCLEAR_STATUSREAD = 0x00;
var MPU6040_INTCLEAR_ANYREAD    = 0x01;

var MPU6040_INTERRUPT_FF_BIT            = 7;
var MPU6040_INTERRUPT_MOT_BIT           = 6;
var MPU6040_INTERRUPT_ZMOT_BIT          = 5;
var MPU6040_INTERRUPT_FIFO_OFLOW_BIT    = 4;
var MPU6040_INTERRUPT_I2C_MST_INT_BIT   = 3;
var MPU6040_INTERRUPT_PLL_RDY_INT_BIT   = 2;
var MPU6040_INTERRUPT_DMP_INT_BIT       = 1;
var MPU6040_INTERRUPT_DATA_RDY_BIT      = 0;

// TODO= figure out what these actually do
// UMPL source code is not very obivous
var MPU6040_DMPINT_5_BIT            = 5;
var MPU6040_DMPINT_4_BIT            = 4;
var MPU6040_DMPINT_3_BIT            = 3;
var MPU6040_DMPINT_2_BIT            = 2;
var MPU6040_DMPINT_1_BIT            = 1;
var MPU6040_DMPINT_0_BIT            = 0;

var MPU6040_MOTION_MOT_XNEG_BIT     = 7;
var MPU6040_MOTION_MOT_XPOS_BIT     = 6;
var MPU6040_MOTION_MOT_YNEG_BIT     = 5;
var MPU6040_MOTION_MOT_YPOS_BIT     = 4;
var MPU6040_MOTION_MOT_ZNEG_BIT     = 3;
var MPU6040_MOTION_MOT_ZPOS_BIT     = 2;
var MPU6040_MOTION_MOT_ZRMOT_BIT    = 0;

var MPU6040_DELAYCTRL_DELAY_ES_SHADOW_BIT   = 7;
var MPU6040_DELAYCTRL_I2C_SLV4_DLY_EN_BIT   = 4;
var MPU6040_DELAYCTRL_I2C_SLV3_DLY_EN_BIT   = 3;
var MPU6040_DELAYCTRL_I2C_SLV2_DLY_EN_BIT   = 2;
var MPU6040_DELAYCTRL_I2C_SLV1_DLY_EN_BIT   = 1;
var MPU6040_DELAYCTRL_I2C_SLV0_DLY_EN_BIT   = 0;

var MPU6040_PATHRESET_GYRO_RESET_BIT    = 2;
var MPU6040_PATHRESET_ACCEL_RESET_BIT   = 1;
var MPU6040_PATHRESET_TEMP_RESET_BIT    = 0;

var MPU6040_DETECT_ACCEL_ON_DELAY_BIT       = 5;
var MPU6040_DETECT_ACCEL_ON_DELAY_LENGTH    = 2;
var MPU6040_DETECT_FF_COUNT_BIT             = 3;
var MPU6040_DETECT_FF_COUNT_LENGTH          = 2;
var MPU6040_DETECT_MOT_COUNT_BIT            = 1;
var MPU6040_DETECT_MOT_COUNT_LENGTH         = 2;

var MPU6040_DETECT_DECREMENT_RESET  = 0x0;
var MPU6040_DETECT_DECREMENT_1      = 0x1;
var MPU6040_DETECT_DECREMENT_2      = 0x2;
var MPU6040_DETECT_DECREMENT_4      = 0x3;

var MPU6040_USERCTRL_DMP_EN_BIT             = 7;
var MPU6040_USERCTRL_FIFO_EN_BIT            = 6;
var MPU6040_USERCTRL_I2C_MST_EN_BIT         = 5;
var MPU6040_USERCTRL_I2C_IF_DIS_BIT         = 4;
var MPU6040_USERCTRL_DMP_RESET_BIT          = 3;
var MPU6040_USERCTRL_FIFO_RESET_BIT         = 2;
var MPU6040_USERCTRL_I2C_MST_RESET_BIT      = 1;
var MPU6040_USERCTRL_SIG_COND_RESET_BIT     = 0;

var MPU6040_PWR1_DEVICE_RESET_BIT   = 7;
var MPU6040_PWR1_SLEEP_BIT          = 6;
var MPU6040_PWR1_CYCLE_BIT          = 5;
var MPU6040_PWR1_TEMP_DIS_BIT       = 3;
var MPU6040_PWR1_CLKSEL_BIT         = 2;
var MPU6040_PWR1_CLKSEL_LENGTH      = 3;

var MPU6040_CLOCK_INTERNAL          = 0x00;
var MPU6040_CLOCK_PLL_XGYRO         = 0x01;
var MPU6040_CLOCK_PLL_YGYRO         = 0x02;
var MPU6040_CLOCK_PLL_ZGYRO         = 0x03;
var MPU6040_CLOCK_PLL_EXT32K        = 0x04;
var MPU6040_CLOCK_PLL_EXT19M        = 0x05;
var MPU6040_CLOCK_KEEP_RESET        = 0x07;

var MPU6040_PWR2_LP_WAKE_CTRL_BIT       = 7;
var MPU6040_PWR2_LP_WAKE_CTRL_LENGTH    = 2;
var MPU6040_PWR2_STBY_XA_BIT            = 5;
var MPU6040_PWR2_STBY_YA_BIT            = 4;
var MPU6040_PWR2_STBY_ZA_BIT            = 3;
var MPU6040_PWR2_STBY_XG_BIT            = 2;
var MPU6040_PWR2_STBY_YG_BIT            = 1;
var MPU6040_PWR2_STBY_ZG_BIT            = 0;

var MPU6040_WAKE_FREQ_1P25      = 0x0;
var MPU6040_WAKE_FREQ_2P5       = 0x1;
var MPU6040_WAKE_FREQ_5         = 0x2;
var MPU6040_WAKE_FREQ_10        = 0x3;

var MPU6040_BANKSEL_PRFTCH_EN_BIT       = 6;
var MPU6040_BANKSEL_CFG_USER_BANK_BIT   = 5;
var MPU6040_BANKSEL_MEM_SEL_BIT         = 4;
var MPU6040_BANKSEL_MEM_SEL_LENGTH      = 5;

var MPU6040_WHO_AM_I_BIT        = 6;
var MPU6040_WHO_AM_I_LENGTH     = 6;

var MPU6040_DMP_MEMORY_BANKS        = 8;
var MPU6040_DMP_MEMORY_BANK_SIZE    = 256;
var MPU6040_DMP_MEMORY_CHUNK_SIZE   = 16

/* these were for the 'i2c' library (not 'raspi-i2c')

function mp6050_write(cmd, data) {
	// return a factory function to...
	return function() {
		// return a promise to..
		return new Promise(function(resolve,reject) {
			// write the bytes
			wire.writeBytes(cmd, data, function(err, res) {
				if(err) return reject(err);
				resolve();
			});
		});
		
	}
}

function mp6050_read(cmd, length) {
	// return a factory function to...
	return function() {
		// return a promise to..
		return new Promise(function(resolve,reject) {
			// read the bytes
			wire.readBytes(cmd, length, function(err, res) {
				if(err) return reject(err);
				resolve(res);
			});
		});
		
	}
}
*/


var i2c = require('i2c-bus');

// open the i2c bus
i2c1 = i2c.open(1, function (err) {
  if (err) throw err;
	
    /* async versions 
	function mp6050_write(cmd, data) {
		// return a factory function to...
		return function() {
			// return a promise to..
			return new Promise(function(resolve,reject) {
				// write the bytes
				i2c1.writeI2cBlock(i2c_address, cmd, data.length, new Buffer(data), function(err, bytesWritten, buffer) {
					if(err) return reject(err);
					resolve(bytesWritten);
				});
			});
			
		}
	}
	
	function mp6050_read(cmd, length) {
		// return a factory function to...
		return function() {
			// return a promise to..
			return new Promise(function(resolve,reject) {
				// read the bytes
				var buffer = new Buffer(length);
				i2c1.readI2cBlock(i2c_address, cmd, length, buffer, function(err, bytesRead, buffer) {
					if(err) return reject(err);
					resolve(buffer);
				});
			});
			
		}
	}
	*/
  
	// synchronous version
	function mp6050_write(cmd, data) {
		// return a factory function to...
		return function() {
			// write the bytes
			return Promise.resolve( i2c1.writeI2cBlockSync(i2c_address, cmd, data.length, new Buffer(data)) );
		}
	}
	
	function mp6050_read(cmd, length) {
		// return a factory function to...
		return function() {
			// read the bytes
			var buffer = new Buffer(length);
			i2c1.readI2cBlockSync(i2c_address, cmd, length, buffer);
			return Promise.resolve(buffer);
		}
	}
	
/* raspi-i2c requires node.js >= 0.12
// open the i2c bus
var i2c = new require('raspi-i2c');

	function mp6050_write(cmd, data) {
		// return a factory function to...
		return function() {
			// return a promise to..
			return new Promise(function(resolve,reject) {
				// write the bytes
				i2c.write(i2c_address, cmd, new Buffer(data), function(err, bytesWritten) {
					if(err) return reject(err);
					resolve(bytesWritten);
				});
			});
			
		}
	}
	
	function mp6050_read(cmd, length) {
		// return a factory function to...
		return function() {
			// return a promise to..
			return new Promise(function(resolve,reject) {
				// read the bytes
				var buffer = new Buffer(length);
				i2c.read(i2c_address, cmd, length, buffer, function(err, buffer) {
					if(err) return reject(err);
					resolve(buffer);
				});
			});
			
		}
	}


*/

	function check_i2c() {
		return mp6050_read(MPU6040_RA_WHO_AM_I, 1)();
	}
	
	function mp6050_reset() {
		// write the reset
		return mp6050_write(MPU6050_RA_PWR_MGMT_1, [0x80]);
	}
	
	function mp6050_memorybank(bank, prefetch, user) {
		// bank index
		var i = bank + (user ? 0x20 : 0) + (prefectch ? 0x40 : 0);
		// switch to bank
		return mp6050_write(MPU6050_RA_BANK_SEL, [i]);
	}
	
	function mp6050_memoryaddress(address) {
		// switch to address
		return mp6050_write(MPU6050_RA_MEM_START_ADDR, [address]);
	}
	
	function mp6050_memorybankaddress(bank, address) {
		return function() {
			return mp6050_memorybank(bank)()
				.then( mp6050_memoryaddress(address) )
		}
	}
	
	function mp6050_memoryblock_write(data, length, bank, address) {
		if(length===undefined) length = data.length;
		if(bank===undefined) bank = 0;
		if(address===undefined) address = 0;
		return function() {
			// switch to initial bank and address now
			return mp6050_memorybankaddress(bank, address)()
				.then(function() {
					// promise to write the entire block
					return new Promise(function(resolve,reject) {
						var index = 0; 
						function next_chunk() {
							// are we done?
							if(index>=length) return resolve();
							// how much can we do?
							var chunk_length = Math.min(length - index, MPU6050_DMP_MEMORY_CHUNK_SIZE);
							// get that block of data
							var chunk_data = data.slice(index, index+chunk_length);
							index += chunk_length;
							address += chunk_length;
							// write the next chunk now
							mp6050_write(MPU6050_RA_MEM_R_W, chunk_data)()
								.then(function() {
									// have we reached the end of the bank?
									if(address >= 256) {
										// switch to next bank now
										address -= 256; bank++;
										mp6050_memorybankaddress(bank, address)()
											.then(next_chunk);
									} else {
										next_chunk();							
									}
								});
						}
						// start the chain
						next_chunk();
					});
				})
		}
	}
	
	
	
	function init_i2c(mode) {
		switch(mode) {
			case 'DMP6': // six axis motion
				return MotionApps20.init();
				break;
			case 'DMP9': // nine axis motion
				return MotionApps41.init();
				break;
			default:
				// no DMP code. basic setup
				// write Sample Rate
				return mp6050_write(0x19, [0x01])()
					// Z-Axis gyro reference used for improved stability (X or Y is fine too)
					.then(mp6050_write(0x6B, [0x03]));
		}
	}
	
	function buffer_words(array, words) {
		var r = [];
		var p = 0;
		for(var i=0; i<words; i++) {
			var v = ( array[p++] << 8 ) + array[p++];
			r.push( v )
		}
		return r;
	}
	
	function buffer_ints(array, words) {
		var r = [];
		var p = 0;
		for(var i=0; i<words; i++) {
			var v = ( array[p++] << 8 ) + array[p++];
			if( v & (1<<15)) { v = -(v ^ 0xFFFF) }; 
			r.push( v )
		}
		return r;
	}
	
	function temp_celcius(n) {
		return Math.round( n/34 + 365.3 ) / 10
	}
	
	function read_temp() {
		return mp6050_read(MPU6040_RA_GYRO_XOUT_H, 6)()
			.then(function(res) {
				return temp_celcius( buffer_ints(res, 1)[0] );
			});
	}
	
	function read_gyro() {
		return mp6050_read(MPU6040_RA_GYRO_XOUT_H, 6)()
			.then(function(res) {
				return buffer_ints(res, 3);
			});
		
	}
	
	function read_accel() {
		return mp6050_read(MPU6040_RA_ACCEL_XOUT_H, 6)()
			.then(function(res) {
				return buffer_ints(res, 3);
			});
	}
	
	function read_all() {
		return new Promise(function(resolve,reject) {
			// send 'ACCEL_XOUT_H', request 14 bytes
			mp6050_read(MPU6040_RA_ACCEL_XOUT_H, 14)()
				.then(function(res) {
					// break down into components
					var ints = buffer_ints(res, 7);
					resolve( {
						accel: ints.slice(0,3),
						gyro: ints.slice(4,8),
						temp: temp_celcius( ints[3] ),
						time: (+ new Date)
					} );
				});
		});
	}
	
	var debounce = (+new Date);
	var history = [];
	
	var debug_history_width = 41;
	var debug_history_scale = 0.025;
	
	function debug_history(count) {
		function crun(c,n) {
			var s=[]; for(var i=0;i<n;i++) s.push(c);
			return s;
		}
		function cset(s,n,c) {
			// console.log('cset',s,n,c);
			n = Math.round(n);
			if((n>=0) && (n<s.length)) s[n]  = c;
		}
		for(var i=Math.max(0, history.length-count); i<history.length; i++) {
			var v = history[i];
			var line = crun(' ',debug_history_width);
			var origin = Math.floor(debug_history_width/2);
			var cscale = debug_history_scale / debug_history_width;
			cset(line,0,'[');
			cset(line,debug_history_width-1,']');
			cset(line,origin,'.');
			cset(line,origin + v.accel[0] * cscale, 'x');
			cset(line,origin + v.accel[1] * cscale, 'y');
			cset(line,origin + v.accel[2] * cscale, 'z');
			var accel = line.join('');
			var line = crun(' ',debug_history_width);
			cset(line,0,'[');
			cset(line,debug_history_width-1,']');
			cset(line,origin,'.');
			cset(line,origin + v.gyro[0] * cscale, 'x');
			cset(line,origin + v.gyro[1] * cscale, 'y');
			cset(line,origin + v.gyro[2] * cscale, 'z');
			var gyro = line.join('');
			console.log(accel,' ',gyro,' '+v.temp+"'C ."+(v.time % 1000));
		}
	}
	
	var debug_time = debounce;
	
	function timer_loop() {
		read_all()
			.then(function(r) {
				history.push(r); // add latest
				while(history.length>1024) history.shift(); // remove older than five samples
				var m = vec3_length( r.accel );
				var now = (+ new Date);
				/* old accellerometer button..
				if(m<13000) {
					// have we cleared the debounce timer?
					var bounce = now - 200; // 200ms debounce
					if(debounce < bounce) {
						if(mouse_click) mouse_click();
						//console.log('click ================================================================== click', now);
						//debug_history(20);
					}
					// reset debounce timer
					debounce = now;
				}
				*/
				// convert gyro values to mouse motion
				var mouse = [ r.gyro[0] >> 8, r.gyro[1] >> 8 ]
				mouse[0] *= -1.0;
				mouse[1] *= 1.0;
				// deadband...
				function deadband(a,i,n) {
					var v = a[i];
					if(v > n) {
						v -= n;
					} else if(v < -n) {
						v += n;
					} else {
						v = 0;
					}
					a[i] = v;
				}
				deadband(mouse,0,2); deadband(mouse,1,2);
				if(mouse[0] || mouse[1]) {
					// send a mouse control message
					// console.log('mouse ',mouse);
					if(mouse_message) mouse_message(mouse);
				}
				// schedule the next one shortly
				setTimeout(timer_loop, 15);
			})
	}
	
	// main start
	check_i2c()
		.then(init_i2c)
		.then(function() {
			console.log('ready');
			return read_all()
				.then(function(r) {
					// store the first known accelleration
					history.push(r.accel);
					// log the temperature to prove we're working
					console.log('temperature ',r.temp,'degC');
				})
		})
		.then(timer_loop);

	
	
});


/*

  Libraries tested:
    raspi-i2c: 
      fails with error "ReferenceError: Symbol is not defined", 
      because library is not compatible with node.js < 0.12, which is not what ships with raspian. (facepalm)
   i2c: 
      random segmentation faults
   i2c-bus:
      works, doesn't crash, uses 20% of the CPU! (compared to ~13% for i2c)
      
   robot.js:
      installation requires a 400mb git download due to them storing entire releases inside the repository. 
      disqualified!
      
   pigpio:
      

*/