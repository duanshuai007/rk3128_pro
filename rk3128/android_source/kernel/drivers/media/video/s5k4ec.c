 #include "generic_sensor.h"

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*/
static int version = KERNEL_VERSION(0,0,1);
module_param(version, int, S_IRUGO);


static int debug; 
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_S5K4EC
#define SENSOR_V4L2_IDENT V4L2_IDENT_S5K4EC
#define SENSOR_ID 0x4EC0
#define SENSOR_BUS_PARAM                     (V4L2_MBUS_MASTER |\
												 V4L2_MBUS_PCLK_SAMPLE_RISING|V4L2_MBUS_HSYNC_ACTIVE_HIGH| V4L2_MBUS_VSYNC_ACTIVE_HIGH|\
												 V4L2_MBUS_DATA_ACTIVE_HIGH  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W					 640//800
#define SENSOR_PREVIEW_H					 480//600   //720   //600
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 7500	   // 5fps
#define SENSOR_FULLRES_H_FPS				 15000	   // 7.5fps
#define SENSOR_720P_FPS 					 30000
#define SENSOR_1080P_FPS					 20000

#define SENSOR_REGISTER_LEN 				 2		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 2		   // sensor register value bytes
									
static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect
                                           |CFG_Scene|CFG_Focus
                                           |CFG_FocusZone);

static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

#if 0//CONFIG_SENSOR_Focus
#include "ov5640_af_firmware.c"
/* ov5640 VCM Command and Status Registers */
#define CONFIG_SENSOR_FocusCenterInCapture	  0
#define CMD_MAIN_Reg		0x3022
//#define CMD_TAG_Reg			0x3023
#define CMD_ACK_Reg 		0x3023
#define CMD_PARA0_Reg		0x3024
#define CMD_PARA1_Reg		0x3025
#define CMD_PARA2_Reg		0x3026
#define CMD_PARA3_Reg		0x3027
#define CMD_PARA4_Reg		0x3028

//#define STA_ZONE_Reg			0x3026
#define STA_FOCUS_Reg		0x3029

/* ov5640 VCM Command  */

#define ConstFocus_Cmd	  0x04
#define StepMode_Cmd	  0x05
#define PauseFocus_Cmd	  0x06
#define ReturnIdle_Cmd	  0x08
#define SetZone_Cmd 	  0x10
#define UpdateZone_Cmd	  0x12
#define SetMotor_Cmd	  0x20
#define SingleFocus_Cmd 			0x03
#define GetFocusResult_Cmd			0x07
#define ReleaseFocus_Cmd			0x08
#define ZoneRelaunch_Cmd			0x12
#define DefaultZoneConfig_Cmd		0x80
#define TouchZoneConfig_Cmd 		0x81
#define CustomZoneConfig_Cmd		0x8f


/* ov5640 Focus State */
//#define S_FIRWRE				0xFF		/*Firmware is downloaded and not run*/
#define S_STARTUP			0x7e		/*Firmware is initializing*/
#define S_ERROR 			0x7f
#define S_IDLE				0x70		/*Idle state, focus is released; lens is located at the furthest position.*/
#define S_FOCUSING			0x00		/*Auto Focus is running.*/
#define S_FOCUSED			0x10		/*Auto Focus is completed.*/

#define S_CAPTURE			0x12
#define S_STEP					0x20

/* ov5640 Zone State */
#define Zone_Is_Focused(a, zone_val)	(zone_val&(1<<(a-3)))
#define Zone_Get_ID(zone_val)			(zone_val&0x03)

#define Zone_CenterMode   0x01
#define Zone_5xMode 	  0x02
#define Zone_5PlusMode	  0x03
#define Zone_4fMode 	  0x04

#define ZoneSel_Auto	  0x0b
#define ZoneSel_SemiAuto  0x0c
#define ZoneSel_Manual	  0x0d
#define ZoneSel_Rotate	  0x0e

/* ov5640 Step Focus Commands */
#define StepFocus_Near_Tag		 0x01
#define StepFocus_Far_Tag		 0x02
#define StepFocus_Furthest_Tag	 0x03
#define StepFocus_Nearest_Tag	 0x04
#define StepFocus_Spec_Tag		 0x10
#endif

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;
	unsigned short int preview_maxlines;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
//==================================================================================
// 01.Start Setting
//==================================================================================

{0xFCFC, 0xD000},	
{0x0010, 0x0001},	//S/W Reset
{0x1030, 0x0000},	//contint_host_int
{0x0014, 0x0001},	//sw_load_complete - Release CORE (Arm) from reset state

{0xffff, 0x0010}, //Delay 10ms

//==================================================================================                
// 02.ETC Setting                                                                                    
//==================================================================================                
                                                                                                    
{0x0028, 0xD000},
{0x002A, 0x1082},
{0x0F12, 0x0155},  //d0_d4_cd10   d0_d4_cd10
{0x0F12, 0x0155},  //d5_d9_cd10   d5_d9_cd10
{0x0F12, 0x0055},  //gpio_cd10 gpio_cd10
{0x0F12, 0x0555},  //clks_output_cd10
{0x002A, 0x100E},
{0x0F12, 0x0000},  //pclk_delay_r
{0x002A, 0x007A},
{0x0F12, 0x0000},
{0x002A, 0xE406},
{0x0F12, 0x0092},  //[7]f_ladlc_en [6:5]f max [4]fadlc_en [3:2]L max [1]ladlc_en [0]adlc_ch_sel
{0x002A, 0xE410},
{0x0F12, 0x3804},  //[15:8]fadlc_filter_co_b, [7:0]fadlc_filter_co_a
{0x002A, 0xE41A},
{0x0F12, 0x0010},  //adlc_ptune_total
{0x002A, 0xF132},    
{0x0F12, 0x0200},
{0x002A, 0xF142},
{0x0F12, 0x0200},  //110404 AE haunting - from_LSI
{0x002A, 0xE420},
{0x0F12, 0x0003},
{0x0F12, 0x0060},
{0x002A, 0xE42E},
{0x0F12, 0x0004},
{0x002A, 0xF400},
{0x0F12, 0x5A3C},
{0x0F12, 0x0023},
{0x0F12, 0x8080},
{0x0F12, 0x03AF},
{0x0F12, 0x000A},
{0x0F12, 0xAA54},
{0x0F12, 0x0040},
{0x0F12, 0x464E},
{0x0F12, 0x0240},
{0x0F12, 0x0240},
{0x0F12, 0x0040},
{0x0F12, 0x1000},
{0x0F12, 0x55FF},
{0x0F12, 0xD000},
{0x0F12, 0x0010},
{0x0F12, 0x0202},
{0x0F12, 0x0401},
{0x0F12, 0x0022},
{0x0F12, 0x0088},
{0x0F12, 0x009F},
{0x0F12, 0x0000},
{0x0F12, 0x1800},
{0x0F12, 0x0088},
{0x0F12, 0x0000},
{0x0F12, 0x2428},
{0x0F12, 0x0000},
{0x0F12, 0x03EE},
{0x0F12, 0x0000},
{0x0F12, 0x0000},
{0x0F12, 0x0000},
{0x002A, 0xF552},
{0x0F12, 0x0708},  //[7:0]lat_st, [15:8]lat_width
{0x0F12, 0x080C},  //[7:0]hold_st, [15:8]hold_width
{0x0028, 0x7000},
{0x002A, 0x18BC},
{0x0F12, 0x0004},  //#senHal_ContPtrs_senModesDataArr
{0x0F12, 0x05B6},  //#senHal_ContPtrs_senModesDataArr_1_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_2_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_3_
{0x0F12, 0x0001},  //#senHal_ContPtrs_senModesDataArr_4_
{0x0F12, 0x05BA},  //#senHal_ContPtrs_senModesDataArr_5_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_6_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_7_
{0x0F12, 0x0007},  //#senHal_ContPtrs_senModesDataArr_8_
{0x0F12, 0x05BA},  //#senHal_ContPtrs_senModesDataArr_9_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_10_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_11_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_12_
{0x0F12, 0x024E},  //#senHal_ContPtrs_senModesDataArr_13_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_14_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_15_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_16_
{0x0F12, 0x05B6},  //#senHal_ContPtrs_senModesDataArr_17_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_18_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_19_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_20_
{0x0F12, 0x05BA},  //#senHal_ContPtrs_senModesDataArr_21_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_22_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_23_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_24_
{0x0F12, 0x024F},  //#senHal_ContPtrs_senModesDataArr_25_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_26_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_27_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_28_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_29_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_30_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_31_
{0x0F12, 0x0075},  //#senHal_ContPtrs_senModesDataArr_32_
{0x0F12, 0x00CF},  //#senHal_ContPtrs_senModesDataArr_33_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_34_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_35_
{0x0F12, 0x0075},  //#senHal_ContPtrs_senModesDataArr_36_
{0x0F12, 0x00D6},  //#senHal_ContPtrs_senModesDataArr_37_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_38_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_39_
{0x0F12, 0x0004},  //#senHal_ContPtrs_senModesDataArr_40_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_41_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_42_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_43_
{0x0F12, 0x00F0},  //#senHal_ContPtrs_senModesDataArr_44_
{0x0F12, 0x01F4},  //#senHal_ContPtrs_senModesDataArr_45_
{0x0F12, 0x029E},  //#senHal_ContPtrs_senModesDataArr_46_
{0x0F12, 0x05B2},  //#senHal_ContPtrs_senModesDataArr_47_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_48_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_49_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_50_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_51_
{0x0F12, 0x01F8},  //#senHal_ContPtrs_senModesDataArr_52_
{0x0F12, 0x0228},  //#senHal_ContPtrs_senModesDataArr_53_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_54_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_55_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_56_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_57_
{0x0F12, 0x0208},  //#senHal_ContPtrs_senModesDataArr_58_
{0x0F12, 0x0238},  //#senHal_ContPtrs_senModesDataArr_59_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_60_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_61_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_62_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_63_
{0x0F12, 0x0218},  //#senHal_ContPtrs_senModesDataArr_64_
{0x0F12, 0x0238},  //#senHal_ContPtrs_senModesDataArr_65_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_66_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_67_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_68_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_69_
{0x0F12, 0x0001},  //#senHal_ContPtrs_senModesDataArr_70_
{0x0F12, 0x0009},  //#senHal_ContPtrs_senModesDataArr_71_
{0x0F12, 0x00DE},  //#senHal_ContPtrs_senModesDataArr_72_
{0x0F12, 0x05C0},  //#senHal_ContPtrs_senModesDataArr_73_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_74_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_75_
{0x0F12, 0x00DF},  //#senHal_ContPtrs_senModesDataArr_76_
{0x0F12, 0x00E4},  //#senHal_ContPtrs_senModesDataArr_77_
{0x0F12, 0x01F8},  //#senHal_ContPtrs_senModesDataArr_78_
{0x0F12, 0x01FD},  //#senHal_ContPtrs_senModesDataArr_79_
{0x0F12, 0x05B6},  //#senHal_ContPtrs_senModesDataArr_80_
{0x0F12, 0x05BB},  //#senHal_ContPtrs_senModesDataArr_81_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_82_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_83_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_84_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_85_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_86_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_87_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_88_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_89_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_90_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_91_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_92_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_93_
{0x0F12, 0x01F8},  //#senHal_ContPtrs_senModesDataArr_94_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_95_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_96_
{0x0F12, 0x0077},  //#senHal_ContPtrs_senModesDataArr_97_
{0x0F12, 0x007E},  //#senHal_ContPtrs_senModesDataArr_98_
{0x0F12, 0x024F},  //#senHal_ContPtrs_senModesDataArr_99_
{0x0F12, 0x025E},  //#senHal_ContPtrs_senModesDataArr_100_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_101_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_102_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_103_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senModesDataArr_104_
{0x0F12, 0x0004},  //#senHal_ContPtrs_senAvgModesDataArr
{0x0F12, 0x09D1},  //#senHal_ContPtrs_senAvgModesDataArr_1_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_2_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_3_
{0x0F12, 0x0001},  //#senHal_ContPtrs_senAvgModesDataArr_4_
{0x0F12, 0x09D5},  //#senHal_ContPtrs_senAvgModesDataArr_5_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_6_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_7_
{0x0F12, 0x0008},  //#senHal_ContPtrs_senAvgModesDataArr_8_
{0x0F12, 0x09D5},  //#senHal_ContPtrs_senAvgModesDataArr_9_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_10_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_11_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_12_
{0x0F12, 0x0326},  //#senHal_ContPtrs_senAvgModesDataArr_13_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_14_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_15_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_16_
{0x0F12, 0x09D1},  //#senHal_ContPtrs_senAvgModesDataArr_17_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_18_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_19_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_20_
{0x0F12, 0x09D5},  //#senHal_ContPtrs_senAvgModesDataArr_21_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_22_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_23_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_24_
{0x0F12, 0x0327},  //#senHal_ContPtrs_senAvgModesDataArr_25_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_26_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_27_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_28_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_29_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_30_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_31_
{0x0F12, 0x0008},  //#senHal_ContPtrs_senAvgModesDataArr_32_
{0x0F12, 0x0084},  //#senHal_ContPtrs_senAvgModesDataArr_33_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_34_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_35_
{0x0F12, 0x0008},  //#senHal_ContPtrs_senAvgModesDataArr_36_
{0x0F12, 0x008D},  //#senHal_ContPtrs_senAvgModesDataArr_37_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_38_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_39_
{0x0F12, 0x0008},  //#senHal_ContPtrs_senAvgModesDataArr_40_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_41_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_42_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_43_
{0x0F12, 0x00AA},  //#senHal_ContPtrs_senAvgModesDataArr_44_
{0x0F12, 0x02AA},  //#senHal_ContPtrs_senAvgModesDataArr_45_
{0x0F12, 0x03AD},  //#senHal_ContPtrs_senAvgModesDataArr_46_
{0x0F12, 0x09CD},  //#senHal_ContPtrs_senAvgModesDataArr_47_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_48_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_49_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_50_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_51_
{0x0F12, 0x02AE},  //#senHal_ContPtrs_senAvgModesDataArr_52_
{0x0F12, 0x02DE},  //#senHal_ContPtrs_senAvgModesDataArr_53_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_54_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_55_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_56_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_57_
{0x0F12, 0x02BE},  //#senHal_ContPtrs_senAvgModesDataArr_58_
{0x0F12, 0x02EE},  //#senHal_ContPtrs_senAvgModesDataArr_59_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_60_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_61_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_62_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_63_
{0x0F12, 0x02CE},  //#senHal_ContPtrs_senAvgModesDataArr_64_
{0x0F12, 0x02EE},  //#senHal_ContPtrs_senAvgModesDataArr_65_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_66_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_67_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_68_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_69_
{0x0F12, 0x0001},  //#senHal_ContPtrs_senAvgModesDataArr_70_
{0x0F12, 0x0009},  //#senHal_ContPtrs_senAvgModesDataArr_71_
{0x0F12, 0x0095},  //#senHal_ContPtrs_senAvgModesDataArr_72_
{0x0F12, 0x09DB},  //#senHal_ContPtrs_senAvgModesDataArr_73_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_74_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_75_
{0x0F12, 0x0096},  //#senHal_ContPtrs_senAvgModesDataArr_76_
{0x0F12, 0x009B},  //#senHal_ContPtrs_senAvgModesDataArr_77_
{0x0F12, 0x02AE},  //#senHal_ContPtrs_senAvgModesDataArr_78_
{0x0F12, 0x02B3},  //#senHal_ContPtrs_senAvgModesDataArr_79_
{0x0F12, 0x09D1},  //#senHal_ContPtrs_senAvgModesDataArr_80_
{0x0F12, 0x09D6},  //#senHal_ContPtrs_senAvgModesDataArr_81_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_82_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_83_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_84_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_85_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_86_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_87_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_88_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_89_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_90_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_91_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_92_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_93_
{0x0F12, 0x02AE},  //#senHal_ContPtrs_senAvgModesDataArr_94_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_95_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_96_
{0x0F12, 0x0009},  //#senHal_ContPtrs_senAvgModesDataArr_97_
{0x0F12, 0x0010},  //#senHal_ContPtrs_senAvgModesDataArr_98_
{0x0F12, 0x0327},  //#senHal_ContPtrs_senAvgModesDataArr_99_
{0x0F12, 0x0336},  //#senHal_ContPtrs_senAvgModesDataArr_100_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_101_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_102_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_103_
{0x0F12, 0x0000},  //#senHal_ContPtrs_senAvgModesDataArr_104_
{0x002A, 0x0EEC},                                                                                                                                                                                                          
{0x0F12, 0x000A},  //Make sensor X start offset to make in subsapling mode
{0x0F12, 0x0014},  //Make sensor X start offset to make in full size mode
{0x002A, 0x1AF8},
{0x0F12, 0x5A3C},  //#senHal_TuneStr_AngTuneData1_2_D000F400 register at subsampling 
{0x002A, 0x1896},
{0x0F12, 0x0002},  //#senHal_SamplingType  // 0002 03EE: PLA setting
{0x0F12, 0x0000},  //#senHal_SamplingMode  // 0 : 2 PLA / 1 : 4PLA
{0x0F12, 0x0003},  //#senHal_PLAOption     // [0] VPLA enable , [1] HPLA enable
{0x002A, 0x189E},
{0x0F12, 0x0FB0},  //#senHal_ExpMinPixels
{0x002A, 0x18AC},
{0x0F12, 0x0060},  //#senHal_uAddColsBin
{0x0F12, 0x0060},  //#senHal_uAddColsNoBin
{0x0F12, 0x05C0},  //#senHal_uMinColsBin
{0x0F12, 0x05C0},  //#senHal_uMinColsNoBin
{0x002A, 0x1AEA},
{0x0F12, 0x8080},  //#senHal_SubF404Tune
{0x0F12, 0x0080},  //#senHal_FullF404Tune
{0x002A, 0x1AE0},
{0x0F12, 0x0000},  //#senHal_bSenAAC
{0x002A, 0x1A72},
{0x0F12, 0x0000},  //#senHal_bSRX                                                                                                                                        //SRX off
{0x002A, 0x18A2},
{0x0F12, 0x0004},  //#senHal_NExpLinesCheckFine                                                 //extend Forbidden area line
{0x002A, 0x1A6A},
{0x0F12, 0x009A},  //#senHal_usForbiddenRightOfs                                                //extend right Forbidden area line
{0x002A, 0x385E},
{0x0F12, 0x024C},  //#Mon_Sen_uExpPixelsOfs
{0x002A, 0x0EE6},
{0x0F12, 0x0000},  //#setot_bUseDigitalHbin
{0x002A, 0x1B2A},
{0x0F12, 0x0300},  //senHal_TuneStr2_usAngTuneGainTh
{0x0F12, 0x00D6},  //senHal_TuneStr2_AngTuneF4CA_0_
{0x0F12, 0x008D},  //senHal_TuneStr2_AngTuneF4CA_1_
{0x0F12, 0x00CF},  //senHal_TuneStr2_AngTuneF4C2_0_
{0x0F12, 0x0084},  //senHal_TuneStr2_AngTuneF4C2_1_

///////////////////////////////////////////////////////////////////////////
// OTP setting
///////////////////////////////////////////////////////////////////////////
{0x002A, 0x0722},
{0x0F12, 0x0100},  //#skl_OTP_usWaitTime  *This register should be positioned in fornt of D0001000
{0x002A, 0x0726},
{0x0F12, 0x0000},  //skl_bUseOTPfunc OTP shading is used,this reg should be 1 
{0x002A, 0x08D6},
{0x0F12, 0x0000},  //#ash_bUseOTPData
{0x002A, 0x146E},
{0x0F12, 0x0000},  //#awbb_otp_disable
{0x002A, 0x08DC},
{0x0F12, 0x0000},  //#ash_bUseGasAlphaOTP

///////////////////////////////////////////////////////////////////////////
// TnP setting
///////////////////////////////////////////////////////////////////////////
{0x0028, 0x7000},
{0x002A, 0x3AF8},
{0x0F12, 0xB5F8},  //70003AF8 
{0x0F12, 0x4B41},  //70003AFA 
{0x0F12, 0x4941},  //70003AFC 
{0x0F12, 0x4842},  //70003AFE 
{0x0F12, 0x2200},  //70003B00 
{0x0F12, 0xC008},  //70003B02 
{0x0F12, 0x6001},  //70003B04 
{0x0F12, 0x4941},  //70003B06 
{0x0F12, 0x4841},  //70003B08 
{0x0F12, 0x2401},  //70003B0A 
{0x0F12, 0xF000},  //70003B0C 
{0x0F12, 0xFC5E},  //70003B0E 
{0x0F12, 0x4940},  //70003B10 
{0x0F12, 0x4841},  //70003B12 
{0x0F12, 0x2702},  //70003B14 
{0x0F12, 0x0022},  //70003B16 
{0x0F12, 0xF000},  //70003B18 
{0x0F12, 0xFC58},  //70003B1A 
{0x0F12, 0x0260},  //70003B1C 
{0x0F12, 0x4C3F},  //70003B1E 
{0x0F12, 0x8020},  //70003B20 
{0x0F12, 0x2600},  //70003B22 
{0x0F12, 0x8066},  //70003B24 
{0x0F12, 0x493E},  //70003B26 
{0x0F12, 0x483E},  //70003B28 
{0x0F12, 0x6041},  //70003B2A 
{0x0F12, 0x493E},  //70003B2C 
{0x0F12, 0x483F},  //70003B2E 
{0x0F12, 0x003A},  //70003B30 
{0x0F12, 0x2503},  //70003B32 
{0x0F12, 0xF000},  //70003B34 
{0x0F12, 0xFC4A},  //70003B36 
{0x0F12, 0x483A},  //70003B38 
{0x0F12, 0x493D},  //70003B3A 
{0x0F12, 0x30C0},  //70003B3C 
{0x0F12, 0x63C1},  //70003B3E 
{0x0F12, 0x4F38},  //70003B40 
{0x0F12, 0x483C},  //70003B42 
{0x0F12, 0x3F80},  //70003B44 
{0x0F12, 0x6438},  //70003B46 
{0x0F12, 0x483B},  //70003B48 
{0x0F12, 0x493C},  //70003B4A 
{0x0F12, 0x6388},  //70003B4C 
{0x0F12, 0x002A},  //70003B4E 
{0x0F12, 0x493B},  //70003B50 
{0x0F12, 0x483C},  //70003B52 
{0x0F12, 0x2504},  //70003B54 
{0x0F12, 0xF000},  //70003B56 
{0x0F12, 0xFC39},  //70003B58 
{0x0F12, 0x002A},  //70003B5A 
{0x0F12, 0x493A},  //70003B5C 
{0x0F12, 0x483B},  //70003B5E 
{0x0F12, 0x2505},  //70003B60 
{0x0F12, 0xF000},  //70003B62 
{0x0F12, 0xF89D},  //70003B64 
{0x0F12, 0x4839},  //70003B66 
{0x0F12, 0x002A},  //70003B68 
{0x0F12, 0x4939},  //70003B6A 
{0x0F12, 0x2506},  //70003B6C 
{0x0F12, 0x1D80},  //70003B6E 
{0x0F12, 0xF000},  //70003B70 
{0x0F12, 0xF896},  //70003B72 
{0x0F12, 0x4835},  //70003B74 
{0x0F12, 0x002A},  //70003B76 
{0x0F12, 0x4936},  //70003B78 
{0x0F12, 0x2507},  //70003B7A 
{0x0F12, 0x300C},  //70003B7C 
{0x0F12, 0xF000},  //70003B7E 
{0x0F12, 0xF88F},  //70003B80 
{0x0F12, 0x4832},  //70003B82 
{0x0F12, 0x002A},  //70003B84 
{0x0F12, 0x4934},  //70003B86 
{0x0F12, 0x2508},  //70003B88 
{0x0F12, 0x3010},  //70003B8A 
{0x0F12, 0xF000},  //70003B8C 
{0x0F12, 0xF888},  //70003B8E 
{0x0F12, 0x002A},  //70003B90 
{0x0F12, 0x4932},  //70003B92 
{0x0F12, 0x4832},  //70003B94 
{0x0F12, 0x2509},  //70003B96 
{0x0F12, 0xF000},  //70003B98 
{0x0F12, 0xFC18},  //70003B9A 
{0x0F12, 0x002A},  //70003B9C 
{0x0F12, 0x4931},  //70003B9E 
{0x0F12, 0x4831},  //70003BA0 
{0x0F12, 0x250A},  //70003BA2 
{0x0F12, 0xF000},  //70003BA4 
{0x0F12, 0xFC12},  //70003BA6 
{0x0F12, 0x002A},  //70003BA8 
{0x0F12, 0x4930},  //70003BAA 
{0x0F12, 0x4830},  //70003BAC 
{0x0F12, 0x250B},  //70003BAE 
{0x0F12, 0xF000},  //70003BB0 
{0x0F12, 0xFC0C},  //70003BB2 
{0x0F12, 0x002A},  //70003BB4 
{0x0F12, 0x492F},  //70003BB6 
{0x0F12, 0x482F},  //70003BB8 
{0x0F12, 0x250C},  //70003BBA 
{0x0F12, 0xF000},  //70003BBC 
{0x0F12, 0xFC06},  //70003BBE 
{0x0F12, 0x002A},  //70003BC0 
{0x0F12, 0x492E},  //70003BC2 
{0x0F12, 0x482E},  //70003BC4 
{0x0F12, 0x250D},  //70003BC6 
{0x0F12, 0xF000},  //70003BC8 
{0x0F12, 0xFC00},  //70003BCA 
{0x0F12, 0x002A},  //70003BCC 
{0x0F12, 0x492D},  //70003BCE 
{0x0F12, 0x482D},  //70003BD0 
{0x0F12, 0x250E},  //70003BD2 
{0x0F12, 0xF000},  //70003BD4 
{0x0F12, 0xFBFA},  //70003BD6 
{0x0F12, 0x8626},  //70003BD8 
{0x0F12, 0x20FF},  //70003BDA 
{0x0F12, 0x1C40},  //70003BDC 
{0x0F12, 0x8660},  //70003BDE 
{0x0F12, 0x482A},  //70003BE0 
{0x0F12, 0x64F8},  //70003BE2 
{0x0F12, 0x492A},  //70003BE4 
{0x0F12, 0x482B},  //70003BE6 
{0x0F12, 0x002A},  //70003BE8 
{0x0F12, 0x240F},  //70003BEA 
{0x0F12, 0xF000},  //70003BEC 
{0x0F12, 0xFBEE},  //70003BEE 
{0x0F12, 0x4929},  //70003BF0 
{0x0F12, 0x482A},  //70003BF2 
{0x0F12, 0x0022},  //70003BF4 
{0x0F12, 0xF000},  //70003BF6 
{0x0F12, 0xFBE9},  //70003BF8 
{0x0F12, 0xBCF8},  //70003BFA 
{0x0F12, 0xBC08},  //70003BFC 
{0x0F12, 0x4718},  //70003BFE 
{0x0F12, 0x017B},  //70003C00 
{0x0F12, 0x4EC2},  //70003C02 
{0x0F12, 0x237F},  //70003C04 
{0x0F12, 0x0000},  //70003C06 
{0x0F12, 0x1F90},  //70003C08 
{0x0F12, 0x7000},  //70003C0A 
{0x0F12, 0x3CB9},  //70003C0C 
{0x0F12, 0x7000},  //70003C0E 
{0x0F12, 0xE38B},  //70003C10 
{0x0F12, 0x0000},  //70003C12 
{0x0F12, 0x3CF1},  //70003C14 
{0x0F12, 0x7000},  //70003C16 
{0x0F12, 0xC3B1},  //70003C18 
{0x0F12, 0x0000},  //70003C1A 
{0x0F12, 0x4780},  //70003C1C 
{0x0F12, 0x7000},  //70003C1E 
{0x0F12, 0x3D4F},  //70003C20 
{0x0F12, 0x7000},  //70003C22 
{0x0F12, 0x0080},  //70003C24 
{0x0F12, 0x7000},  //70003C26 
{0x0F12, 0x3D8B},  //70003C28 
{0x0F12, 0x7000},  //70003C2A 
{0x0F12, 0xB49D},  //70003C2C 
{0x0F12, 0x0000},  //70003C2E 
{0x0F12, 0x3E37},  //70003C30 
{0x0F12, 0x7000},  //70003C32 
{0x0F12, 0x3DEB},  //70003C34 
{0x0F12, 0x7000},  //70003C36 
{0x0F12, 0xFFFF},  //70003C38 
{0x0F12, 0x00FF},  //70003C3A 
{0x0F12, 0x17E0},  //70003C3C 
{0x0F12, 0x7000},  //70003C3E 
{0x0F12, 0x3FB3},  //70003C40 
{0x0F12, 0x7000},  //70003C42 
{0x0F12, 0x053D},  //70003C44 
{0x0F12, 0x0000},  //70003C46 
{0x0F12, 0x0000},  //70003C48 
{0x0F12, 0x0A89},  //70003C4A 
{0x0F12, 0x6CD2},  //70003C4C 
{0x0F12, 0x0000},  //70003C4E 
{0x0F12, 0x02C9},  //70003C50 
{0x0F12, 0x0000},  //70003C52 
{0x0F12, 0x0000},  //70003C54 
{0x0F12, 0x0A9A},  //70003C56 
{0x0F12, 0x0000},  //70003C58 
{0x0F12, 0x02D2},  //70003C5A 
{0x0F12, 0x4001},  //70003C5C 
{0x0F12, 0x7000},  //70003C5E 
{0x0F12, 0x9E65},  //70003C60 
{0x0F12, 0x0000},  //70003C62 
{0x0F12, 0x4075},  //70003C64 
{0x0F12, 0x7000},  //70003C66 
{0x0F12, 0x7C49},  //70003C68 
{0x0F12, 0x0000},  //70003C6A 
{0x0F12, 0x40E9},  //70003C6C 
{0x0F12, 0x7000},  //70003C6E 
{0x0F12, 0x7C63},  //70003C70 
{0x0F12, 0x0000},  //70003C72 
{0x0F12, 0x4105},  //70003C74 
{0x0F12, 0x7000},  //70003C76 
{0x0F12, 0x8F01},  //70003C78 
{0x0F12, 0x0000},  //70003C7A 
{0x0F12, 0x41A7},  //70003C7C 
{0x0F12, 0x7000},  //70003C7E 
{0x0F12, 0x7F3F},  //70003C80 
{0x0F12, 0x0000},  //70003C82 
{0x0F12, 0x4235},  //70003C84 
{0x0F12, 0x7000},  //70003C86 
{0x0F12, 0x98C5},  //70003C88 
{0x0F12, 0x0000},  //70003C8A 
{0x0F12, 0x42FB},  //70003C8C 
{0x0F12, 0x7000},  //70003C8E 
{0x0F12, 0x4351},  //70003C90 
{0x0F12, 0x7000},  //70003C92 
{0x0F12, 0xA70B},  //70003C94 
{0x0F12, 0x0000},  //70003C96 
{0x0F12, 0x4373},  //70003C98 
{0x0F12, 0x7000},  //70003C9A 
{0x0F12, 0x400D},  //70003C9C 
{0x0F12, 0x0000},  //70003C9E 
{0x0F12, 0xB570},  //70003CA0 
{0x0F12, 0x000C},  //70003CA2 
{0x0F12, 0x0015},  //70003CA4 
{0x0F12, 0x0029},  //70003CA6 
{0x0F12, 0xF000},  //70003CA8 
{0x0F12, 0xFB98},  //70003CAA 
{0x0F12, 0x49F8},  //70003CAC 
{0x0F12, 0x00A8},  //70003CAE 
{0x0F12, 0x500C},  //70003CB0 
{0x0F12, 0xBC70},  //70003CB2 
{0x0F12, 0xBC08},  //70003CB4 
{0x0F12, 0x4718},  //70003CB6 
{0x0F12, 0x6808},  //70003CB8 
{0x0F12, 0x0400},  //70003CBA 
{0x0F12, 0x0C00},  //70003CBC 
{0x0F12, 0x6849},  //70003CBE 
{0x0F12, 0x0409},  //70003CC0 
{0x0F12, 0x0C09},  //70003CC2 
{0x0F12, 0x4AF3},  //70003CC4 
{0x0F12, 0x8992},  //70003CC6 
{0x0F12, 0x2A00},  //70003CC8 
{0x0F12, 0xD00D},  //70003CCA 
{0x0F12, 0x2300},  //70003CCC 
{0x0F12, 0x1A89},  //70003CCE 
{0x0F12, 0xD400},  //70003CD0 
{0x0F12, 0x000B},  //70003CD2 
{0x0F12, 0x0419},  //70003CD4 
{0x0F12, 0x0C09},  //70003CD6 
{0x0F12, 0x23FF},  //70003CD8 
{0x0F12, 0x33C1},  //70003CDA 
{0x0F12, 0x1810},  //70003CDC 
{0x0F12, 0x4298},  //70003CDE 
{0x0F12, 0xD800},  //70003CE0 
{0x0F12, 0x0003},  //70003CE2 
{0x0F12, 0x0418},  //70003CE4 
{0x0F12, 0x0C00},  //70003CE6 
{0x0F12, 0x4AEB},  //70003CE8 
{0x0F12, 0x8150},  //70003CEA 
{0x0F12, 0x8191},  //70003CEC 
{0x0F12, 0x4770},  //70003CEE 
{0x0F12, 0xB5F3},  //70003CF0 
{0x0F12, 0x0004},  //70003CF2 
{0x0F12, 0xB081},  //70003CF4 
{0x0F12, 0x9802},  //70003CF6 
{0x0F12, 0x6800},  //70003CF8 
{0x0F12, 0x0600},  //70003CFA 
{0x0F12, 0x0E00},  //70003CFC 
{0x0F12, 0x2201},  //70003CFE 
{0x0F12, 0x0015},  //70003D00 
{0x0F12, 0x0021},  //70003D02 
{0x0F12, 0x3910},  //70003D04 
{0x0F12, 0x408A},  //70003D06 
{0x0F12, 0x40A5},  //70003D08 
{0x0F12, 0x4FE4},  //70003D0A 
{0x0F12, 0x0016},  //70003D0C 
{0x0F12, 0x2C10},  //70003D0E 
{0x0F12, 0xDA03},  //70003D10 
{0x0F12, 0x8839},  //70003D12 
{0x0F12, 0x43A9},  //70003D14 
{0x0F12, 0x8039},  //70003D16 
{0x0F12, 0xE002},  //70003D18 
{0x0F12, 0x8879},  //70003D1A 
{0x0F12, 0x43B1},  //70003D1C 
{0x0F12, 0x8079},  //70003D1E 
{0x0F12, 0xF000},  //70003D20 
{0x0F12, 0xFB64},  //70003D22 
{0x0F12, 0x2C10},  //70003D24 
{0x0F12, 0xDA03},  //70003D26 
{0x0F12, 0x8839},  //70003D28 
{0x0F12, 0x4329},  //70003D2A 
{0x0F12, 0x8039},  //70003D2C 
{0x0F12, 0xE002},  //70003D2E 
{0x0F12, 0x8879},  //70003D30 
{0x0F12, 0x4331},  //70003D32 
{0x0F12, 0x8079},  //70003D34 
{0x0F12, 0x49DA},  //70003D36 
{0x0F12, 0x8809},  //70003D38 
{0x0F12, 0x2900},  //70003D3A 
{0x0F12, 0xD102},  //70003D3C 
{0x0F12, 0xF000},  //70003D3E 
{0x0F12, 0xFB5D},  //70003D40 
{0x0F12, 0x2000},  //70003D42 
{0x0F12, 0x9902},  //70003D44 
{0x0F12, 0x6008},  //70003D46 
{0x0F12, 0xBCFE},  //70003D48 
{0x0F12, 0xBC08},  //70003D4A 
{0x0F12, 0x4718},  //70003D4C 
{0x0F12, 0xB538},  //70003D4E 
{0x0F12, 0x9C04},  //70003D50 
{0x0F12, 0x0015},  //70003D52 
{0x0F12, 0x002A},  //70003D54 
{0x0F12, 0x9400},  //70003D56 
{0x0F12, 0xF000},  //70003D58 
{0x0F12, 0xFB58},  //70003D5A 
{0x0F12, 0x4AD1},  //70003D5C 
{0x0F12, 0x8811},  //70003D5E 
{0x0F12, 0x2900},  //70003D60 
{0x0F12, 0xD00F},  //70003D62 
{0x0F12, 0x8820},  //70003D64 
{0x0F12, 0x4281},  //70003D66 
{0x0F12, 0xD20C},  //70003D68 
{0x0F12, 0x8861},  //70003D6A 
{0x0F12, 0x8853},  //70003D6C 
{0x0F12, 0x4299},  //70003D6E 
{0x0F12, 0xD200},  //70003D70 
{0x0F12, 0x1E40},  //70003D72 
{0x0F12, 0x0400},  //70003D74 
{0x0F12, 0x0C00},  //70003D76 
{0x0F12, 0x8020},  //70003D78 
{0x0F12, 0x8851},  //70003D7A 
{0x0F12, 0x8061},  //70003D7C 
{0x0F12, 0x4368},  //70003D7E 
{0x0F12, 0x1840},  //70003D80 
{0x0F12, 0x6060},  //70003D82 
{0x0F12, 0xBC38},  //70003D84 
{0x0F12, 0xBC08},  //70003D86 
{0x0F12, 0x4718},  //70003D88 
{0x0F12, 0xB5F8},  //70003D8A 
{0x0F12, 0x0004},  //70003D8C 
{0x0F12, 0x6808},  //70003D8E 
{0x0F12, 0x0400},  //70003D90 
{0x0F12, 0x0C00},  //70003D92 
{0x0F12, 0x2201},  //70003D94 
{0x0F12, 0x0015},  //70003D96 
{0x0F12, 0x0021},  //70003D98 
{0x0F12, 0x3910},  //70003D9A 
{0x0F12, 0x408A},  //70003D9C 
{0x0F12, 0x40A5},  //70003D9E 
{0x0F12, 0x4FBE},  //70003DA0 
{0x0F12, 0x0016},  //70003DA2 
{0x0F12, 0x2C10},  //70003DA4 
{0x0F12, 0xDA03},  //70003DA6 
{0x0F12, 0x8839},  //70003DA8 
{0x0F12, 0x43A9},  //70003DAA 
{0x0F12, 0x8039},  //70003DAC 
{0x0F12, 0xE002},  //70003DAE 
{0x0F12, 0x8879},  //70003DB0 
{0x0F12, 0x43B1},  //70003DB2 
{0x0F12, 0x8079},  //70003DB4 
{0x0F12, 0xF000},  //70003DB6 
{0x0F12, 0xFB31},  //70003DB8 
{0x0F12, 0x2C10},  //70003DBA 
{0x0F12, 0xDA03},  //70003DBC 
{0x0F12, 0x8838},  //70003DBE 
{0x0F12, 0x4328},  //70003DC0 
{0x0F12, 0x8038},  //70003DC2 
{0x0F12, 0xE002},  //70003DC4 
{0x0F12, 0x8878},  //70003DC6 
{0x0F12, 0x4330},  //70003DC8 
{0x0F12, 0x8078},  //70003DCA 
{0x0F12, 0x48B6},  //70003DCC 
{0x0F12, 0x8800},  //70003DCE 
{0x0F12, 0x0400},  //70003DD0 
{0x0F12, 0xD507},  //70003DD2 
{0x0F12, 0x4BB5},  //70003DD4 
{0x0F12, 0x7819},  //70003DD6 
{0x0F12, 0x4AB5},  //70003DD8 
{0x0F12, 0x7810},  //70003DDA 
{0x0F12, 0x7018},  //70003DDC 
{0x0F12, 0x7011},  //70003DDE 
{0x0F12, 0x49B4},  //70003DE0 
{0x0F12, 0x8188},  //70003DE2 
{0x0F12, 0xBCF8},  //70003DE4 
{0x0F12, 0xBC08},  //70003DE6 
{0x0F12, 0x4718},  //70003DE8 
{0x0F12, 0xB538},  //70003DEA 
{0x0F12, 0x48B2},  //70003DEC 
{0x0F12, 0x4669},  //70003DEE 
{0x0F12, 0xF000},  //70003DF0 
{0x0F12, 0xFB1C},  //70003DF2 
{0x0F12, 0x48B1},  //70003DF4 
{0x0F12, 0x49B0},  //70003DF6 
{0x0F12, 0x69C2},  //70003DF8 
{0x0F12, 0x2400},  //70003DFA 
{0x0F12, 0x31A8},  //70003DFC 
{0x0F12, 0x2A00},  //70003DFE 
{0x0F12, 0xD008},  //70003E00 
{0x0F12, 0x61C4},  //70003E02 
{0x0F12, 0x684A},  //70003E04 
{0x0F12, 0x6242},  //70003E06 
{0x0F12, 0x6282},  //70003E08 
{0x0F12, 0x466B},  //70003E0A 
{0x0F12, 0x881A},  //70003E0C 
{0x0F12, 0x6302},  //70003E0E 
{0x0F12, 0x885A},  //70003E10 
{0x0F12, 0x6342},  //70003E12 
{0x0F12, 0x6A02},  //70003E14 
{0x0F12, 0x2A00},  //70003E16 
{0x0F12, 0xD00A},  //70003E18 
{0x0F12, 0x6204},  //70003E1A 
{0x0F12, 0x6849},  //70003E1C 
{0x0F12, 0x6281},  //70003E1E 
{0x0F12, 0x466B},  //70003E20 
{0x0F12, 0x8819},  //70003E22 
{0x0F12, 0x6301},  //70003E24 
{0x0F12, 0x8859},  //70003E26 
{0x0F12, 0x6341},  //70003E28 
{0x0F12, 0x49A5},  //70003E2A 
{0x0F12, 0x88C9},  //70003E2C 
{0x0F12, 0x63C1},  //70003E2E 
{0x0F12, 0xF000},  //70003E30 
{0x0F12, 0xFB04},  //70003E32 
{0x0F12, 0xE7A6},  //70003E34 
{0x0F12, 0xB5F0},  //70003E36 
{0x0F12, 0xB08B},  //70003E38 
{0x0F12, 0x20FF},  //70003E3A 
{0x0F12, 0x1C40},  //70003E3C 
{0x0F12, 0x49A1},  //70003E3E 
{0x0F12, 0x89CC},  //70003E40 
{0x0F12, 0x4E9E},  //70003E42 
{0x0F12, 0x6AB1},  //70003E44 
{0x0F12, 0x4284},  //70003E46 
{0x0F12, 0xD101},  //70003E48 
{0x0F12, 0x489F},  //70003E4A 
{0x0F12, 0x6081},  //70003E4C 
{0x0F12, 0x6A70},  //70003E4E 
{0x0F12, 0x0200},  //70003E50 
{0x0F12, 0xF000},  //70003E52 
{0x0F12, 0xFAFB},  //70003E54 
{0x0F12, 0x0400},  //70003E56 
{0x0F12, 0x0C00},  //70003E58 
{0x0F12, 0x4A96},  //70003E5A 
{0x0F12, 0x8A11},  //70003E5C 
{0x0F12, 0x9109},  //70003E5E 
{0x0F12, 0x2101},  //70003E60 
{0x0F12, 0x0349},  //70003E62 
{0x0F12, 0x4288},  //70003E64 
{0x0F12, 0xD200},  //70003E66 
{0x0F12, 0x0001},  //70003E68 
{0x0F12, 0x4A92},  //70003E6A 
{0x0F12, 0x8211},  //70003E6C 
{0x0F12, 0x4D97},  //70003E6E 
{0x0F12, 0x8829},  //70003E70 
{0x0F12, 0x9108},  //70003E72 
{0x0F12, 0x4A8B},  //70003E74 
{0x0F12, 0x2303},  //70003E76 
{0x0F12, 0x3222},  //70003E78 
{0x0F12, 0x1F91},  //70003E7A 
{0x0F12, 0xF000},  //70003E7C 
{0x0F12, 0xFAEC},  //70003E7E 
{0x0F12, 0x8028},  //70003E80 
{0x0F12, 0x488E},  //70003E82 
{0x0F12, 0x4987},  //70003E84 
{0x0F12, 0x6BC2},  //70003E86 
{0x0F12, 0x6AC0},  //70003E88 
{0x0F12, 0x4282},  //70003E8A 
{0x0F12, 0xD201},  //70003E8C 
{0x0F12, 0x8CC8},  //70003E8E 
{0x0F12, 0x8028},  //70003E90 
{0x0F12, 0x88E8},  //70003E92 
{0x0F12, 0x9007},  //70003E94 
{0x0F12, 0x2240},  //70003E96 
{0x0F12, 0x4310},  //70003E98 
{0x0F12, 0x80E8},  //70003E9A 
{0x0F12, 0x2000},  //70003E9C 
{0x0F12, 0x0041},  //70003E9E 
{0x0F12, 0x194B},  //70003EA0 
{0x0F12, 0x001E},  //70003EA2 
{0x0F12, 0x3680},  //70003EA4 
{0x0F12, 0x8BB2},  //70003EA6 
{0x0F12, 0xAF04},  //70003EA8 
{0x0F12, 0x527A},  //70003EAA 
{0x0F12, 0x4A7D},  //70003EAC 
{0x0F12, 0x188A},  //70003EAE 
{0x0F12, 0x8897},  //70003EB0 
{0x0F12, 0x83B7},  //70003EB2 
{0x0F12, 0x33A0},  //70003EB4 
{0x0F12, 0x891F},  //70003EB6 
{0x0F12, 0xAE01},  //70003EB8 
{0x0F12, 0x5277},  //70003EBA 
{0x0F12, 0x8A11},  //70003EBC 
{0x0F12, 0x8119},  //70003EBE 
{0x0F12, 0x1C40},  //70003EC0 
{0x0F12, 0x0400},  //70003EC2 
{0x0F12, 0x0C00},  //70003EC4 
{0x0F12, 0x2806},  //70003EC6 
{0x0F12, 0xD3E9},  //70003EC8 
{0x0F12, 0xF000},  //70003ECA 
{0x0F12, 0xFACD},  //70003ECC 
{0x0F12, 0xF000},  //70003ECE 
{0x0F12, 0xFAD3},  //70003ED0 
{0x0F12, 0x4F79},  //70003ED2 
{0x0F12, 0x37A8},  //70003ED4 
{0x0F12, 0x2800},  //70003ED6 
{0x0F12, 0xD10A},  //70003ED8 
{0x0F12, 0x1FE0},  //70003EDA 
{0x0F12, 0x38FD},  //70003EDC 
{0x0F12, 0xD001},  //70003EDE 
{0x0F12, 0x1CC0},  //70003EE0 
{0x0F12, 0xD105},  //70003EE2 
{0x0F12, 0x4874},  //70003EE4 
{0x0F12, 0x8829},  //70003EE6 
{0x0F12, 0x3818},  //70003EE8 
{0x0F12, 0x6840},  //70003EEA 
{0x0F12, 0x4348},  //70003EEC 
{0x0F12, 0x6078},  //70003EEE 
{0x0F12, 0x4972},  //70003EF0 
{0x0F12, 0x6878},  //70003EF2 
{0x0F12, 0x6B89},  //70003EF4 
{0x0F12, 0x4288},  //70003EF6 
{0x0F12, 0xD300},  //70003EF8 
{0x0F12, 0x0008},  //70003EFA 
{0x0F12, 0x6078},  //70003EFC 
{0x0F12, 0x2000},  //70003EFE 
{0x0F12, 0x0041},  //70003F00 
{0x0F12, 0xAA04},  //70003F02 
{0x0F12, 0x5A53},  //70003F04 
{0x0F12, 0x194A},  //70003F06 
{0x0F12, 0x269C},  //70003F08 
{0x0F12, 0x52B3},  //70003F0A 
{0x0F12, 0xAB01},  //70003F0C 
{0x0F12, 0x5A59},  //70003F0E 
{0x0F12, 0x32A0},  //70003F10 
{0x0F12, 0x8111},  //70003F12 
{0x0F12, 0x1C40},  //70003F14 
{0x0F12, 0x0400},  //70003F16 
{0x0F12, 0x0C00},  //70003F18 
{0x0F12, 0x2806},  //70003F1A 
{0x0F12, 0xD3F0},  //70003F1C 
{0x0F12, 0x4965},  //70003F1E 
{0x0F12, 0x9809},  //70003F20 
{0x0F12, 0x8208},  //70003F22 
{0x0F12, 0x9808},  //70003F24 
{0x0F12, 0x8028},  //70003F26 
{0x0F12, 0x9807},  //70003F28 
{0x0F12, 0x80E8},  //70003F2A 
{0x0F12, 0x1FE0},  //70003F2C 
{0x0F12, 0x38FD},  //70003F2E 
{0x0F12, 0xD13B},  //70003F30 
{0x0F12, 0x4D64},  //70003F32 
{0x0F12, 0x89E8},  //70003F34 
{0x0F12, 0x1FC1},  //70003F36 
{0x0F12, 0x39FF},  //70003F38 
{0x0F12, 0xD136},  //70003F3A 
{0x0F12, 0x4C5F},  //70003F3C 
{0x0F12, 0x8AE0},  //70003F3E 
{0x0F12, 0xF000},  //70003F40 
{0x0F12, 0xFAA2},  //70003F42 
{0x0F12, 0x0006},  //70003F44 
{0x0F12, 0x8B20},  //70003F46 
{0x0F12, 0xF000},  //70003F48 
{0x0F12, 0xFAA6},  //70003F4A 
{0x0F12, 0x9000},  //70003F4C 
{0x0F12, 0x6AA1},  //70003F4E 
{0x0F12, 0x6878},  //70003F50 
{0x0F12, 0x1809},  //70003F52 
{0x0F12, 0x0200},  //70003F54 
{0x0F12, 0xF000},  //70003F56 
{0x0F12, 0xFA79},  //70003F58 
{0x0F12, 0x0400},  //70003F5A 
{0x0F12, 0x0C00},  //70003F5C 
{0x0F12, 0x0022},  //70003F5E 
{0x0F12, 0x3246},  //70003F60 
{0x0F12, 0x0011},  //70003F62 
{0x0F12, 0x310A},  //70003F64 
{0x0F12, 0x2305},  //70003F66 
{0x0F12, 0xF000},  //70003F68 
{0x0F12, 0xFA76},  //70003F6A 
{0x0F12, 0x66E8},  //70003F6C 
{0x0F12, 0x6B23},  //70003F6E 
{0x0F12, 0x0002},  //70003F70 
{0x0F12, 0x0031},  //70003F72 
{0x0F12, 0x0018},  //70003F74 
{0x0F12, 0xF000},  //70003F76 
{0x0F12, 0xFA97},  //70003F78 
{0x0F12, 0x466B},  //70003F7A 
{0x0F12, 0x8518},  //70003F7C 
{0x0F12, 0x6EEA},  //70003F7E 
{0x0F12, 0x6B60},  //70003F80 
{0x0F12, 0x9900},  //70003F82 
{0x0F12, 0xF000},  //70003F84 
{0x0F12, 0xFA90},  //70003F86 
{0x0F12, 0x466B},  //70003F88 
{0x0F12, 0x8558},  //70003F8A 
{0x0F12, 0x0029},  //70003F8C 
{0x0F12, 0x980A},  //70003F8E 
{0x0F12, 0x3170},  //70003F90 
{0x0F12, 0xF000},  //70003F92 
{0x0F12, 0xFA91},  //70003F94 
{0x0F12, 0x0028},  //70003F96 
{0x0F12, 0x3060},  //70003F98 
{0x0F12, 0x8A02},  //70003F9A 
{0x0F12, 0x4946},  //70003F9C 
{0x0F12, 0x3128},  //70003F9E 
{0x0F12, 0x808A},  //70003FA0 
{0x0F12, 0x8A42},  //70003FA2 
{0x0F12, 0x80CA},  //70003FA4 
{0x0F12, 0x8A80},  //70003FA6 
{0x0F12, 0x8108},  //70003FA8 
{0x0F12, 0xB00B},  //70003FAA 
{0x0F12, 0xBCF0},  //70003FAC 
{0x0F12, 0xBC08},  //70003FAE 
{0x0F12, 0x4718},  //70003FB0 
{0x0F12, 0xB570},  //70003FB2 
{0x0F12, 0x2400},  //70003FB4 
{0x0F12, 0x4D46},  //70003FB6 
{0x0F12, 0x4846},  //70003FB8 
{0x0F12, 0x8881},  //70003FBA 
{0x0F12, 0x4846},  //70003FBC 
{0x0F12, 0x8041},  //70003FBE 
{0x0F12, 0x2101},  //70003FC0 
{0x0F12, 0x8001},  //70003FC2 
{0x0F12, 0xF000},  //70003FC4 
{0x0F12, 0xFA80},  //70003FC6 
{0x0F12, 0x4842},  //70003FC8 
{0x0F12, 0x3820},  //70003FCA 
{0x0F12, 0x8BC0},  //70003FCC 
{0x0F12, 0xF000},  //70003FCE 
{0x0F12, 0xFA83},  //70003FD0 
{0x0F12, 0x4B42},  //70003FD2 
{0x0F12, 0x220D},  //70003FD4 
{0x0F12, 0x0712},  //70003FD6 
{0x0F12, 0x18A8},  //70003FD8 
{0x0F12, 0x8806},  //70003FDA 
{0x0F12, 0x00E1},  //70003FDC 
{0x0F12, 0x18C9},  //70003FDE 
{0x0F12, 0x81CE},  //70003FE0 
{0x0F12, 0x8846},  //70003FE2 
{0x0F12, 0x818E},  //70003FE4 
{0x0F12, 0x8886},  //70003FE6 
{0x0F12, 0x824E},  //70003FE8 
{0x0F12, 0x88C0},  //70003FEA 
{0x0F12, 0x8208},  //70003FEC 
{0x0F12, 0x3508},  //70003FEE 
{0x0F12, 0x042D},  //70003FF0 
{0x0F12, 0x0C2D},  //70003FF2 
{0x0F12, 0x1C64},  //70003FF4 
{0x0F12, 0x0424},  //70003FF6 
{0x0F12, 0x0C24},  //70003FF8 
{0x0F12, 0x2C07},  //70003FFA 
{0x0F12, 0xD3EC},  //70003FFC 
{0x0F12, 0xE658},  //70003FFE 
{0x0F12, 0xB510},  //70004000 
{0x0F12, 0x4834},  //70004002 
{0x0F12, 0x4C34},  //70004004 
{0x0F12, 0x88C0},  //70004006 
{0x0F12, 0x8060},  //70004008 
{0x0F12, 0x2001},  //7000400A 
{0x0F12, 0x8020},  //7000400C 
{0x0F12, 0x4831},  //7000400E 
{0x0F12, 0x3820},  //70004010 
{0x0F12, 0x8BC0},  //70004012 
{0x0F12, 0xF000},  //70004014 
{0x0F12, 0xFA60},  //70004016 
{0x0F12, 0x88E0},  //70004018 
{0x0F12, 0x4A31},  //7000401A 
{0x0F12, 0x2800},  //7000401C 
{0x0F12, 0xD003},  //7000401E 
{0x0F12, 0x4930},  //70004020 
{0x0F12, 0x8849},  //70004022 
{0x0F12, 0x2900},  //70004024 
{0x0F12, 0xD009},  //70004026 
{0x0F12, 0x2001},  //70004028 
{0x0F12, 0x03C0},  //7000402A 
{0x0F12, 0x8050},  //7000402C 
{0x0F12, 0x80D0},  //7000402E 
{0x0F12, 0x2000},  //70004030 
{0x0F12, 0x8090},  //70004032 
{0x0F12, 0x8110},  //70004034 
{0x0F12, 0xBC10},  //70004036 
{0x0F12, 0xBC08},  //70004038 
{0x0F12, 0x4718},  //7000403A 
{0x0F12, 0x8050},  //7000403C 
{0x0F12, 0x8920},  //7000403E 
{0x0F12, 0x80D0},  //70004040 
{0x0F12, 0x8960},  //70004042 
{0x0F12, 0x0400},  //70004044 
{0x0F12, 0x1400},  //70004046 
{0x0F12, 0x8090},  //70004048 
{0x0F12, 0x89A1},  //7000404A 
{0x0F12, 0x0409},  //7000404C 
{0x0F12, 0x1409},  //7000404E 
{0x0F12, 0x8111},  //70004050 
{0x0F12, 0x89E3},  //70004052 
{0x0F12, 0x8A24},  //70004054 
{0x0F12, 0x2B00},  //70004056 
{0x0F12, 0xD104},  //70004058 
{0x0F12, 0x17C3},  //7000405A 
{0x0F12, 0x0F5B},  //7000405C 
{0x0F12, 0x1818},  //7000405E 
{0x0F12, 0x10C0},  //70004060 
{0x0F12, 0x8090},  //70004062 
{0x0F12, 0x2C00},  //70004064 
{0x0F12, 0xD1E6},  //70004066 
{0x0F12, 0x17C8},  //70004068 
{0x0F12, 0x0F40},  //7000406A 
{0x0F12, 0x1840},  //7000406C 
{0x0F12, 0x10C0},  //7000406E 
{0x0F12, 0x8110},  //70004070 
{0x0F12, 0xE7E0},  //70004072 
{0x0F12, 0xB510},  //70004074 
{0x0F12, 0x000C},  //70004076 
{0x0F12, 0x4919},  //70004078 
{0x0F12, 0x2204},  //7000407A 
{0x0F12, 0x6820},  //7000407C 
{0x0F12, 0x5E8A},  //7000407E 
{0x0F12, 0x0140},  //70004080 
{0x0F12, 0x1A80},  //70004082 
{0x0F12, 0x0280},  //70004084 
{0x0F12, 0x8849},  //70004086 
{0x0F12, 0xF000},  //70004088 
{0x0F12, 0xFA2E},  //7000408A 
{0x0F12, 0x6020},  //7000408C 
{0x0F12, 0xE7D2},  //7000408E 
{0x0F12, 0x38D4},  //70004090 
{0x0F12, 0x7000},  //70004092 
{0x0F12, 0x17D0},  //70004094 
{0x0F12, 0x7000},  //70004096 
{0x0F12, 0x5000},  //70004098 
{0x0F12, 0xD000},  //7000409A 
{0x0F12, 0x1100},  //7000409C 
{0x0F12, 0xD000},  //7000409E 
{0x0F12, 0x171A},  //700040A0 
{0x0F12, 0x7000},  //700040A2 
{0x0F12, 0x4780},  //700040A4 
{0x0F12, 0x7000},  //700040A6 
{0x0F12, 0x2FCA},  //700040A8 
{0x0F12, 0x7000},  //700040AA 
{0x0F12, 0x2FC5},  //700040AC 
{0x0F12, 0x7000},  //700040AE 
{0x0F12, 0x2FC6},  //700040B0 
{0x0F12, 0x7000},  //700040B2 
{0x0F12, 0x2ED8},  //700040B4 
{0x0F12, 0x7000},  //700040B6 
{0x0F12, 0x2BD0},  //700040B8 
{0x0F12, 0x7000},  //700040BA 
{0x0F12, 0x17E0},  //700040BC 
{0x0F12, 0x7000},  //700040BE 
{0x0F12, 0x2DE8},  //700040C0 
{0x0F12, 0x7000},  //700040C2 
{0x0F12, 0x37E0},  //700040C4 
{0x0F12, 0x7000},  //700040C6 
{0x0F12, 0x210C},  //700040C8 
{0x0F12, 0x7000},  //700040CA 
{0x0F12, 0x1484},  //700040CC 
{0x0F12, 0x7000},  //700040CE 
{0x0F12, 0xA006},  //700040D0 
{0x0F12, 0x0000},  //700040D2 
{0x0F12, 0x0724},  //700040D4 
{0x0F12, 0x7000},  //700040D6 
{0x0F12, 0xA000},  //700040D8 
{0x0F12, 0xD000},  //700040DA 
{0x0F12, 0x2270},  //700040DC 
{0x0F12, 0x7000},  //700040DE 
{0x0F12, 0x2558},  //700040E0 
{0x0F12, 0x7000},  //700040E2 
{0x0F12, 0x146C},  //700040E4 
{0x0F12, 0x7000},  //700040E6 
{0x0F12, 0xB510},  //700040E8 
{0x0F12, 0x000C},  //700040EA 
{0x0F12, 0x49AC},  //700040EC 
{0x0F12, 0x2208},  //700040EE 
{0x0F12, 0x6820},  //700040F0 
{0x0F12, 0x5E8A},  //700040F2 
{0x0F12, 0x0140},  //700040F4 
{0x0F12, 0x1A80},  //700040F6 
{0x0F12, 0x0280},  //700040F8 
{0x0F12, 0x88C9},  //700040FA 
{0x0F12, 0xF000},  //700040FC 
{0x0F12, 0xF9F4},  //700040FE 
{0x0F12, 0x6020},  //70004100 
{0x0F12, 0xE798},  //70004102 
{0x0F12, 0xB5FE},  //70004104 
{0x0F12, 0x000C},  //70004106 
{0x0F12, 0x6825},  //70004108 
{0x0F12, 0x6866},  //7000410A 
{0x0F12, 0x68A0},  //7000410C 
{0x0F12, 0x9001},  //7000410E 
{0x0F12, 0x68E7},  //70004110 
{0x0F12, 0x1BA8},  //70004112 
{0x0F12, 0x42B5},  //70004114 
{0x0F12, 0xDA00},  //70004116 
{0x0F12, 0x1B70},  //70004118 
{0x0F12, 0x9000},  //7000411A 
{0x0F12, 0x49A0},  //7000411C 
{0x0F12, 0x48A1},  //7000411E 
{0x0F12, 0x884A},  //70004120 
{0x0F12, 0x8843},  //70004122 
{0x0F12, 0x435A},  //70004124 
{0x0F12, 0x2304},  //70004126 
{0x0F12, 0x5ECB},  //70004128 
{0x0F12, 0x0A92},  //7000412A 
{0x0F12, 0x18D2},  //7000412C 
{0x0F12, 0x02D2},  //7000412E 
{0x0F12, 0x0C12},  //70004130 
{0x0F12, 0x88CB},  //70004132 
{0x0F12, 0x8880},  //70004134 
{0x0F12, 0x4343},  //70004136 
{0x0F12, 0x0A98},  //70004138 
{0x0F12, 0x2308},  //7000413A 
{0x0F12, 0x5ECB},  //7000413C 
{0x0F12, 0x18C0},  //7000413E 
{0x0F12, 0x02C0},  //70004140 
{0x0F12, 0x0C00},  //70004142 
{0x0F12, 0x0411},  //70004144 
{0x0F12, 0x0400},  //70004146 
{0x0F12, 0x1409},  //70004148 
{0x0F12, 0x1400},  //7000414A 
{0x0F12, 0x1A08},  //7000414C 
{0x0F12, 0x4995},  //7000414E 
{0x0F12, 0x39E0},  //70004150 
{0x0F12, 0x6148},  //70004152 
{0x0F12, 0x9801},  //70004154 
{0x0F12, 0x3040},  //70004156 
{0x0F12, 0x7880},  //70004158 
{0x0F12, 0x2800},  //7000415A 
{0x0F12, 0xD103},  //7000415C 
{0x0F12, 0x9801},  //7000415E 
{0x0F12, 0x0029},  //70004160 
{0x0F12, 0xF000},  //70004162 
{0x0F12, 0xF9C7},  //70004164 
{0x0F12, 0x8839},  //70004166 
{0x0F12, 0x9800},  //70004168 
{0x0F12, 0x4281},  //7000416A 
{0x0F12, 0xD814},  //7000416C 
{0x0F12, 0x8879},  //7000416E 
{0x0F12, 0x9800},  //70004170 
{0x0F12, 0x4281},  //70004172 
{0x0F12, 0xD20C},  //70004174 
{0x0F12, 0x9801},  //70004176 
{0x0F12, 0x0029},  //70004178 
{0x0F12, 0xF000},  //7000417A 
{0x0F12, 0xF9C3},  //7000417C 
{0x0F12, 0x9801},  //7000417E 
{0x0F12, 0x0029},  //70004180 
{0x0F12, 0xF000},  //70004182 
{0x0F12, 0xF9BF},  //70004184 
{0x0F12, 0x9801},  //70004186 
{0x0F12, 0x0029},  //70004188 
{0x0F12, 0xF000},  //7000418A 
{0x0F12, 0xF9BB},  //7000418C 
{0x0F12, 0xE003},  //7000418E 
{0x0F12, 0x9801},  //70004190 
{0x0F12, 0x0029},  //70004192 
{0x0F12, 0xF000},  //70004194 
{0x0F12, 0xF9B6},  //70004196 
{0x0F12, 0x9801},  //70004198 
{0x0F12, 0x0032},  //7000419A 
{0x0F12, 0x0039},  //7000419C 
{0x0F12, 0xF000},  //7000419E 
{0x0F12, 0xF9B9},  //700041A0 
{0x0F12, 0x6020},  //700041A2 
{0x0F12, 0xE5D0},  //700041A4 
{0x0F12, 0xB57C},  //700041A6 
{0x0F12, 0x487F},  //700041A8 
{0x0F12, 0xA901},  //700041AA 
{0x0F12, 0x0004},  //700041AC 
{0x0F12, 0xF000},  //700041AE 
{0x0F12, 0xF93D},  //700041B0 
{0x0F12, 0x466B},  //700041B2 
{0x0F12, 0x88D9},  //700041B4 
{0x0F12, 0x8898},  //700041B6 
{0x0F12, 0x4B7A},  //700041B8 
{0x0F12, 0x3346},  //700041BA 
{0x0F12, 0x1E9A},  //700041BC 
{0x0F12, 0xF000},  //700041BE 
{0x0F12, 0xF9B1},  //700041C0 
{0x0F12, 0x4879},  //700041C2 
{0x0F12, 0x4977},  //700041C4 
{0x0F12, 0x3812},  //700041C6 
{0x0F12, 0x3140},  //700041C8 
{0x0F12, 0x8A42},  //700041CA 
{0x0F12, 0x888B},  //700041CC 
{0x0F12, 0x18D2},  //700041CE 
{0x0F12, 0x8242},  //700041D0 
{0x0F12, 0x8AC2},  //700041D2 
{0x0F12, 0x88C9},  //700041D4 
{0x0F12, 0x1851},  //700041D6 
{0x0F12, 0x82C1},  //700041D8 
{0x0F12, 0x0020},  //700041DA 
{0x0F12, 0x4669},  //700041DC 
{0x0F12, 0xF000},  //700041DE 
{0x0F12, 0xF925},  //700041E0 
{0x0F12, 0x4872},  //700041E2 
{0x0F12, 0x214D},  //700041E4 
{0x0F12, 0x8301},  //700041E6 
{0x0F12, 0x2196},  //700041E8 
{0x0F12, 0x8381},  //700041EA 
{0x0F12, 0x211D},  //700041EC 
{0x0F12, 0x3020},  //700041EE 
{0x0F12, 0x8001},  //700041F0 
{0x0F12, 0xF000},  //700041F2 
{0x0F12, 0xF99F},  //700041F4 
{0x0F12, 0xF000},  //700041F6 
{0x0F12, 0xF9A5},  //700041F8 
{0x0F12, 0x486D},  //700041FA 
{0x0F12, 0x4C6D},  //700041FC 
{0x0F12, 0x6E00},  //700041FE 
{0x0F12, 0x60E0},  //70004200 
{0x0F12, 0x466B},  //70004202 
{0x0F12, 0x8818},  //70004204 
{0x0F12, 0x8859},  //70004206 
{0x0F12, 0x0025},  //70004208 
{0x0F12, 0x1A40},  //7000420A 
{0x0F12, 0x3540},  //7000420C 
{0x0F12, 0x61A8},  //7000420E 
{0x0F12, 0x4864},  //70004210 
{0x0F12, 0x9900},  //70004212 
{0x0F12, 0x3060},  //70004214 
{0x0F12, 0xF000},  //70004216 
{0x0F12, 0xF99D},  //70004218 
{0x0F12, 0x466B},  //7000421A 
{0x0F12, 0x8819},  //7000421C 
{0x0F12, 0x1DE0},  //7000421E 
{0x0F12, 0x30F9},  //70004220 
{0x0F12, 0x8741},  //70004222 
{0x0F12, 0x8859},  //70004224 
{0x0F12, 0x8781},  //70004226 
{0x0F12, 0x2000},  //70004228 
{0x0F12, 0x71A0},  //7000422A 
{0x0F12, 0x74A8},  //7000422C 
{0x0F12, 0xBC7C},  //7000422E 
{0x0F12, 0xBC08},  //70004230 
{0x0F12, 0x4718},  //70004232 
{0x0F12, 0xB5F8},  //70004234 
{0x0F12, 0x0005},  //70004236 
{0x0F12, 0x6808},  //70004238 
{0x0F12, 0x0400},  //7000423A 
{0x0F12, 0x0C00},  //7000423C 
{0x0F12, 0x684A},  //7000423E 
{0x0F12, 0x0412},  //70004240 
{0x0F12, 0x0C12},  //70004242 
{0x0F12, 0x688E},  //70004244 
{0x0F12, 0x68CC},  //70004246 
{0x0F12, 0x4955},  //70004248 
{0x0F12, 0x884B},  //7000424A 
{0x0F12, 0x4343},  //7000424C 
{0x0F12, 0x0A98},  //7000424E 
{0x0F12, 0x2304},  //70004250 
{0x0F12, 0x5ECB},  //70004252 
{0x0F12, 0x18C0},  //70004254 
{0x0F12, 0x02C0},  //70004256 
{0x0F12, 0x0C00},  //70004258 
{0x0F12, 0x88CB},  //7000425A 
{0x0F12, 0x4353},  //7000425C 
{0x0F12, 0x0A9A},  //7000425E 
{0x0F12, 0x2308},  //70004260 
{0x0F12, 0x5ECB},  //70004262 
{0x0F12, 0x18D1},  //70004264 
{0x0F12, 0x02C9},  //70004266 
{0x0F12, 0x0C09},  //70004268 
{0x0F12, 0x2701},  //7000426A 
{0x0F12, 0x003A},  //7000426C 
{0x0F12, 0x40AA},  //7000426E 
{0x0F12, 0x9200},  //70004270 
{0x0F12, 0x002A},  //70004272 
{0x0F12, 0x3A10},  //70004274 
{0x0F12, 0x4097},  //70004276 
{0x0F12, 0x2D10},  //70004278 
{0x0F12, 0xDA06},  //7000427A 
{0x0F12, 0x4A4E},  //7000427C 
{0x0F12, 0x9B00},  //7000427E 
{0x0F12, 0x8812},  //70004280 
{0x0F12, 0x439A},  //70004282 
{0x0F12, 0x4B4C},  //70004284 
{0x0F12, 0x801A},  //70004286 
{0x0F12, 0xE003},  //70004288 
{0x0F12, 0x4B4B},  //7000428A 
{0x0F12, 0x885A},  //7000428C 
{0x0F12, 0x43BA},  //7000428E 
{0x0F12, 0x805A},  //70004290 
{0x0F12, 0x0023},  //70004292 
{0x0F12, 0x0032},  //70004294 
{0x0F12, 0xF000},  //70004296 
{0x0F12, 0xF945},  //70004298 
{0x0F12, 0x2D10},  //7000429A 
{0x0F12, 0xDA05},  //7000429C 
{0x0F12, 0x4946},  //7000429E 
{0x0F12, 0x9A00},  //700042A0 
{0x0F12, 0x8808},  //700042A2 
{0x0F12, 0x4310},  //700042A4 
{0x0F12, 0x8008},  //700042A6 
{0x0F12, 0xE003},  //700042A8 
{0x0F12, 0x4843},  //700042AA 
{0x0F12, 0x8841},  //700042AC 
{0x0F12, 0x4339},  //700042AE 
{0x0F12, 0x8041},  //700042B0 
{0x0F12, 0x4D40},  //700042B2 
{0x0F12, 0x2000},  //700042B4 
{0x0F12, 0x3580},  //700042B6 
{0x0F12, 0x88AA},  //700042B8 
{0x0F12, 0x5E30},  //700042BA 
{0x0F12, 0x2100},  //700042BC 
{0x0F12, 0xF000},  //700042BE 
{0x0F12, 0xF951},  //700042C0 
{0x0F12, 0x8030},  //700042C2 
{0x0F12, 0x2000},  //700042C4 
{0x0F12, 0x88AA},  //700042C6 
{0x0F12, 0x5E20},  //700042C8 
{0x0F12, 0x2100},  //700042CA 
{0x0F12, 0xF000},  //700042CC 
{0x0F12, 0xF94A},  //700042CE 
{0x0F12, 0x8020},  //700042D0 
{0x0F12, 0xE587},  //700042D2 
{0x0F12, 0xB510},  //700042D4 
{0x0F12, 0xF000},  //700042D6 
{0x0F12, 0xF94D},  //700042D8 
{0x0F12, 0x4A38},  //700042DA 
{0x0F12, 0x8D50},  //700042DC 
{0x0F12, 0x2800},  //700042DE 
{0x0F12, 0xD007},  //700042E0 
{0x0F12, 0x4933},  //700042E2 
{0x0F12, 0x31C0},  //700042E4 
{0x0F12, 0x684B},  //700042E6 
{0x0F12, 0x4935},  //700042E8 
{0x0F12, 0x4283},  //700042EA 
{0x0F12, 0xD202},  //700042EC 
{0x0F12, 0x8D90},  //700042EE 
{0x0F12, 0x81C8},  //700042F0 
{0x0F12, 0xE6A0},  //700042F2 
{0x0F12, 0x8DD0},  //700042F4 
{0x0F12, 0x81C8},  //700042F6 
{0x0F12, 0xE69D},  //700042F8 
{0x0F12, 0xB5F8},  //700042FA 
{0x0F12, 0xF000},  //700042FC 
{0x0F12, 0xF942},  //700042FE 
{0x0F12, 0x4D2E},  //70004300 
{0x0F12, 0x8E28},  //70004302 
{0x0F12, 0x2800},  //70004304 
{0x0F12, 0xD01F},  //70004306 
{0x0F12, 0x4E2E},  //70004308 
{0x0F12, 0x4829},  //7000430A 
{0x0F12, 0x68B4},  //7000430C 
{0x0F12, 0x6800},  //7000430E 
{0x0F12, 0x4284},  //70004310 
{0x0F12, 0xD903},  //70004312 
{0x0F12, 0x1A21},  //70004314 
{0x0F12, 0x0849},  //70004316 
{0x0F12, 0x1847},  //70004318 
{0x0F12, 0xE006},  //7000431A 
{0x0F12, 0x4284},  //7000431C 
{0x0F12, 0xD203},  //7000431E 
{0x0F12, 0x1B01},  //70004320 
{0x0F12, 0x0849},  //70004322 
{0x0F12, 0x1A47},  //70004324 
{0x0F12, 0xE000},  //70004326 
{0x0F12, 0x0027},  //70004328 
{0x0F12, 0x0020},  //7000432A 
{0x0F12, 0x4920},  //7000432C 
{0x0F12, 0x3120},  //7000432E 
{0x0F12, 0x7A0C},  //70004330 
{0x0F12, 0x2C00},  //70004332 
{0x0F12, 0xD004},  //70004334 
{0x0F12, 0x0200},  //70004336 
{0x0F12, 0x0039},  //70004338 
{0x0F12, 0xF000},  //7000433A 
{0x0F12, 0xF887},  //7000433C 
{0x0F12, 0x8668},  //7000433E 
{0x0F12, 0x2C00},  //70004340 
{0x0F12, 0xD000},  //70004342 
{0x0F12, 0x60B7},  //70004344 
{0x0F12, 0xE54D},  //70004346 
{0x0F12, 0x20FF},  //70004348 
{0x0F12, 0x1C40},  //7000434A 
{0x0F12, 0x8668},  //7000434C 
{0x0F12, 0xE549},  //7000434E 
{0x0F12, 0xB510},  //70004350 
{0x0F12, 0x000C},  //70004352 
{0x0F12, 0x6820},  //70004354 
{0x0F12, 0x0400},  //70004356 
{0x0F12, 0x0C00},  //70004358 
{0x0F12, 0x4918},  //7000435A 
{0x0F12, 0x8E0A},  //7000435C 
{0x0F12, 0x2A00},  //7000435E 
{0x0F12, 0xD003},  //70004360 
{0x0F12, 0x8E49},  //70004362 
{0x0F12, 0x0200},  //70004364 
{0x0F12, 0xF000},  //70004366 
{0x0F12, 0xF871},  //70004368 
{0x0F12, 0x6020},  //7000436A 
{0x0F12, 0x0400},  //7000436C 
{0x0F12, 0x0C00},  //7000436E 
{0x0F12, 0xE661},  //70004370 
{0x0F12, 0xB570},  //70004372 
{0x0F12, 0x680C},  //70004374 
{0x0F12, 0x4D14},  //70004376 
{0x0F12, 0x0020},  //70004378 
{0x0F12, 0x6F29},  //7000437A 
{0x0F12, 0xF000},  //7000437C 
{0x0F12, 0xF90A},  //7000437E 
{0x0F12, 0x6F69},  //70004380 
{0x0F12, 0x1D20},  //70004382 
{0x0F12, 0xF000},  //70004384 
{0x0F12, 0xF906},  //70004386 
{0x0F12, 0x480C},  //70004388 
{0x0F12, 0x8E00},  //7000438A 
{0x0F12, 0x2800},  //7000438C 
{0x0F12, 0xD006},  //7000438E 
{0x0F12, 0x4907},  //70004390 
{0x0F12, 0x2214},  //70004392 
{0x0F12, 0x3168},  //70004394 
{0x0F12, 0x0008},  //70004396 
{0x0F12, 0x383C},  //70004398 
{0x0F12, 0xF000},  //7000439A 
{0x0F12, 0xF903},  //7000439C 
{0x0F12, 0xE488},  //7000439E 
{0x0F12, 0x2558},  //700043A0 
{0x0F12, 0x7000},  //700043A2 
{0x0F12, 0x2AB8},  //700043A4 
{0x0F12, 0x7000},  //700043A6 
{0x0F12, 0x145E},  //700043A8 
{0x0F12, 0x7000},  //700043AA 
{0x0F12, 0x2698},  //700043AC 
{0x0F12, 0x7000},  //700043AE 
{0x0F12, 0x2BB8},  //700043B0 
{0x0F12, 0x7000},  //700043B2 
{0x0F12, 0x2998},  //700043B4 
{0x0F12, 0x7000},  //700043B6 
{0x0F12, 0x1100},  //700043B8 
{0x0F12, 0xD000},  //700043BA 
{0x0F12, 0x4780},  //700043BC 
{0x0F12, 0x7000},  //700043BE 
{0x0F12, 0xE200},  //700043C0 
{0x0F12, 0xD000},  //700043C2 
{0x0F12, 0x210C},  //700043C4 
{0x0F12, 0x7000},  //700043C6 
{0x0F12, 0x0000},  //700043C8 
{0x0F12, 0x7000},  //700043CA 
{0x0F12, 0x4778},  //700043CC 
{0x0F12, 0x46C0},  //700043CE 
{0x0F12, 0xC000},  //700043D0 
{0x0F12, 0xE59F},  //700043D2 
{0x0F12, 0xFF1C},  //700043D4 
{0x0F12, 0xE12F},  //700043D6 
{0x0F12, 0x1789},  //700043D8 
{0x0F12, 0x0001},  //700043DA 
{0x0F12, 0x4778},  //700043DC 
{0x0F12, 0x46C0},  //700043DE 
{0x0F12, 0xC000},  //700043E0 
{0x0F12, 0xE59F},  //700043E2 
{0x0F12, 0xFF1C},  //700043E4 
{0x0F12, 0xE12F},  //700043E6 
{0x0F12, 0x16F1},  //700043E8 
{0x0F12, 0x0001},  //700043EA 
{0x0F12, 0x4778},  //700043EC 
{0x0F12, 0x46C0},  //700043EE 
{0x0F12, 0xC000},  //700043F0 
{0x0F12, 0xE59F},  //700043F2 
{0x0F12, 0xFF1C},  //700043F4 
{0x0F12, 0xE12F},  //700043F6 
{0x0F12, 0xC3B1},  //700043F8 
{0x0F12, 0x0000},  //700043FA 
{0x0F12, 0x4778},  //700043FC 
{0x0F12, 0x46C0},  //700043FE 
{0x0F12, 0xC000},  //70004400 
{0x0F12, 0xE59F},  //70004402 
{0x0F12, 0xFF1C},  //70004404 
{0x0F12, 0xE12F},  //70004406 
{0x0F12, 0xC36D},  //70004408 
{0x0F12, 0x0000},  //7000440A 
{0x0F12, 0x4778},  //7000440C 
{0x0F12, 0x46C0},  //7000440E 
{0x0F12, 0xC000},  //70004410 
{0x0F12, 0xE59F},  //70004412 
{0x0F12, 0xFF1C},  //70004414 
{0x0F12, 0xE12F},  //70004416 
{0x0F12, 0xF6D7},  //70004418 
{0x0F12, 0x0000},  //7000441A 
{0x0F12, 0x4778},  //7000441C 
{0x0F12, 0x46C0},  //7000441E 
{0x0F12, 0xC000},  //70004420 
{0x0F12, 0xE59F},  //70004422 
{0x0F12, 0xFF1C},  //70004424 
{0x0F12, 0xE12F},  //70004426 
{0x0F12, 0xB49D},  //70004428 
{0x0F12, 0x0000},  //7000442A 
{0x0F12, 0x4778},  //7000442C 
{0x0F12, 0x46C0},  //7000442E 
{0x0F12, 0xC000},  //70004430 
{0x0F12, 0xE59F},  //70004432 
{0x0F12, 0xFF1C},  //70004434 
{0x0F12, 0xE12F},  //70004436 
{0x0F12, 0x7EDF},  //70004438 
{0x0F12, 0x0000},  //7000443A 
{0x0F12, 0x4778},  //7000443C 
{0x0F12, 0x46C0},  //7000443E 
{0x0F12, 0xC000},  //70004440 
{0x0F12, 0xE59F},  //70004442 
{0x0F12, 0xFF1C},  //70004444 
{0x0F12, 0xE12F},  //70004446 
{0x0F12, 0x448D},  //70004448 
{0x0F12, 0x0000},  //7000444A 
{0x0F12, 0x4778},  //7000444C 
{0x0F12, 0x46C0},  //7000444E 
{0x0F12, 0xF004},  //70004450 
{0x0F12, 0xE51F},  //70004452 
{0x0F12, 0x29EC},  //70004454 
{0x0F12, 0x0001},  //70004456 
{0x0F12, 0x4778},  //70004458 
{0x0F12, 0x46C0},  //7000445A 
{0x0F12, 0xC000},  //7000445C 
{0x0F12, 0xE59F},  //7000445E 
{0x0F12, 0xFF1C},  //70004460 
{0x0F12, 0xE12F},  //70004462 
{0x0F12, 0x2EF1},  //70004464 
{0x0F12, 0x0000},  //70004466 
{0x0F12, 0x4778},  //70004468 
{0x0F12, 0x46C0},  //7000446A 
{0x0F12, 0xC000},  //7000446C 
{0x0F12, 0xE59F},  //7000446E 
{0x0F12, 0xFF1C},  //70004470 
{0x0F12, 0xE12F},  //70004472 
{0x0F12, 0xEE03},  //70004474 
{0x0F12, 0x0000},  //70004476 
{0x0F12, 0x4778},  //70004478 
{0x0F12, 0x46C0},  //7000447A 
{0x0F12, 0xC000},  //7000447C 
{0x0F12, 0xE59F},  //7000447E 
{0x0F12, 0xFF1C},  //70004480 
{0x0F12, 0xE12F},  //70004482 
{0x0F12, 0xA58B},  //70004484 
{0x0F12, 0x0000},  //70004486 
{0x0F12, 0x4778},  //70004488 
{0x0F12, 0x46C0},  //7000448A 
{0x0F12, 0xC000},  //7000448C 
{0x0F12, 0xE59F},  //7000448E 
{0x0F12, 0xFF1C},  //70004490 
{0x0F12, 0xE12F},  //70004492 
{0x0F12, 0x7C49},  //70004494 
{0x0F12, 0x0000},  //70004496 
{0x0F12, 0x4778},  //70004498 
{0x0F12, 0x46C0},  //7000449A 
{0x0F12, 0xC000},  //7000449C 
{0x0F12, 0xE59F},  //7000449E 
{0x0F12, 0xFF1C},  //700044A0 
{0x0F12, 0xE12F},  //700044A2 
{0x0F12, 0x7C63},  //700044A4 
{0x0F12, 0x0000},  //700044A6 
{0x0F12, 0x4778},  //700044A8 
{0x0F12, 0x46C0},  //700044AA 
{0x0F12, 0xC000},  //700044AC 
{0x0F12, 0xE59F},  //700044AE 
{0x0F12, 0xFF1C},  //700044B0 
{0x0F12, 0xE12F},  //700044B2 
{0x0F12, 0x2DB7},  //700044B4 
{0x0F12, 0x0000},  //700044B6 
{0x0F12, 0x4778},  //700044B8 
{0x0F12, 0x46C0},  //700044BA 
{0x0F12, 0xC000},  //700044BC 
{0x0F12, 0xE59F},  //700044BE 
{0x0F12, 0xFF1C},  //700044C0 
{0x0F12, 0xE12F},  //700044C2 
{0x0F12, 0xEB3D},  //700044C4 
{0x0F12, 0x0000},  //700044C6 
{0x0F12, 0x4778},  //700044C8 
{0x0F12, 0x46C0},  //700044CA 
{0x0F12, 0xC000},  //700044CC 
{0x0F12, 0xE59F},  //700044CE 
{0x0F12, 0xFF1C},  //700044D0 
{0x0F12, 0xE12F},  //700044D2 
{0x0F12, 0xF061},  //700044D4 
{0x0F12, 0x0000},  //700044D6 
{0x0F12, 0x4778},  //700044D8 
{0x0F12, 0x46C0},  //700044DA 
{0x0F12, 0xC000},  //700044DC 
{0x0F12, 0xE59F},  //700044DE 
{0x0F12, 0xFF1C},  //700044E0 
{0x0F12, 0xE12F},  //700044E2 
{0x0F12, 0xF0EF},  //700044E4 
{0x0F12, 0x0000},  //700044E6 
{0x0F12, 0x4778},  //700044E8 
{0x0F12, 0x46C0},  //700044EA 
{0x0F12, 0xF004},  //700044EC 
{0x0F12, 0xE51F},  //700044EE 
{0x0F12, 0x2824},  //700044F0 
{0x0F12, 0x0001},  //700044F2 
{0x0F12, 0x4778},  //700044F4 
{0x0F12, 0x46C0},  //700044F6 
{0x0F12, 0xC000},  //700044F8 
{0x0F12, 0xE59F},  //700044FA 
{0x0F12, 0xFF1C},  //700044FC 
{0x0F12, 0xE12F},  //700044FE 
{0x0F12, 0x8EDD},  //70004500 
{0x0F12, 0x0000},  //70004502 
{0x0F12, 0x4778},  //70004504 
{0x0F12, 0x46C0},  //70004506 
{0x0F12, 0xC000},  //70004508 
{0x0F12, 0xE59F},  //7000450A 
{0x0F12, 0xFF1C},  //7000450C 
{0x0F12, 0xE12F},  //7000450E 
{0x0F12, 0x8DCB},  //70004510 
{0x0F12, 0x0000},  //70004512 
{0x0F12, 0x4778},  //70004514 
{0x0F12, 0x46C0},  //70004516 
{0x0F12, 0xC000},  //70004518 
{0x0F12, 0xE59F},  //7000451A 
{0x0F12, 0xFF1C},  //7000451C 
{0x0F12, 0xE12F},  //7000451E 
{0x0F12, 0x8E17},  //70004520 
{0x0F12, 0x0000},  //70004522 
{0x0F12, 0x4778},  //70004524 
{0x0F12, 0x46C0},  //70004526 
{0x0F12, 0xC000},  //70004528 
{0x0F12, 0xE59F},  //7000452A 
{0x0F12, 0xFF1C},  //7000452C 
{0x0F12, 0xE12F},  //7000452E 
{0x0F12, 0x98C5},  //70004530 
{0x0F12, 0x0000},  //70004532 
{0x0F12, 0x4778},  //70004534 
{0x0F12, 0x46C0},  //70004536 
{0x0F12, 0xC000},  //70004538 
{0x0F12, 0xE59F},  //7000453A 
{0x0F12, 0xFF1C},  //7000453C 
{0x0F12, 0xE12F},  //7000453E 
{0x0F12, 0x7C7D},  //70004540 
{0x0F12, 0x0000},  //70004542 
{0x0F12, 0x4778},  //70004544 
{0x0F12, 0x46C0},  //70004546 
{0x0F12, 0xC000},  //70004548 
{0x0F12, 0xE59F},  //7000454A 
{0x0F12, 0xFF1C},  //7000454C 
{0x0F12, 0xE12F},  //7000454E 
{0x0F12, 0x7E31},  //70004550 
{0x0F12, 0x0000},  //70004552 
{0x0F12, 0x4778},  //70004554 
{0x0F12, 0x46C0},  //70004556 
{0x0F12, 0xC000},  //70004558 
{0x0F12, 0xE59F},  //7000455A 
{0x0F12, 0xFF1C},  //7000455C 
{0x0F12, 0xE12F},  //7000455E 
{0x0F12, 0x7EAB},  //70004560 
{0x0F12, 0x0000},  //70004562 
{0x0F12, 0x4778},  //70004564 
{0x0F12, 0x46C0},  //70004566 
{0x0F12, 0xC000},  //70004568 
{0x0F12, 0xE59F},  //7000456A 
{0x0F12, 0xFF1C},  //7000456C 
{0x0F12, 0xE12F},  //7000456E 
{0x0F12, 0x7501},  //70004570 
{0x0F12, 0x0000},  //70004572 
{0x0F12, 0x4778},  //70004574 
{0x0F12, 0x46C0},  //70004576 
{0x0F12, 0xC000},  //70004578 
{0x0F12, 0xE59F},  //7000457A 
{0x0F12, 0xFF1C},  //7000457C 
{0x0F12, 0xE12F},  //7000457E 
{0x0F12, 0xF63F},  //70004580 
{0x0F12, 0x0000},  //70004582 
{0x0F12, 0x4778},  //70004584 
{0x0F12, 0x46C0},  //70004586 
{0x0F12, 0xC000},  //70004588 
{0x0F12, 0xE59F},  //7000458A 
{0x0F12, 0xFF1C},  //7000458C 
{0x0F12, 0xE12F},  //7000458E 
{0x0F12, 0x3D0B},  //70004590 
{0x0F12, 0x0000},  //70004592 
{0x0F12, 0x4778},  //70004594 
{0x0F12, 0x46C0},  //70004596 
{0x0F12, 0xC000},  //70004598 
{0x0F12, 0xE59F},  //7000459A 
{0x0F12, 0xFF1C},  //7000459C 
{0x0F12, 0xE12F},  //7000459E 
{0x0F12, 0x29BF},  //700045A0 
{0x0F12, 0x0001},  //700045A2 
{0x0F12, 0x4778},  //700045A4 
{0x0F12, 0x46C0},  //700045A6 
{0x0F12, 0xF004},  //700045A8 
{0x0F12, 0xE51F},  //700045AA 
{0x0F12, 0x26D8},  //700045AC 
{0x0F12, 0x0001},  //700045AE 
{0x0028, 0x7000},
{0x002A, 0x47B0},
{0x0F12, 0x0000},  //TNP_Regs_BUse1FrameAE   (0: off, 1: on)                                     
{0x0028, 0xD000},
{0x002A, 0x1000},
{0x0F12, 0x0001},

//================================================================================== 
// 06.Gas_Anti Shading                                                               	
//================================================================================== 	
{0x0028, 0x7000},
{0x002A, 0x08B4},	
{0x0F12, 0x0001},	//wbt_bUseOutdoorASH	
{0x002A, 0x08BC},	
{0x0F12, 0x00C0},	//TVAR_ash_AwbAshCord_0_ 2300K	
{0x0F12, 0x00DF},	//TVAR_ash_AwbAshCord_1_ 2750K	
{0x0F12, 0x0100},	//TVAR_ash_AwbAshCord_2_ 3300K	
{0x0F12, 0x0125},	//TVAR_ash_AwbAshCord_3_ 4150K	
{0x0F12, 0x015F},	//TVAR_ash_AwbAshCord_4_ 5250K	
{0x0F12, 0x017C},	//TVAR_ash_AwbAshCord_5_ 6400K	
{0x0F12, 0x0194},	//TVAR_ash_AwbAshCord_6_ 7500K	
	
{0x002A, 0x08F6},	
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_0__0_ R  // 2300K     	
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_0__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_0__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_0__3_ B    

{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_1__0_ R  // 2750K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_1__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_1__2_ GB  
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_1__3_ B   

{0x0F12, 0x3800},	//TVAR_ash_GASAlpha_2__0_ R  // 3300K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_2__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_2__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_2__3_ B   

{0x0F12, 0x3800}, 	//TVAR_ash_GASAlpha_3__0_ R  // 4150K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_3__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_3__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_3__3_ B    

{0x0F12, 0x3800}, 	//TVAR_ash_GASAlpha_4__0_ R  // 5250K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_4__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_4__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_4__3_ B      

{0x0F12, 0x4000}, 	//TVAR_ash_GASAlpha_5__0_ R  // 6400K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_5__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_5__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_5__3_ B        

{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_6__0_ R  // 7500K 
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_6__1_ GR          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_6__2_ GB          
{0x0F12, 0x4000},	//TVAR_ash_GASAlpha_6__3_ B        

//Outdoor GAS Alpha   
{0x0F12, 0x4000},  	
{0x0F12, 0x4000},	
{0x0F12, 0x4000},	
{0x0F12, 0x4000},	

{0x002A, 0x08F4},	
{0x0F12, 0x0001},	//ash_bUseGasAlpha
{0x002A, 0x0D26}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F01}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0x0000}, 
{0x0F12, 0x0F0F}, 
{0x0F12, 0x0F00}, 
{0x0F12, 0x000F}, 
{0x0F12, 0xF25B}, 
{0x0F12, 0x987E}, 
{0x0F12, 0x6351}, 
{0x0F12, 0xB3D0}, 
{0x0F12, 0x2B7A}, 
{0x0F12, 0x0787}, 
{0x0F12, 0x596E}, 
{0x0F12, 0x64D4}, 
{0x0F12, 0x96DF}, 
{0x0F12, 0x4AEE}, 
{0x0F12, 0x00C0}, 
{0x0F12, 0xBAC2}, 
{0x0F12, 0xB45A}, 
{0x0F12, 0x771D}, 
{0x0F12, 0x737E}, 
{0x0F12, 0xC4B4}, 
{0x0F12, 0xD73E}, 
{0x0F12, 0x8263}, 
{0x0F12, 0x5812}, 
{0x0F12, 0x7A9D}, 
{0x0F12, 0xB9B1}, 
{0x0F12, 0x2356}, 
{0x0F12, 0x1171}, 
{0x0F12, 0x865C}, 
{0x0F12, 0x8009}, 
{0x0F12, 0xD8FF}, 
{0x0F12, 0xE500}, 
{0x0F12, 0xFAF8}, 
{0x0F12, 0x4448}, 
{0x0F12, 0x0FAD}, 
{0x0F12, 0xD754}, 
{0x0F12, 0xE182}, 
{0x0F12, 0x4793}, 
{0x0F12, 0xF2F7}, 
{0x0F12, 0xA348}, 
{0x0F12, 0x3ED0}, 
{0x0F12, 0x1BD5}, 
{0x0F12, 0x8516}, 
{0x0F12, 0x713A}, 
{0x0F12, 0xAB5A}, 
{0x0F12, 0x3655}, 
{0x0F12, 0x0061}, 
{0x0F12, 0x4704}, 
{0x0F12, 0x5D18}, 
{0x0F12, 0xA22F}, 
{0x0F12, 0x4218}, 
{0x0F12, 0x005C}, 
{0x0F12, 0xBFE1}, 
{0x0F12, 0xAFB7}, 
{0x0F12, 0x8E84}, 
{0x0F12, 0x5E1B}, 
{0x0F12, 0xCB6A}, 
{0x0F12, 0xE333}, 
{0x0F12, 0x73A1}, 
{0x0F12, 0x771A}, 
{0x0F12, 0x5905}, 
{0x0F12, 0xD3A8}, 
{0x0F12, 0x19B1}, 
{0x0F12, 0x0966}, 
{0x0F12, 0x9276}, 
{0x0F12, 0x5CA5}, 
{0x0F12, 0xF073}, 
{0x0F12, 0xD58F}, 
{0x0F12, 0x09E2}, 
{0x0F12, 0x3566}, 
{0x0F12, 0x1760}, 
{0x0F12, 0xEAD9}, 
{0x0F12, 0xDE66}, 
{0x0F12, 0x4551}, 
{0x0F12, 0xEC7B}, 
{0x0F12, 0xB597}, 
{0x0F12, 0x2FBA}, 
{0x0F12, 0xEBBF}, 
{0x0F12, 0x903C}, 
{0x0F12, 0x6FBE}, 
{0x0F12, 0xAB3D}, 
{0x0F12, 0x26FA}, 
{0x0F12, 0x11E8}, 
{0x0F12, 0x553F}, 
{0x0F12, 0x7740}, 
{0x0F12, 0x8141}, 
{0x0F12, 0x5588}, 
{0x0F12, 0x0B75}, 
{0x0F12, 0xA9D7}, 
{0x0F12, 0xCAC2}, 
{0x0F12, 0x55D3}, 
{0x0F12, 0x9750}, 
{0x0F12, 0xB17B}, 
{0x0F12, 0xCD52}, 
{0x0F12, 0x9520}, 
{0x0F12, 0x3891}, 
{0x0F12, 0xA01F}, 
{0x0F12, 0x9FFE}, 
{0x0F12, 0x2232}, 
{0x0F12, 0x1F8C}, 
{0x0F12, 0x8004}, 
{0x0F12, 0x997E}, 
{0x0F12, 0xC1AD}, 
{0x0F12, 0xD0C2}, 
{0x0F12, 0x39AD}, 
{0x0F12, 0x181C}, 
{0x0F12, 0x0F3A}, 
{0x0F12, 0xCD3A}, 
{0x0F12, 0xE37B}, 
{0x0F12, 0x72C5}, 
{0x0F12, 0xA3DF}, 
{0x0F12, 0xD97C}, 
{0x0F12, 0x39D7}, 
{0x0F12, 0xF2EC}, 
{0x0F12, 0x950D}, 
{0x0F12, 0x6522}, 
{0x0F12, 0xB1B2}, 
{0x0F12, 0x3209}, 
{0x0F12, 0xFF87}, 
{0x0F12, 0x60CD}, 
{0x0F12, 0x5CF0}, 
{0x0F12, 0xA430}, 
{0x0F12, 0x426A}, 
{0x0F12, 0xF218}, 
{0x0F12, 0xD51B}, 
{0x0F12, 0xA693}, 
{0x0F12, 0x8AF5}, 
{0x0F12, 0x5231}, 
{0x0F12, 0xE1A0}, 
{0x0F12, 0xE594}, 
{0x0F12, 0x55F2}, 
{0x0F12, 0x7828}, 
{0x0F12, 0x5B8C}, 
{0x0F12, 0xE08A}, 
{0x0F12, 0xFA0D}, 
{0x0F12, 0x10F1}, 
{0x0F12, 0xB180}, 
{0x0F12, 0x58B6}, 
{0x0F12, 0xF123}, 
{0x0F12, 0xD1E0}, 
{0x0F12, 0x243C}, 
{0x0F12, 0x1812}, 
{0x0F12, 0x15EB}, 
{0x0F12, 0xED2A}, 
{0x0F12, 0xDB4F}, 
{0x0F12, 0x4546}, 
{0x0F12, 0xD9A5}, 
{0x0F12, 0xDF50}, 
{0x0F12, 0x147A},                   
	
//==================================================================================
// 08.AF Setting
//==================================================================================
    
//AF interface setting
{0x0028, 0x7000},                                                                  
{0x002A, 0x01FC},                                                                  
{0x0F12, 0x0000},  //#REG_TC_IPRM_LedGpio
{0x002A, 0x1720},  
{0x0F12, 0x0000},  //#afd_usFlags
{0x002A, 0x01FE},                                                                  
{0x0F12, 0x0003},	//REG_TC_IPRM_CM_Init_AfModeType VCM IIC                   
{0x0F12, 0x0000},	//REG_TC_IPRM_CM_Init_PwmConfig1                           
{0x002A, 0x0204},                                                                  
{0x0F12, 0x0061},	//REG_TC_IPRM_CM_Init_GpioConfig1 AF Enable GPIO 6	
{0x002A, 0x020C},                                                                  
{0x0F12, 0x2F0C},	//REG_TC_IPRM_CM_Init_Mi2cBits                             
{0x0F12, 0x0190},	//REG_TC_IPRM_CM_Init_Mi2cRateKhz IIC Speed     

{0x002A, 0x0294},                                                                  
{0x0F12, 0x0100},	//REG_TC_AF_FstWinStartX                                   
{0x0F12, 0x00E3},	//REG_TC_AF_FstWinStartY                                   
{0x0F12, 0x0200},	//REG_TC_AF_FstWinSizeX                                    
{0x0F12, 0x0238},	//REG_TC_AF_FstWinSizeY                                    
{0x0F12, 0x01C6},	//REG_TC_AF_ScndWinStartX                                  
{0x0F12, 0x0166},	//REG_TC_AF_ScndWinStartY                                  
{0x0F12, 0x0074},	//REG_TC_AF_ScndWinSizeX                                   
{0x0F12, 0x0132},	//REG_TC_AF_ScndWinSizeY                                   
{0x0F12, 0x0001},	//REG_TC_AF_WinSizesUpdated                                
                                                                           
	
//2nd search setting	
{0x002A, 0x070E},                                                                  
{0x0F12, 0x00FF}, //C0 	//skl_af_StatOvlpExpFactor                                 
{0x002A, 0x071E},                                                                  
{0x0F12, 0x0000}, //01 00 	//skl_af_bAfStatOff                                        
{0x002A, 0x163C},                                                                  
{0x0F12, 0x0000},	//af_search_usAeStable                                     
{0x002A, 0x1648},                                                                  
{0x0F12, 0x9000},  //#af_search_usSingleAfFlags
{0x002A, 0x1652},                                                                  
{0x0F12, 0x0003},  //#af_search_usFinePeakCount
{0x0F12, 0x0000},	//af_search_usFineMaxScale                                 
{0x002A, 0x15E0},                                                                  
{0x0F12, 0x0902},	//af_pos_usFineStepNumSize                                 
                                                                           
//Peak Threshold	
{0x002A, 0x164C},                                                                  
{0x0F12, 0x0003},	//af_search_usMinPeakSamples                               
{0x002A, 0x163E},                                                                  
{0x0F12, 0x00E5}, //80 //8a //A0 	//A0 af_search_usPeakThr Full search (E5 90%)                 
{0x0F12, 0x0098}, //80 //a0 //8a  //98	//98 af_search_usPeakThrLow                                   
{0x002A, 0x47A8},	                               
{0x0F12, 0x0080},	//TNP, Macro Threshold register
	
	
//Home Pos	
{0x002A, 0x15D4},                                                                  
{0x0F12, 0x0020},  //#af_pos_usHomePos
{0x0F12, 0xD020},  //#af_pos_usLowConfPos
	
	
//AF statistics	
{0x002A, 0x169A},                                                                  
{0x0F12, 0xFF95},	//af_search_usConfCheckOrder_1_                            
{0x002A, 0x166A},                                                                  
{0x0F12, 0x0280},	//af_search_usConfThr_4_                                   
{0x002A, 0x1676},	                                                           
{0x0F12, 0x03FF},  //#af_search_usConfThr_10_
{0x0F12, 0x0320},	//af_search_usConfThr_11_                                  
{0x002A, 0x16BC},	                                                           
{0x0F12, 0x0030},	//af_stat_usMinStatVal                                     
{0x002A, 0x16E0},	                                                           
{0x0F12, 0x0060},	//af_scene_usSceneLowNormBrThr                             
{0x002A, 0x16D4},	                                                           
{0x0F12, 0x0010},	//af_stat_usBpfThresh                                      
{0x002A, 0x1656},	                                                           
{0x0F12, 0x0000},	//af_search_usCapturePolicy                                
{0x002A, 0x15E6},                                                                  
{0x0F12, 0x003C},	//af_pos_usCaptureFixedPos                                 
                                                                     
{0x0F12, 0x0011},	//af_pos_usTableLastInd                                    
{0x0F12, 0x0020},	//af_pos_usTable_0_                                        
{0x0F12, 0x0028},	//af_pos_usTable_1_                                        
{0x0F12, 0x0030},	//af_pos_usTable_2_                                        
{0x0F12, 0x0038},	//af_pos_usTable_3_                                        
{0x0F12, 0x0040},	//af_pos_usTable_4_  
{0x0F12, 0x0048},	//af_pos_usTable_5_                                        
{0x0F12, 0x0050},	//af_pos_usTable_6_                                        
{0x0F12, 0x0058},	//af_pos_usTable_7_                                        
{0x0F12, 0x0060},	//af_pos_usTable_8_                                        
{0x0F12, 0x0068},	//af_pos_usTable_9_                                        
{0x0F12, 0x0070},	//af_pos_usTable_10 
{0x0F12, 0x0078},	//af_pos_usTable_11_                                       
{0x0F12, 0x0080},	//af_pos_usTable_12_                                       
{0x0F12, 0x0088},	//af_pos_usTable_13_                                       
{0x0F12, 0x0090},	//af_pos_usTable_14_                                       
{0x0F12, 0x00A0},	//af_pos_usTable_15_ 
{0x0F12, 0x00B0},	//af_pos_usTable_16_                                       
{0x0F12, 0x00C0},	//af_pos_usTable_17_                                       
//{0x0F12, 0x00E0},	//af_pos_usTable_18_                                       
//{0x0F12, 0x009C},	//af_pos_usTable_19_                                       
//{0x0F12, 0x00A2},	//af_pos_usTable_20_                                       
//{0x0F12, 0x00A8},	//af_pos_usTable_21_                                       
//{0x0F12, 0x00AE},	//af_pos_usTable_22_                                       
//{0x0F12, 0x00B4},	//af_pos_usTable_23_                                       
//{0x0F12, 0x00BA},	//af_pos_usTable_24_  
  	
//VCM AF driver with PWM/I2C                                                                         
{0x002A, 0x1722},                                                                  
{0x0F12, 0x8000},	//afd_usParam_0_                                           
{0x0F12, 0x0006},	//afd_usParam_1_                                           
{0x0F12, 0x3FF0},	//afd_usParam_2_                                           
{0x0F12, 0x03E8},	//afd_usParam_3_                                           
{0x0F12, 0x0000},	//afd_usParam_4_                                           
{0x0F12, 0x0080},   //10 afd_usParam_5_                                           
{0x0F12, 0x0009},  //#afd_usParam_6_
{0x0F12, 0x0020},  //#afd_usParam_7_
{0x0F12, 0x0040},	//afd_usParam_8_                                           
{0x0F12, 0x0080},	//afd_usParam_9_                                           
{0x0F12, 0x00C0},	//afd_usParam_10_                                          
{0x0F12, 0x00E0},	//afd_usParam_11_                                          
                                                                     
{0x002A, 0x1748},
{0x0F12, 0x0001},
{0x002A, 0x070A},
{0x0F12, 0x0000},  //#skl_af_bValidateCapture
{0x002A, 0x0710},
{0x0F12, 0x00FF},  //#skl_af_CaptureOvlpExpFactor
{0x002A, 0x16E0},  //FOR AF LOWLIGHT FOCUS
{0x0F12, 0x0010},
{0x002A, 0x1664},
{0x0F12, 0x03FF},
{0x002A, 0x028C},                                                                  
{0x0F12, 0x0003},	//REG_TC_AF_AfCmd
	
//==================================================================================
// 09.AWB-BASIC setting
//==================================================================================

// AWB init Start point
  {0x002A, 0x145E},
  {0x0F12, 0x0580}, //awbb_GainsInit_0_
  {0x0F12, 0x0428}, //awbb_GainsInit_1_
  {0x0F12, 0x07B0}, //awbb_GainsInit_2_
// AWB Convergence Speed
  {0x0F12, 0x0008}, //awbb_WpFilterMinThr
  {0x0F12, 0x0190}, //awbb_WpFilterMaxThr
  {0x0F12, 0x00A0}, //awbb_WpFilterCoef
  {0x0F12, 0x0004}, //awbb_WpFilterSize
  {0x0F12, 0x0002}, //awbb_GridEnable
  {0x002A, 0x144E},
  {0x0F12, 0x0000}, //awbb_RGainOff
  {0x0F12, 0x0000}, //awbb_BGainOff
  {0x0F12, 0x0000}, //awbb_GGainOff
  {0x0F12, 0x00C2}, //awbb_Alpha_Comp_Mode
  {0x0F12, 0x0002}, //awbb_Rpl_InvalidOutDoor
  {0x0F12, 0x0001}, //awbb_UseGrThrCorr
  {0x0F12, 0x0074}, //awbb_Use_Filters
  {0x0F12, 0x0001}, //awbb_CorrectMinNumPatches
// White Locus
  {0x002A, 0x11F0},
  {0x0F12, 0x012C}, //awbb_IntcR
  {0x0F12, 0x0121}, //awbb_IntcB
  {0x0F12, 0x02DF}, //awbb_GLocusR
  {0x0F12, 0x0314}, //awbb_GLocusB
  {0x002A, 0x120E},
  {0x0F12, 0x0000}, //awbb_MovingScale10
  {0x0F12, 0x05FD}, //awbb_GamutWidthThr1
  {0x0F12, 0x036B}, //awbb_GamutHeightThr1
  {0x0F12, 0x0020}, //awbb_GamutWidthThr2
  {0x0F12, 0x001A}, //awbb_GamutHeightThr2
  {0x002A, 0x1278},
  {0x0F12, 0xFEF7}, //awbb_SCDetectionMap_SEC_StartR_B
  {0x0F12, 0x0021}, //awbb_SCDetectionMap_SEC_StepR_B
  {0x0F12, 0x07D0}, //awbb_SCDetectionMap_SEC_SunnyNB
  {0x0F12, 0x07D0}, //awbb_SCDetectionMap_SEC_StepNB
  {0x0F12, 0x01C8}, //awbb_SCDetectionMap_SEC_LowTempR_B
  {0x0F12, 0x0096}, //awbb_SCDetectionMap_SEC_SunnyNBZone
  {0x0F12, 0x0004}, //awbb_SCDetectionMap_SEC_LowTempR_BZone
  {0x002A, 0x1224},
  {0x0F12, 0x0032}, //awbb_LowBr
  {0x0F12, 0x001E}, //awbb_LowBr_NBzone
  {0x0F12, 0x00E2}, //awbb_YThreshHigh
  {0x0F12, 0x0010}, //awbb_YThreshLow_Norm
  {0x0F12, 0x0002}, //awbb_YThreshLow_Low
  {0x002A, 0x2BA4},
  {0x0F12, 0x0004}, //Mon_AWB_ByPassMode
  {0x002A, 0x11FC},
  {0x0F12, 0x000C}, //awbb_MinNumOfFinalPatches
  {0x002A, 0x1208},
  {0x0F12, 0x0020}, //awbb_MinNumOfChromaclassifpatches
// Indoor Zone
  {0x002A, 0x101C},
  {0x0F12, 0x0360}, //awbb_IndoorGrZones_m_BGrid_0__m_left
  {0x0F12, 0x036C}, //awbb_IndoorGrZones_m_BGrid_0__m_right
  {0x0F12, 0x0320}, //awbb_IndoorGrZones_m_BGrid_1__m_left
  {0x0F12, 0x038A}, //awbb_IndoorGrZones_m_BGrid_1__m_right
  {0x0F12, 0x02E8}, //awbb_IndoorGrZones_m_BGrid_2__m_left
  {0x0F12, 0x0380}, //awbb_IndoorGrZones_m_BGrid_2__m_right
  {0x0F12, 0x02BE}, //awbb_IndoorGrZones_m_BGrid_3__m_left
  {0x0F12, 0x035A}, //awbb_IndoorGrZones_m_BGrid_3__m_right
  {0x0F12, 0x0298}, //awbb_IndoorGrZones_m_BGrid_4__m_left
  {0x0F12, 0x0334}, //awbb_IndoorGrZones_m_BGrid_4__m_right
  {0x0F12, 0x0272}, //awbb_IndoorGrZones_m_BGrid_5__m_left
  {0x0F12, 0x030E}, //awbb_IndoorGrZones_m_BGrid_5__m_right
  {0x0F12, 0x024C}, //awbb_IndoorGrZones_m_BGrid_6__m_left
  {0x0F12, 0x02EA}, //awbb_IndoorGrZones_m_BGrid_6__m_right
  {0x0F12, 0x0230}, //awbb_IndoorGrZones_m_BGrid_7__m_left
  {0x0F12, 0x02CC}, //awbb_IndoorGrZones_m_BGrid_7__m_right
  {0x0F12, 0x0214}, //awbb_IndoorGrZones_m_BGrid_8__m_left
  {0x0F12, 0x02B0}, //awbb_IndoorGrZones_m_BGrid_8__m_right
  {0x0F12, 0x01F8}, //awbb_IndoorGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0294}, //awbb_IndoorGrZones_m_BGrid_9__m_right
  {0x0F12, 0x01DC}, //awbb_IndoorGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0278}, //awbb_IndoorGrZones_m_BGrid_10__m_right
  {0x0F12, 0x01C0}, //awbb_IndoorGrZones_m_BGrid_11__m_left
  {0x0F12, 0x0264}, //awbb_IndoorGrZones_m_BGrid_11__m_right
  {0x0F12, 0x01AA}, //awbb_IndoorGrZones_m_BGrid_12__m_left
  {0x0F12, 0x0250}, //awbb_IndoorGrZones_m_BGrid_12__m_right
  {0x0F12, 0x0196}, //awbb_IndoorGrZones_m_BGrid_13__m_left
  {0x0F12, 0x023C}, //awbb_IndoorGrZones_m_BGrid_13__m_right
  {0x0F12, 0x0180}, //awbb_IndoorGrZones_m_BGrid_14__m_left
  {0x0F12, 0x0228}, //awbb_IndoorGrZones_m_BGrid_14__m_right
  {0x0F12, 0x016C}, //awbb_IndoorGrZones_m_BGrid_15__m_left
  {0x0F12, 0x0214}, //awbb_IndoorGrZones_m_BGrid_15__m_right
  {0x0F12, 0x0168}, //awbb_IndoorGrZones_m_BGrid_16__m_left
  {0x0F12, 0x0200}, //awbb_IndoorGrZones_m_BGrid_16__m_right
  {0x0F12, 0x0172}, //awbb_IndoorGrZones_m_BGrid_17__m_left
  {0x0F12, 0x01EC}, //awbb_IndoorGrZones_m_BGrid_17__m_right
  {0x0F12, 0x019A}, //awbb_IndoorGrZones_m_BGrid_18__m_left
  {0x0F12, 0x01D8}, //awbb_IndoorGrZones_m_BGrid_18__m_right
  {0x0F12, 0x0000}, //awbb_IndoorGrZones_m_BGrid_19__m_left
  {0x0F12, 0x0000}, //awbb_IndoorGrZones_m_BGrid_19__m_right
  {0x0F12, 0x0005}, //awbb_IndoorGrZones_m_GridStep
  {0x002A, 0x1070},
  {0x0F12, 0x0013}, //awbb_IndoorGrZones_ZInfo_m_GridSz
  {0x002A, 0x1074},
  {0x0F12, 0x00EC}, //awbb_IndoorGrZones_m_Boffs
// Outdoor Zone
  {0x002A, 0x1078},
  {0x0F12, 0x0232}, //awbb_OutdoorGrZones_m_BGrid_0__m_left
  {0x0F12, 0x025A}, //awbb_OutdoorGrZones_m_BGrid_0__m_right
  {0x0F12, 0x021E}, //awbb_OutdoorGrZones_m_BGrid_1__m_left
  {0x0F12, 0x0274}, //awbb_OutdoorGrZones_m_BGrid_1__m_right
  {0x0F12, 0x020E}, //awbb_OutdoorGrZones_m_BGrid_2__m_left
  {0x0F12, 0x028E}, //awbb_OutdoorGrZones_m_BGrid_2__m_right
  {0x0F12, 0x0200}, //awbb_OutdoorGrZones_m_BGrid_3__m_left
  {0x0F12, 0x0290}, //awbb_OutdoorGrZones_m_BGrid_3__m_right
  {0x0F12, 0x01F4}, //awbb_OutdoorGrZones_m_BGrid_4__m_left
  {0x0F12, 0x0286}, //awbb_OutdoorGrZones_m_BGrid_4__m_right
  {0x0F12, 0x01E8}, //awbb_OutdoorGrZones_m_BGrid_5__m_left
  {0x0F12, 0x027E}, //awbb_OutdoorGrZones_m_BGrid_5__m_right
  {0x0F12, 0x01DE}, //awbb_OutdoorGrZones_m_BGrid_6__m_left
  {0x0F12, 0x0274}, //awbb_OutdoorGrZones_m_BGrid_6__m_right
  {0x0F12, 0x01D2}, //awbb_OutdoorGrZones_m_BGrid_7__m_left
  {0x0F12, 0x0268}, //awbb_OutdoorGrZones_m_BGrid_7__m_right
  {0x0F12, 0x01D0}, //awbb_OutdoorGrZones_m_BGrid_8__m_left
  {0x0F12, 0x025E}, //awbb_OutdoorGrZones_m_BGrid_8__m_right
  {0x0F12, 0x01D6}, //awbb_OutdoorGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0252}, //awbb_OutdoorGrZones_m_BGrid_9__m_right
  {0x0F12, 0x01E2}, //awbb_OutdoorGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0248}, //awbb_OutdoorGrZones_m_BGrid_10__m_right
  {0x0F12, 0x01F4}, //awbb_OutdoorGrZones_m_BGrid_11__m_left
  {0x0F12, 0x021A}, //awbb_OutdoorGrZones_m_BGrid_11__m_right
  {0x0F12, 0x0004}, //awbb_OutdoorGrZones_m_GridStep
  {0x002A, 0x10AC},
  {0x0F12, 0x000C}, //awbb_OutdoorGrZones_ZInfo_m_GridSz
  {0x002A, 0x10B0},
  {0x0F12, 0x01DA}, //awbb_OutdoorGrZones_m_Boffs
// Low Brightness Zone
  {0x002A, 0x10B4},
  {0x0F12, 0x0348}, //awbb_LowBrGrZones_m_BGrid_0__m_left
  {0x0F12, 0x03B6}, //awbb_LowBrGrZones_m_BGrid_0__m_right
  {0x0F12, 0x02B8}, //awbb_LowBrGrZones_m_BGrid_1__m_left
  {0x0F12, 0x03B6}, //awbb_LowBrGrZones_m_BGrid_1__m_right
  {0x0F12, 0x0258}, //awbb_LowBrGrZones_m_BGrid_2__m_left
  {0x0F12, 0x038E}, //awbb_LowBrGrZones_m_BGrid_2__m_right
  {0x0F12, 0x0212}, //awbb_LowBrGrZones_m_BGrid_3__m_left
  {0x0F12, 0x0348}, //awbb_LowBrGrZones_m_BGrid_3__m_right
  {0x0F12, 0x01CC}, //awbb_LowBrGrZones_m_BGrid_4__m_left
  {0x0F12, 0x030C}, //awbb_LowBrGrZones_m_BGrid_4__m_right
  {0x0F12, 0x01A2}, //awbb_LowBrGrZones_m_BGrid_5__m_left
  {0x0F12, 0x02D2}, //awbb_LowBrGrZones_m_BGrid_5__m_right
  {0x0F12, 0x0170}, //awbb_LowBrGrZones_m_BGrid_6__m_left
  {0x0F12, 0x02A6}, //awbb_LowBrGrZones_m_BGrid_6__m_right
  {0x0F12, 0x014C}, //awbb_LowBrGrZones_m_BGrid_7__m_left
  {0x0F12, 0x0280}, //awbb_LowBrGrZones_m_BGrid_7__m_right
  {0x0F12, 0x0128}, //awbb_LowBrGrZones_m_BGrid_8__m_left
  {0x0F12, 0x025C}, //awbb_LowBrGrZones_m_BGrid_8__m_right
  {0x0F12, 0x0146}, //awbb_LowBrGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0236}, //awbb_LowBrGrZones_m_BGrid_9__m_right
  {0x0F12, 0x0164}, //awbb_LowBrGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0212}, //awbb_LowBrGrZones_m_BGrid_10__m_right
  {0x0F12, 0x0000}, //awbb_LowBrGrZones_m_BGrid_11__m_left
  {0x0F12, 0x0000}, //awbb_LowBrGrZones_m_BGrid_11__m_right
  {0x0F12, 0x0006}, //awbb_LowBrGrZones_m_GridStep
  {0x002A, 0x10E8},
  {0x0F12, 0x000B}, //awbb_LowBrGrZones_ZInfo_m_GridSz
  {0x002A, 0x10EC},
  {0x0F12, 0x00D2}, //awbb_LowBrGrZones_m_Boffs

// Low Temp. Zone
  {0x002A, 0x10F0},
  {0x0F12, 0x039A},
  {0x0F12, 0x0000}, //awbb_CrclLowT_R_c
  {0x0F12, 0x00FE},
  {0x0F12, 0x0000}, //awbb_CrclLowT_B_c
  {0x0F12, 0x2284},
  {0x0F12, 0x0000}, //awbb_CrclLowT_Rad_c

//AWB - GridCorrection
  {0x002A, 0x1434},
  {0x0F12, 0x02C1}, //awbb_GridConst_1_0_
  {0x0F12, 0x033A}, //awbb_GridConst_1_1_
  {0x0F12, 0x038A}, //awbb_GridConst_1_2_
  {0x0F12, 0x101A}, //awbb_GridConst_2_0_
  {0x0F12, 0x1075}, //awbb_GridConst_2_1_
  {0x0F12, 0x113D}, //awbb_GridConst_2_2_
  {0x0F12, 0x113F}, //awbb_GridConst_2_3_
  {0x0F12, 0x11AF}, //awbb_GridConst_2_4_
  {0x0F12, 0x11F0}, //awbb_GridConst_2_5_
  {0x0F12, 0x00B2}, //awbb_GridCoeff_R_1
  {0x0F12, 0x00B8}, //awbb_GridCoeff_B_1
  {0x0F12, 0x00CA}, //awbb_GridCoeff_R_2
  {0x0F12, 0x009D}, //awbb_GridCoeff_B_2
  
  {0x002A, 0x146C},  
  {0x0F12, 0x0002},  //#awbb_GridEnable

// Indoor Grid Offset
  {0x002A, 0x13A4},
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__1_
  {0x0F12, 0xFFB0}, //awbb_GridCorr_R_1__2_
  {0x0F12, 0xFFB0}, //awbb_GridCorr_R_1__3_
  {0x0F12, 0xFF9C}, //awbb_GridCorr_R_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__5_

// Outdoor Grid Offset
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__5_

	
	
    //==================================================================================
    // 10.Clock Setting
    //==================================================================================
    //For MCLK=24MHz, PCLK=5DC0
	{0x002a, 0x01f8},      //System Setting  
	{0x0f12, 0x5dc0},
	{0x002a, 0x0212},//
	{0x0f12, 0x0002},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},//0x0216  PVI
	{0x002a, 0x021a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x59D8},
	{0x0f12, 0x59D8},
	{0x0f12, 0x59D8},
	{0x002a, 0x022c},
	{0x0f12, 0x0001},
	
	{0x002a, 0x0f30},
	{0x0f12, 0x0001},
	{0x002a, 0x0f2a},
	{0x0f12, 0x0000},//AFC_Default60Hz 0001:60Hz 0000h:50Hz
	{0x002a, 0x04e6},//REG_TC_DBG_AutoAlgEnBits
	{0x0f12, 0x077f},//0x075f
	{0x002a, 0x04d6},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},

// AE Setting
	{0x002a, 0x1484},
	{0x0f12, 0x0048},//TVAR_ae_BrAve
 //ae_StatMode bit[3] BLC has to be bypassed to prevent AE weight change especially backlight scene
   {0x002A, 0x148A},
   {0x0F12, 0x000F},   //ae_StatMode
   {0x002A, 0x058C},
   {0x0F12, 0x3520},
   {0x0F12, 0x0000},   //lt_uMaxExp1
   {0x0F12, 0xD4C0},
   {0x0F12, 0x0001},   //lt_uMaxExp2
   {0x0F12, 0x3520},
   {0x0F12, 0x0000},   //lt_uCapMaxExp1
   {0x0F12, 0xD4C0},
   {0x0F12, 0x0001},   //lt_uCapMaxExp2
   {0x002A, 0x059C},
   {0x0F12, 0x0380},   //lt_uMaxAnGain1
   {0x0F12, 0x0700},   //lt_uMaxAnGain2
   {0x0F12, 0x0100},   //lt_uMaxDigGain
   {0x0F12, 0x1000},   //lt_uMaxTotGain
   {0x002A, 0x0544},
   {0x0F12, 0x0111},   //lt_uLimitHigh
   {0x0F12, 0x00EF},   //lt_uLimitLow
   {0x002A, 0x0608},
   {0x0F12, 0x0001},   //lt_ExpGain_uSubsamplingmode
   {0x0F12, 0x0001},   //lt_ExpGain_uNonSubsampling
   {0x0F12, 0x0800},   //lt_ExpGain_ExpCurveGainMaxStr
   {0x0F12, 0x0100},   //0100   //lt_ExpGain_ExpCurveGainMaxStr_0__uMaxDigGain
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0x0000},   //0000   //lt_ExpGain_ExpCurveGainMaxStr_0__ulExpIn_0_
   {0x0F12, 0x0A3C},   //0A3C
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x0D05},   //0D05
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x4008},   //4008
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x7000},   //7400  //?? //700Lux
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x9C00},   //C000  //?? //9C00->9F->A5 //400Lux
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xAD00},   //AD00
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0xF1D4},   //F1D4
   {0x0F12, 0x0002},   //0002
   {0x0F12, 0xDC00},   //DC00
   {0x0F12, 0x0005},   //0005
   {0x0F12, 0xDC00},   //DC00
   {0x0F12, 0x0005},   //0005         //
   {0x002A, 0x0638},   //0638
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0x0000},   //0000   //lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_
   {0x0F12, 0x0A3C},   //0A3C
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x0D05},   //0D05
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x3408},   //3408
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x3408},   //3408
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x6810},   //6810
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x8214},   //8214
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xC350},   //C350
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xD4C0},   //C350
   {0x0F12, 0x0001},   //0000
   {0x0F12, 0xD4C0},   //C350
   {0x0F12, 0x0001},   //0000
   {0x002A, 0x0660},
   {0x0F12, 0x0650},   //lt_ExpGain_ExpCurveGainMaxStr_1_
   {0x0F12, 0x0100},   //lt_ExpGain_ExpCurveGainMaxStr_1__uMaxDigGain
   {0x002A, 0x06B8},
   {0x0F12, 0x452C},
   {0x0F12, 0x0007},   //0005   //lt_uMaxLei
   {0x002A, 0x05D0},
   {0x0F12, 0x0000},   //lt_mbr_Peak_behind 
	
	


	
    //================================================================================== 
    // 13.AE Weight (Normal)                                                             
    //================================================================================== 
	{0x002a, 0x1492},
	{0x0f12, 0x0100},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0000},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0201},
	{0x0f12, 0x0303},
	{0x0f12, 0x0303},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0403},
	{0x0f12, 0x0304},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0403},
	{0x0f12, 0x0304},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0403},
	{0x0f12, 0x0304},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0303},
	{0x0f12, 0x0303},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0202},
	{0x0f12, 0x0202},
	{0x0f12, 0x0102},
	
	{0x002a, 0x0484},
	{0x0f12, 0x0002},
	{0x002a, 0x183a},
	{0x0f12, 0x0001},
	{0x002a, 0x17f6},
	{0x0f12, 0x023c},
	{0x0f12, 0x0248},
	{0x002a, 0x1840},
	{0x0f12, 0x0001},
	{0x0f12, 0x0100},
	{0x0f12, 0x0120},
	{0x0f12, 0x0180},
	{0x0f12, 0x0200},
	{0x0f12, 0x0400},
	{0x0f12, 0x0800},
	{0x0f12, 0x0a00},
	{0x0f12, 0x1000},
	{0x0f12, 0x0100},
	{0x0f12, 0x00a0},
	{0x0f12, 0x0090},
	{0x0f12, 0x0080},
	{0x0f12, 0x0070},
	{0x0f12, 0x0045},
	{0x0f12, 0x0030},
	{0x0f12, 0x0010},
	{0x002a, 0x1884},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x002a, 0x1826},
	{0x0f12, 0x0100},
	{0x0f12, 0x00c0},
	{0x0f12, 0x0080},
	{0x0f12, 0x000a},
	{0x0f12, 0x0000},
	{0x0f12, 0x0030},
	{0x0f12, 0x0040},
	{0x0f12, 0x0048},
	{0x0f12, 0x0050},
	{0x0f12, 0x0060},
	{0x002a, 0x4784},
	{0x0f12, 0x00a0},
	{0x0f12, 0x00c0},
	{0x0f12, 0x00d0},
	{0x0f12, 0x0100},
	{0x0f12, 0x0200},
	{0x0f12, 0x0300},
	{0x0f12, 0x0088},
	{0x0f12, 0x00b0},
	{0x0f12, 0x00c0},
	{0x0f12, 0x0100},
	{0x0f12, 0x0200},
	{0x0f12, 0x0300},
	{0x002a, 0x479c},
	{0x0f12, 0x0120},
	{0x0f12, 0x0150},
	{0x0f12, 0x0200},
	{0x0f12, 0x003c},
	{0x0f12, 0x003b},
	{0x0f12, 0x0026},
	

	

    //==================================================================================
    // 15.CCM Setting
    //==================================================================================
 {0x002A, 0x08A6},
 {0x0F12, 0x00C0}, //SARR_AwbCcmCord[0]
 {0x0F12, 0x0100}, //SARR_AwbCcmCord[1]
 {0x0F12, 0x0125}, //SARR_AwbCcmCord[2]
 {0x0F12, 0x015F}, //SARR_AwbCcmCord[3]
 {0x0F12, 0x017C}, //SARR_AwbCcmCord[4]
 {0x0F12, 0x0194}, //SARR_AwbCcmCord[5]
 {0x002A, 0x0898},
 {0x0F12, 0x4800}, //TVAR_wbt_pBaseCcms
 {0x0F12, 0x7000},
 {0x002A, 0x08A0},
 {0x0F12, 0x48D8}, //TVAR_wbt_pOutdoorCcm
 {0x0F12, 0x7000},
//Horizon
 {0x002A, 0x4800},
 {0x0F12, 0x016C}, //TVAR_wbt_pBaseCcms[0]
 {0x0F12, 0xFF94}, //TVAR_wbt_pBaseCcms[1]
 {0x0F12, 0xFFCE}, //TVAR_wbt_pBaseCcms[2]
 {0x0F12, 0xFF20}, //TVAR_wbt_pBaseCcms[3]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[4]
 {0x0F12, 0xFF53}, //TVAR_wbt_pBaseCcms[5]
 {0x0F12, 0x003F}, //TVAR_wbt_pBaseCcms[6]
 {0x0F12, 0x0007}, //TVAR_wbt_pBaseCcms[7]
 {0x0F12, 0x01DF}, //TVAR_wbt_pBaseCcms[8]
 {0x0F12, 0x0110}, //TVAR_wbt_pBaseCcms[9]
 {0x0F12, 0x00DF}, //TVAR_wbt_pBaseCcms[10]
 {0x0F12, 0xFF47}, //TVAR_wbt_pBaseCcms[11]
 {0x0F12, 0x0206}, //TVAR_wbt_pBaseCcms[12]
 {0x0F12, 0xFF7F}, //TVAR_wbt_pBaseCcms[13]
 {0x0F12, 0x0191}, //TVAR_wbt_pBaseCcms[14]
 {0x0F12, 0xFF06}, //TVAR_wbt_pBaseCcms[15]
 {0x0F12, 0x01BA}, //TVAR_wbt_pBaseCcms[16]
 {0x0F12, 0x0108}, //TVAR_wbt_pBaseCcms[17]

// INCA A
 {0x0F12, 0x01EB}, //TVAR_wbt_pBaseCcms[18]
 {0x0F12, 0xFFD1}, //TVAR_wbt_pBaseCcms[19]
 {0x0F12, 0xFFDC}, //TVAR_wbt_pBaseCcms[20]
 {0x0F12, 0xFF2B}, //TVAR_wbt_pBaseCcms[21]
 {0x0F12, 0x0200}, //TVAR_wbt_pBaseCcms[22]
 {0x0F12, 0xFEFD}, //TVAR_wbt_pBaseCcms[23]
 {0x0F12, 0x001C}, //TVAR_wbt_pBaseCcms[24]
 {0x0F12, 0xFFE4}, //TVAR_wbt_pBaseCcms[25]
 {0x0F12, 0x01CD}, //TVAR_wbt_pBaseCcms[26]
 {0x0F12, 0x00E3}, //TVAR_wbt_pBaseCcms[27]
 {0x0F12, 0x00E3}, //TVAR_wbt_pBaseCcms[28]
 {0x0F12, 0xFE80}, //TVAR_wbt_pBaseCcms[29]
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[30]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[31]
 {0x0F12, 0x0199}, //TVAR_wbt_pBaseCcms[32]
 {0x0F12, 0xFF06}, //TVAR_wbt_pBaseCcms[33]
 {0x0F12, 0x01BA}, //TVAR_wbt_pBaseCcms[34]
 {0x0F12, 0x0108}, //TVAR_wbt_pBaseCcms[35]
//Warm White
 {0x0F12, 0x022B}, //TVAR_wbt_pBaseCcms[36]
 {0x0F12, 0xFF8C}, //TVAR_wbt_pBaseCcms[37]
 {0x0F12, 0xFFD8}, //TVAR_wbt_pBaseCcms[38]
 {0x0F12, 0xFECA}, //TVAR_wbt_pBaseCcms[39]
 {0x0F12, 0x0192}, //TVAR_wbt_pBaseCcms[40]
 {0x0F12, 0xFF0E}, //TVAR_wbt_pBaseCcms[41]
 {0x0F12, 0xFFD2}, //TVAR_wbt_pBaseCcms[42]
 {0x0F12, 0xFFD2}, //TVAR_wbt_pBaseCcms[43]
 {0x0F12, 0x01C8}, //TVAR_wbt_pBaseCcms[44]
 {0x0F12, 0x00F9}, //TVAR_wbt_pBaseCcms[45]
 {0x0F12, 0x00DE}, //TVAR_wbt_pBaseCcms[46]
 {0x0F12, 0xFF26}, //TVAR_wbt_pBaseCcms[47]
 {0x0F12, 0x0151}, //TVAR_wbt_pBaseCcms[48]
 {0x0F12, 0xFF50}, //TVAR_wbt_pBaseCcms[49]
 {0x0F12, 0x0147}, //TVAR_wbt_pBaseCcms[50]
 {0x0F12, 0xFF86}, //TVAR_wbt_pBaseCcms[51]
 {0x0F12, 0x01B4}, //TVAR_wbt_pBaseCcms[52]
 {0x0F12, 0x017E}, //TVAR_wbt_pBaseCcms[53]
//Cool White
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[54]
 {0x0F12, 0xFFD3}, //TVAR_wbt_pBaseCcms[55]
 {0x0F12, 0xFFE9}, //TVAR_wbt_pBaseCcms[56]
 {0x0F12, 0xFF5B}, //TVAR_wbt_pBaseCcms[57]
 {0x0F12, 0x025A}, //TVAR_wbt_pBaseCcms[58]
 {0x0F12, 0xFF80}, //TVAR_wbt_pBaseCcms[59]
 {0x0F12, 0xFFC8}, //TVAR_wbt_pBaseCcms[60]
 {0x0F12, 0xFFC1}, //TVAR_wbt_pBaseCcms[61]
 {0x0F12, 0x013A}, //TVAR_wbt_pBaseCcms[62]
 {0x0F12, 0x0112}, //TVAR_wbt_pBaseCcms[63]
 {0x0F12, 0x00EE}, //TVAR_wbt_pBaseCcms[64]
 {0x0F12, 0xFF99}, //TVAR_wbt_pBaseCcms[65]
 {0x0F12, 0x009E}, //TVAR_wbt_pBaseCcms[66]
 {0x0F12, 0xFF5F}, //TVAR_wbt_pBaseCcms[67]
 {0x0F12, 0x01A8}, //TVAR_wbt_pBaseCcms[68]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[69]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[70]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[71]
//D50
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[72]
 {0x0F12, 0xFFD3}, //TVAR_wbt_pBaseCcms[73]
 {0x0F12, 0xFFE9}, //TVAR_wbt_pBaseCcms[74]
 {0x0F12, 0xFF5B}, //TVAR_wbt_pBaseCcms[75]
 {0x0F12, 0x025A}, //TVAR_wbt_pBaseCcms[76]
 {0x0F12, 0xFF80}, //TVAR_wbt_pBaseCcms[77]
 {0x0F12, 0xFFC8}, //TVAR_wbt_pBaseCcms[78]
 {0x0F12, 0xFFC1}, //TVAR_wbt_pBaseCcms[79]
 {0x0F12, 0x013A}, //TVAR_wbt_pBaseCcms[80]
 {0x0F12, 0x0112}, //TVAR_wbt_pBaseCcms[81]
 {0x0F12, 0x00EE}, //TVAR_wbt_pBaseCcms[82]
 {0x0F12, 0xFF99}, //TVAR_wbt_pBaseCcms[83]
 {0x0F12, 0x009E}, //TVAR_wbt_pBaseCcms[84]
 {0x0F12, 0xFF5F}, //TVAR_wbt_pBaseCcms[85]
 {0x0F12, 0x01A8}, //TVAR_wbt_pBaseCcms[86]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[87]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[88]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[89]
//D65
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[90]
 {0x0F12, 0xFFD3}, //TVAR_wbt_pBaseCcms[91]
 {0x0F12, 0xFFE9}, //TVAR_wbt_pBaseCcms[92]
 {0x0F12, 0xFF5B}, //TVAR_wbt_pBaseCcms[93]
 {0x0F12, 0x025A}, //TVAR_wbt_pBaseCcms[94]
 {0x0F12, 0xFF80}, //TVAR_wbt_pBaseCcms[95]
 {0x0F12, 0xFFC8}, //TVAR_wbt_pBaseCcms[96]
 {0x0F12, 0xFFC1}, //TVAR_wbt_pBaseCcms[97]
 {0x0F12, 0x013A}, //TVAR_wbt_pBaseCcms[98]
 {0x0F12, 0x0112}, //TVAR_wbt_pBaseCcms[99]
 {0x0F12, 0x00EE}, //TVAR_wbt_pBaseCcms[100]
 {0x0F12, 0xFF99}, //TVAR_wbt_pBaseCcms[101]
 {0x0F12, 0x009E}, //TVAR_wbt_pBaseCcms[102]
 {0x0F12, 0xFF5F}, //TVAR_wbt_pBaseCcms[103]
 {0x0F12, 0x01A8}, //TVAR_wbt_pBaseCcms[104]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[105]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[106]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[107]
//Outdoor
 {0x002A, 0x48d8},	
 {0x0F12, 0x01AA}, //TVAR_wbt_pOutdoorCcm[0]
 {0x0F12, 0xFFB6}, //TVAR_wbt_pOutdoorCcm[1]
 {0x0F12, 0xFFD4}, //TVAR_wbt_pOutdoorCcm[2]
 {0x0F12, 0xFF6C}, //TVAR_wbt_pOutdoorCcm[3]
 {0x0F12, 0x01E8}, //TVAR_wbt_pOutdoorCcm[4]
 {0x0F12, 0xFF79}, //TVAR_wbt_pOutdoorCcm[5]
 {0x0F12, 0x0000}, //TVAR_wbt_pOutdoorCcm[6]
 {0x0F12, 0xFFF5}, //TVAR_wbt_pOutdoorCcm[7]
 {0x0F12, 0x023C}, //TVAR_wbt_pOutdoorCcm[8]
 {0x0F12, 0x00BF}, //TVAR_wbt_pOutdoorCcm[9]
 {0x0F12, 0x00E0}, //TVAR_wbt_pOutdoorCcm[10]
 {0x0F12, 0xFF5B}, //TVAR_wbt_pOutdoorCcm[11]
 {0x0F12, 0x022D}, //TVAR_wbt_pOutdoorCcm[12]
 {0x0F12, 0xFF9B}, //TVAR_wbt_pOutdoorCcm[13]
 {0x0F12, 0x01E1}, //TVAR_wbt_pOutdoorCcm[14]
 {0x0F12, 0xFF34}, //TVAR_wbt_pOutdoorCcm[15]
 {0x0F12, 0x014B}, //TVAR_wbt_pOutdoorCcm[16]
 {0x0F12, 0x01B6}, //TVAR_wbt_pOutdoorCcm[17]

	//16.gamma
		{0x002A, 0x0734},
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[0][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[0][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[0][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[0][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[0][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[0][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[0][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[0][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[0][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[0][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[0][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[0][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[0][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[0][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[0][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[0][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[0][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[0][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[0][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[0][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[1][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[1][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[1][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[1][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[1][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[1][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[1][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[1][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[1][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[1][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[1][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[1][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[1][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[1][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[1][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[1][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[1][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[1][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[1][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[1][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[2][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[2][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[2][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[2][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[2][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[2][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[2][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[2][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[2][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[2][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[2][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[2][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[2][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[2][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[2][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[2][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[2][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[2][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[2][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[2][19] 


//================================================================================== 
// 17.AFIT                                                                           
//==================================================================================            
  {0x002a, 0x0944},
  {0x0f12, 0x0010}, //afit_uNoiseIndInDoor 0x0050                                                                              
	{0x0f12, 0x0020}, //afit_uNoiseIndInDoor 0x00b0                                                                              
	{0x0f12, 0x0100}, //afit_uNoiseIndInDoor    0x0196                                                                           
	{0x0f12, 0x0245}, //afit_uNoiseIndInDoor                                                                              
	{0x0f12, 0x0300}, //afit_uNoiseIndInDoor                                                                              
	{0x002a, 0x0938},                                                                                                     
	{0x0f12, 0x0000}, // on/off AFIT by NB option                                                                         
	{0x0f12, 0x0014}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x00d2}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x0384}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x07d0}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x1388}, //SARR_uNormBrInDoor                                                                                
	{0x002a, 0x0976},                                                                                                     
	{0x0f12, 0x0070}, //afit_usGamutTh                                                                                    
	{0x0f12, 0x0005}, //afit_usNeargrayOffset                                                                             
	{0x0f12, 0x0000}, //afit_bUseSenBpr                                                                                   
	{0x0f12, 0x01cc}, //afit_usBprThr_0_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_1_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_2_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_3_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_4_                                                                                  
	{0x0f12, 0x0180}, //afit_NIContrastAFITValue                                                                          
	{0x0f12, 0x0196}, //afit_NIContrastTh                                                                                 
	{0x002a, 0x098c},                                                                                                     
	{0x0f12, 0x0000}, //7000098C//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0xFFF0}, //7000098E//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000990//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000992//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000994//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00C0}, //70000996//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000998//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //7000099A//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //7000099C//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01F4}, //7000099E//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //700009A0//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //700009A2//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00A0}, //700009A4//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //700009A6//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //700009A8//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0040}, //700009AA//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x00A0}, //700009AC//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //700009AE//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //700009B0//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //700009B2//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x3604}, //700009B4//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x032A}, //700009B6//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0403}, //700009B8//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x1B06}, //700009BA//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x6015}, //700009BC//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x00C0}, //700009BE//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x6080}, //700009C0//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x4080}, //700009C2//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0640}, //700009C4//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //700009C6//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //700009C8//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xFF01}, //700009CA//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0000}, //700009CC//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0400}, //700009CE//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x365A}, //700009D0//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x102A}, //700009D2//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000B}, //700009D4//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0600}, //700009D6//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5A0F}, //700009D8//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0505}, //700009DA//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //700009DC//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //700009DE//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //700009E0//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x7028}, //700009E2//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0468}, //700009E4//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh 0x0418                                           
	{0x0f12, 0x0101}, //700009E6//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x0800}, //700009E8//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x1804}, //700009EA//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //700009EC//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //700009EE//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //700009F0//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //700009F2//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //700009F4//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //700009F6//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //700009F8//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1E10}, //700009FA//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000B}, //700009FC//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //700009FE//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000A00//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000A02//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000A04//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000A06//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000A08//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000A0A//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000A0C//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000A0E//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x1C04}, //70000A10//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0214}, //70000A12//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1002}, //70000A14//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0C10}, //70000A16//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1A0C}, //70000A18//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x6018}, //70000A1A//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000A1C//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000A1E//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000A20//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0A0A}, //70000A22//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000A24//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x2A36}, //70000A26//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh                                
	{0x0f12, 0x7A52}, //70000A28//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x4A5C}, //70000A2A//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xFFFF}, //70000A2C//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000A2E//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0A01}, //70000A30//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010A}, //70000A32//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x3601}, //70000A34//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x602A}, //70000A36//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x5657}, //70000A38//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xFF37}, //70000A3A//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08FF}, //70000A3C//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000A3E//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000A40//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000A42//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0xFFF0}, //70000A44//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000A46//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000A48//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000A4A//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00C0}, //70000A4C//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000A4E//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000A50//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000A52//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01F4}, //70000A54//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000A56//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000A58//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00A0}, //70000A5A//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000A5C//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000A5E//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000A60//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000A62//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000A64//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000A66//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000A68//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x2404}, //70000A6A//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x031B}, //70000A6C//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0103}, //70000A6E//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x1205}, //70000A70//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x400D}, //70000A72//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0080}, //70000A74//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x2080}, //70000A76//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x3040}, //70000A78//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0630}, //70000A7A//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000A7C//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000A7E//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xFF01}, //70000A80//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000A82//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000A84//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x245A}, //70000A86//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1018}, //70000A88//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000B}, //70000A8A//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0B00}, //70000A8C//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5A0F}, //70000A8E//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0505}, //70000A90//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000A92//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000A94//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000A96//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x7428}, //70000A98//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x046C}, //70000A9A//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh   0x041c                                         
	{0x0f12, 0x0101}, //70000A9C//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x0800}, //70000A9E//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x1004}, //70000AA0//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000AA2//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000AA4//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000AA6//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000AA8//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000AAA//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000AAC//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000AAE//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1E10}, //70000AB0//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000B}, //70000AB2//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000AB4//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000AB6//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000AB8//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000ABA//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000ABC//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000ABE//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000AC0//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000AC2//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000AC4//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x1F04}, //70000AC6//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0218}, //70000AC8//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1102}, //70000ACA//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0911}, //70000ACC//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1A09}, //70000ACE//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x6018}, //70000AD0//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000AD2//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000AD4//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000AD6//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0A0A}, //70000AD8//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000ADA//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x1B24}, //70000ADC//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x1b24                               
	{0x0f12, 0x8363}, //70000ADE//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x4750}, //70000AE0//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xFFFF}, //70000AE2//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000AE4//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0A01}, //70000AE6//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010A}, //70000AE8//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x2401}, //70000AEA//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x6A1B}, //70000AEC//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x4C63}, //70000AEE//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xFF2C}, //70000AF0//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08FF}, //70000AF2//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000AF4//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000AF6//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000AF8//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0xFFF0}, //70000AFA//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000AFC//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000AFE//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000B00//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00C0}, //70000B02//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000B04//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000B06//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000B08//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01F4}, //70000B0A//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000B0C//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000B0E//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00A0}, //70000B10//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000B12//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000B14//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000B16//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000B18//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000B1A//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000B1C//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000B1E//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x1B04}, //70000B20//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x0312}, //70000B22//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000B24//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0C03}, //70000B26//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x2806}, //70000B28//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0060}, //70000B2A//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x1580}, //70000B2C//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x2020}, //70000B2E//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000B30//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000B32//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000B34//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xFF01}, //70000B36//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000B38//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000B3A//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145A}, //70000B3C//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000B3E//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000B}, //70000B40//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0E00}, //70000B42//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5A0F}, //70000B44//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0504}, //70000B46//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000B48//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000B4A//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000B4C//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x7828}, //70000B4E//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0478}, //70000B50//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh  0x0428                                          
	{0x0f12, 0x0101}, //70000B52//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x8000}, //70000B54//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0A04}, //70000B56//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000B58//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000B5A//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000B5C//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000B5E//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000B60//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000B62//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000B64//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1E10}, //70000B66//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000B}, //70000B68//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000B6A//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000B6C//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000B6E//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000B70//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000B72//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000B74//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000B76//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000B78//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000B7A//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2404}, //70000B7C//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0221}, //70000B7E//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1202}, //70000B80//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0513}, //70000B82//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1A07}, //70000B84//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x6818}, //70000B86//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000B88//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000B8A//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000B8C//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0A0A}, //70000B8E//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000B90//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x141D}, //70000B92//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh  0x141d                              
	{0x0f12, 0x907E}, //70000B94//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x4140}, //70000B96//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xFFFF}, //70000B98//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000B9A//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0A01}, //70000B9C//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010A}, //70000B9E//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x1B01}, //70000BA0//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x7612}, //70000BA2//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x4471}, //70000BA4//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xFF25}, //70000BA6//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08FF}, //70000BA8//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000BAA//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000BAC//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000BAE//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0xFFF0}, //70000BB0//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000BB2//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000BB4//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000BB6//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00C0}, //70000BB8//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000BBA//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000BBC//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000BBE//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01F4}, //70000BC0//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000BC2//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000BC4//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00A0}, //70000BC6//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000BC8//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000BCA//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000BCC//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000BCE//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000BD0//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000BD2//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000BD4//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x1504}, //70000BD6//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x030F}, //70000BD8//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000BDA//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0902}, //70000BDC//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x2004}, //70000BDE//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0050}, //70000BE0//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x1140}, //70000BE2//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x201C}, //70000BE4//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000BE6//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000BE8//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000BEA//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xFF01}, //70000BEC//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000BEE//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000BF0//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145A}, //70000BF2//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000BF4//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000B}, //70000BF6//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x1000}, //70000BF8//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5A0F}, //70000BFA//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0503}, //70000BFC//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000BFE//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000C00//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000C02//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x7C28}, //70000C04//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x047C}, //70000C06//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh  0x042c                                          
	{0x0f12, 0x0101}, //70000C08//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0xFF00}, //70000C0A//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0904}, //70000C0C//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000C0E//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000C10//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000C12//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000C14//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000C16//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000C18//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000C1A//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1E10}, //70000C1C//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000B}, //70000C1E//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000C20//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000C22//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000C24//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000C26//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000C28//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000C2A//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000C2C//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0305}, //70000C2E//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0406}, //70000C30//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2804}, //70000C32//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0228}, //70000C34//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1402}, //70000C36//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0418}, //70000C38//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1A06}, //70000C3A//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x7018}, //70000C3C//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000C3E//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000C40//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000C42//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0A0A}, //70000C44//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000C46//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x1117}, //70000C48//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x1117                               
	{0x0f12, 0x9587}, //70000C4A//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x3C37}, //70000C4C//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xFFFF}, //70000C4E//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000C50//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0A01}, //70000C52//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010A}, //70000C54//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x1501}, //70000C56//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x7B0F}, //70000C58//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x3D7D}, //70000C5A//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xFF1E}, //70000C5C//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08FF}, //70000C5E//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000C60//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000C62//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000C64//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0xFFF0}, //70000C66//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000C68//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000C6A//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000C6C//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00C0}, //70000C6E//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000C70//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000C72//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000C74//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01F4}, //70000C76//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000C78//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000C7A//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00A0}, //70000C7C//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000C7E//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000C80//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000C82//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000C84//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000C86//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000C88//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000C8A//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x0F04}, //70000C8C//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x030C}, //70000C8E//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000C90//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0602}, //70000C92//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x1803}, //70000C94//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0040}, //70000C96//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x0E20}, //70000C98//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x2018}, //70000C9A//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000C9C//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000C9E//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000CA0//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xFF01}, //70000CA2//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000CA4//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0200}, //70000CA6//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145A}, //70000CA8//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000CAA//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000B}, //70000CAC//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x1200}, //70000CAE//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5A0F}, //70000CB0//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0502}, //70000CB2//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000CB4//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000CB6//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000CB8//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x8028}, //70000CBA//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0480}, //70000CBC//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh 0x0430                                            
	{0x0f12, 0x0101}, //70000CBE//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0xFF00}, //70000CC0//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0804}, //70000CC2//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000CC4//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000CC6//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000CC8//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000CCA//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000CCC//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000CCE//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000CD0//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1E10}, //70000CD2//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000B}, //70000CD4//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000CD6//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000CD8//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000CDA//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000CDC//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000CDE//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000CE0//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000CE2//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000CE4//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000CE6//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2C04}, //70000CE8//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x022C}, //70000CEA//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1402}, //70000CEC//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0418}, //70000CEE//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1A05}, //70000CF0//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x7018}, //70000CF2//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000CF4//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000CF6//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000CF8//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0A0A}, //70000CFA//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000CFC//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0C0F}, //70000CFE//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x0c0f                               
	{0x0f12, 0x9C8A}, //70000D00//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x3930}, //70000D02//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xFFFF}, //70000D04//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000D06//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0A01}, //70000D08//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010A}, //70000D0A//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x0F01}, //70000D0C//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x830C}, //70000D0E//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x397F}, //70000D10//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xFF19}, //70000D12//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08FF}, //70000D14//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000D16//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001},   //70000D18 AFITB_bnr_nClustLevel_C    [0]   bWideWide[1]                                          
	{0x0f12, 0x23ce}, //70000D19//ConstAfitBaseVals                                                                       
	{0x0f12, 0xfdc8}, //70000D1A//ConstAfitBaseVals                                                                       
	{0x0f12, 0x112e}, //70000D1B//ConstAfitBaseVals                                                                       
	{0x0f12, 0x93a5}, //70000D1C//ConstAfitBaseVals                                                                       
	{0x0f12, 0xfe67}, //70000D1D//ConstAfitBaseVals                                                                       
	{0x0f12, 0x0000}, //70000D1E//ConstAfitBaseVals   
	                                                                    
	{0x002a, 0x0250},
	{0x0f12, 0x0A00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000C},
	{0x0f12, 0x0A00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000C},
	{0x002a, 0x0494},
	{0x0f12, 0x0A00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0A00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	
	{0x002a, 0x02A6},
	{0x0f12, 0x0280},//#REG_0TC_PCFG_usWidth//Hsize:800//        //PCFG                                   
	{0x0f12, 0x01e0},//#REG_0TC_PCFG_usHeight//Vsize:600//       //output image width:800                 
	{0x0f12, 0x0005},//#REG_0TC_PCFG_Format//5:YUV7:Raw9:JPG//   //output image height:600                
	{0x0f12, 0x4f1a},//#REG_0TC_PCFG_usMaxOut4KHzRate//          //yuv                                    
	{0x0f12, 0x4f1a},//#REG_0TC_PCFG_usMinOut4KHzRate//                                                   
	{0x0f12, 0x0100},//#REG_0TC_PCFG_OutClkPerPix88//                                                     
	{0x0f12, 0x0300},//#REG_0TC_PCFG_uBpp88//                                                             
	{0x0f12, 0x0052},//#REG_0TC_PCFG_PVIMask//                                                            
	{0x0f12, 0x0000},//#REG_0TC_PCFG_OIFMask//                                                            
	{0x0f12, 0x01e0},//#REG_0TC_PCFG_usJpegPacketSize//                                                   
	{0x0f12, 0x0000},//#REG_0TC_PCFG_usJpegTotalPackets//                                                 
	{0x0f12, 0x0000},//#REG_0TC_PCFG_uClockInd//                                                          
	{0x0f12, 0x0000},//#REG_0TC_PCFG_usFrTimeType//                                                       
	{0x0f12, 0x0001},//#REG_0TC_PCFG_FrRateQualityType//    0x0001   0x0002                                           
	{0x0f12, 0x01f4},//29A//#REG_0TC_PCFG_usMaxFrTimeMsecMult10//                                         
	{0x0f12, 0x0000},//14D//#REG_0TC_PCFG_usMinFrTimeMsecMult10//                                         
	{0x002a, 0x02d0},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	
	{0x002a, 0x0396},//CGFG
	{0x0f12, 0x0001},//#REG_0TC_CCFG_uCaptureMode//         
	{0x0f12, 0x0A00},//#REG_0TC_CCFG_usWidth//              //
	{0x0f12, 0x0780},//#REG_0TC_CCFG_usHeight//             //
	{0x0f12, 0x0005},//#REG_0TC_CCFG_Format//               //
	{0x0f12, 0x4f1a},//#REG_0TC_CCFG_usMaxOut4KHzRate//     
	{0x0f12, 0x4f1a},//#REG_0TC_CCFG_usMinOut4KHzRate//     
	{0x0f12, 0x0100},//#REG_0TC_CCFG_OutClkPerPix88//       
	{0x0f12, 0x0300},//#REG_0TC_CCFG_uBpp88//               
	{0x0f12, 0x0052},//#REG_0TC_CCFG_PVIMask//              
	{0x0f12, 0x0000},//#REG_0TC_CCFG_OIFMask//              
	{0x0f12, 0x01e0},//#REG_0TC_CCFG_usJpegPacketSize//     
	{0x0f12, 0x0000},//#REG_0TC_CCFG_usJpegTotalPackets//   
	{0x0f12, 0x0000},//#REG_0TC_CCFG_uClockInd//            
	{0x0f12, 0x0000},//#REG_0TC_CCFG_usFrTimeType//         
	{0x0f12, 0x0002},//#REG_0TC_CCFG_FrRateQualityType//    
	{0x0f12, 0x0535},//#REG_0TC_CCFG_usMaxFrTimeMsecMult10//
	{0x0f12, 0x0000},//#REG_0TC_CCFG_usMinFrTimeMsecMult10//

	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	SensorEnd
};

/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
  {0xFCFC, 0xD000},
  {0x0028, 0x7000},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x002a, 0x0242},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x0244},
	{0x0f12, 0x0001},
	{0xffff, 0x00f4},//delay
	SensorEnd
};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
  {0xFCFC, 0xD000},
  {0x0028, 0x7000},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x002a, 0x0242},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x0244},
	{0x0f12, 0x0001},
	{0xffff, 0x00f4},//delay
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
{0x0028, 0x7000},
{0x002A, 0x02A6},
{0x0F12, 0x0280},  //#REG_0TC_PCFG_usWidth
{0x0F12, 0x01e0},  //#REG_0TC_PCFG_usHeight
{0x0F12, 0x0005},  //#REG_0TC_PCFG_Format
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMaxOut4KHzRate
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMinOut4KHzRate
{0x0F12, 0x0100},  //#REG_0TC_PCFG_OutClkPerPix88
{0x0F12, 0x0300},  //#REG_0TC_PCFG_uBpp88
{0x0F12, 0x0052},  //#REG_0TC_PCFG_PVIMask
{0x0F12, 0x0000},  //#REG_0TC_PCFG_OIFMask
{0x0F12, 0x03c0},  //#REG_0TC_PCFG_usJpegPacketSize
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usJpegTotalPackets
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uClockInd
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usFrTimeType
{0x0F12, 0x0001},  //#REG_0TC_PCFG_FrRateQualityType
{0x0F12, 0x01f4},  //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usMinFrTimeMsecMult10
{0x002A, 0x02D0},
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uPrevMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uCaptureMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uRotation
{0x002A, 0x0250},
{0x0F12, 0x0A00},  //#REG_TC_GP_PrevReqInputWidth                     //Sensor Crop Width 2560
{0x0F12, 0x0780},  //#REG_TC_GP_PrevReqInputHeight                    //Sensor Crop Height 1920
{0x0F12, 0x0010},  //#REG_TC_GP_PrevInputWidthOfs                     //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x000C},  //#REG_TC_GP_PrevInputHeightOfs                    //Sensor VOffset         12 = (1944-1920)/2
{0x0F12, 0x0A00},  //#REG_TC_GP_CapReqInputWidth                      //Sensor Crop Width 2560
{0x0F12, 0x0780},  //#REG_TC_GP_CapReqInputHeight                     //Sensor Crop Height 1920
{0x0F12, 0x0010},  //#REG_TC_GP_CapInputWidthOfs                      //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x000C},  //#REG_TC_GP_CapInputHeightOfs                     //Sensor VOffset         12 = (1944-1920)/2
{0x002A, 0x0494},
{0x0F12, 0x0A00},  //#REG_TC_PZOOM_PrevZoomReqInputWidth              //ISP Input Width 2560
{0x0F12, 0x0780},  //#REG_TC_PZOOM_PrevZoomReqInputHeight             //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputWidthOfs           //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputHeightOfs          //ISP Input VOffset    0
{0x0F12, 0x0A00},  //#REG_TC_PZOOM_CapZoomReqInputWidth               //ISP Input Width 2560
{0x0F12, 0x0780},  //#REG_TC_PZOOM_CapZoomReqInputHeight              //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputWidthOfs            //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputHeightOfs           //ISP Input VOffset    0
{0x002A, 0x0262},
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInPre
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInCap
{0x002A, 0x0266},
{0x0F12, 0x0000},  //#REG_TC_GP_ActivePrevConfig
{0x002A, 0x026A},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevOpenAfterChange
{0x002A, 0x026E},
{0x0F12, 0x0000},	 //REG_TC_GP_ActiveCapConfig
{0x002A, 0x024E},
{0x0F12, 0x0001},  //#REG_TC_GP_NewConfigSync
{0x002A, 0x0268},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevConfigChanged
{0x002A, 0x0270},
{0x0F12, 0x0001},  //#REG_TC_GP_CapConfigChanged
{0x002A, 0x023E},
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreview
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreviewChanged
{0xffff, 0x00c8},
{0x0028, 0x7000},
{0x002A, 0x028C},
{0x0F12, 0x0003},
{0xffff, 0x000a},
{0x002A, 0x028C},
{0x0F12, 0x0006},
{0xffff, 0x0064},
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
{0x0028, 0x7000},
{0x002A, 0x02A6},
{0x0F12, 0x0500},  //#REG_0TC_PCFG_usWidth
{0x0F12, 0x02D0},  //#REG_0TC_PCFG_usHeight
{0x0F12, 0x0005},  //#REG_0TC_PCFG_Format
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMaxOut4KHzRate
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMinOut4KHzRate
{0x0F12, 0x0100},  //#REG_0TC_PCFG_OutClkPerPix88
{0x0F12, 0x0300},  //#REG_0TC_PCFG_uBpp88
{0x0F12, 0x0052},  //#REG_0TC_PCFG_PVIMask
{0x0F12, 0x0000},  //#REG_0TC_PCFG_OIFMask
{0x0F12, 0x03c0},  //#REG_0TC_PCFG_usJpegPacketSize
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usJpegTotalPackets
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uClockInd
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usFrTimeType
{0x0F12, 0x0001},  //#REG_0TC_PCFG_FrRateQualityType
{0x0F12, 0x01f4},  //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usMinFrTimeMsecMult10
{0x002A, 0x02D0},
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uPrevMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uCaptureMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uRotation
{0x002A, 0x0250},
{0x0F12, 0x0A00},  //#REG_TC_GP_PrevReqInputWidth                     //Sensor Crop Width 2560
{0x0F12, 0x05A0},  //#REG_TC_GP_PrevReqInputHeight                    //Sensor Crop Height 1920
{0x0F12, 0x0010},  //#REG_TC_GP_PrevInputWidthOfs                     //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x00FC},  //#REG_TC_GP_PrevInputHeightOfs                    //Sensor VOffset         12 = (1944-1920)/2
{0x0F12, 0x0A00},  //#REG_TC_GP_CapReqInputWidth                      //Sensor Crop Width 2560
{0x0F12, 0x05A0},  //#REG_TC_GP_CapReqInputHeight                     //Sensor Crop Height 1920
{0x0F12, 0x0010},  //#REG_TC_GP_CapInputWidthOfs                      //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x00FC},  //#REG_TC_GP_CapInputHeightOfs                     //Sensor VOffset         12 = (1944-1920)/2
{0x002A, 0x0494},
{0x0F12, 0x0A00},  //#REG_TC_PZOOM_PrevZoomReqInputWidth              //ISP Input Width 2560
{0x0F12, 0x05A0},  //#REG_TC_PZOOM_PrevZoomReqInputHeight             //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputWidthOfs           //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputHeightOfs          //ISP Input VOffset    0
{0x0F12, 0x0A00},  //#REG_TC_PZOOM_CapZoomReqInputWidth               //ISP Input Width 2560
{0x0F12, 0x05A0},  //#REG_TC_PZOOM_CapZoomReqInputHeight              //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputWidthOfs            //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputHeightOfs           //ISP Input VOffset    0
{0x002A, 0x0262},
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInPre
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInCap
{0x002A, 0x0266},
{0x0F12, 0x0000},  //#REG_TC_GP_ActivePrevConfig
{0x002A, 0x026A},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevOpenAfterChange
{0x002A, 0x026E},
{0x0F12, 0x0000},	 //REG_TC_GP_ActiveCapConfig
{0x002A, 0x024E},
{0x0F12, 0x0001},  //#REG_TC_GP_NewConfigSync
{0x002A, 0x0268},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevConfigChanged
{0x002A, 0x0270},
{0x0F12, 0x0001},  //#REG_TC_GP_CapConfigChanged
{0x002A, 0x023E},
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreview
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreviewChanged
{0xffff, 0x00c8},
{0x0028, 0x7000},
{0x002A, 0x028C},
{0x0F12, 0x0003},
{0xffff, 0x000a},
{0x002A, 0x028C},
{0x0F12, 0x0006},
{0xffff, 0x0064},
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
{0x0028, 0x7000},
{0x002A, 0x02A6},
{0x0F12, 0x0780},  //#REG_0TC_PCFG_usWidth
{0x0F12, 0x0438},  //#REG_0TC_PCFG_usHeight
{0x0F12, 0x0005},  //#REG_0TC_PCFG_Format
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMaxOut4KHzRate
{0x0F12, 0x4f1a},  //#REG_0TC_PCFG_usMinOut4KHzRate
{0x0F12, 0x0100},  //#REG_0TC_PCFG_OutClkPerPix88
{0x0F12, 0x0300},  //#REG_0TC_PCFG_uBpp88
{0x0F12, 0x0052},  //#REG_0TC_PCFG_PVIMask
{0x0F12, 0x0000},  //#REG_0TC_PCFG_OIFMask
{0x0F12, 0x03c0},  //#REG_0TC_PCFG_usJpegPacketSize
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usJpegTotalPackets
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uClockInd
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usFrTimeType
{0x0F12, 0x0002},  //#REG_0TC_PCFG_FrRateQualityType
{0x0F12, 0x0340},  //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
{0x0F12, 0x0000},  //#REG_0TC_PCFG_usMinFrTimeMsecMult10
{0x002A, 0x02D0},
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uPrevMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uCaptureMirror
{0x0F12, 0x0000},  //#REG_0TC_PCFG_uRotation
{0x002A, 0x0250},
{0x0F12, 0x0780},  //#REG_TC_GP_PrevReqInputWidth                     //Sensor Crop Width 2560
{0x0F12, 0x0438},  //#REG_TC_GP_PrevReqInputHeight                    //Sensor Crop Height 1920
{0x0F12, 0x0150},  //#REG_TC_GP_PrevInputWidthOfs                     //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x01b0},  //#REG_TC_GP_PrevInputHeightOfs                    //Sensor VOffset         12 = (1944-1920)/2
{0x0F12, 0x0780},  //#REG_TC_GP_CapReqInputWidth                      //Sensor Crop Width 2560
{0x0F12, 0x0438},  //#REG_TC_GP_CapReqInputHeight                     //Sensor Crop Height 1920
{0x0F12, 0x0150},  //#REG_TC_GP_CapInputWidthOfs                      //Sensor HOffset         16 = (2592-2560)/2
{0x0F12, 0x01b0},  //#REG_TC_GP_CapInputHeightOfs                     //Sensor VOffset         12 = (1944-1920)/2
{0x002A, 0x0494},
{0x0F12, 0x0780},  //#REG_TC_PZOOM_PrevZoomReqInputWidth              //ISP Input Width 2560
{0x0F12, 0x0438},  //#REG_TC_PZOOM_PrevZoomReqInputHeight             //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputWidthOfs           //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_PrevZoomReqInputHeightOfs          //ISP Input VOffset    0
{0x0F12, 0x0780},  //#REG_TC_PZOOM_CapZoomReqInputWidth               //ISP Input Width 2560
{0x0F12, 0x0438},  //#REG_TC_PZOOM_CapZoomReqInputHeight              //ISP Input Height 1920
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputWidthOfs            //ISP Input HOffset    0
{0x0F12, 0x0000},  //#REG_TC_PZOOM_CapZoomReqInputHeightOfs           //ISP Input VOffset    0
{0x002A, 0x0262},
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInPre
{0x0F12, 0x0001},  //#REG_TC_GP_bUseReqInputInCap
{0x002A, 0x0266},
{0x0F12, 0x0000},  //#REG_TC_GP_ActivePrevConfig
{0x002A, 0x026A},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevOpenAfterChange
{0x002A, 0x026E},
{0x0F12, 0x0000},	 //REG_TC_GP_ActiveCapConfig
{0x002A, 0x024E},
{0x0F12, 0x0001},  //#REG_TC_GP_NewConfigSync
{0x002A, 0x0268},
{0x0F12, 0x0001},  //#REG_TC_GP_PrevConfigChanged
{0x002A, 0x0270},
{0x0F12, 0x0001},  //#REG_TC_GP_CapConfigChanged
{0x002A, 0x023E},
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreview
{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreviewChanged
{0xffff, 0x00c8},
{0x0028, 0x7000},
{0x002A, 0x028C},
{0x0F12, 0x0003},
{0xffff, 0x000a},
{0x002A, 0x028C},
{0x0F12, 0x0006},
{0xffff, 0x0064},
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorRegVal(0xfcfc, 0xd000),
	SensorRegVal(0x0010, 0x0001),
	SensorRegVal(0x1030, 0x0000),
	SensorRegVal(0x0014, 0x0001),
	SensorRegVal(0xffff, 0x0010),//delay 10ms	
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorEnd
};

/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x077f},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x06d0},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x0520},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x0620},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x0580},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	//Office
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x06c0},
	{0x002a, 0x04be},
	{0x0f12, 0x0440},
	{0x002a, 0x04c2},
	{0x0f12, 0x07a0},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd

};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	//Home
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x0540},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x08f0},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3

	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0000},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0001},*/
	 SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0004},
*/
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
/*	//Negative
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0003},
*/
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0007},
*/
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//	Greenish
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0009},
*/
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0080},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00a0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00c0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00e0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0100},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0120},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0140},

	SensorEnd
};
/*
static	struct rk_sensor_reg sensor_Exposure7[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0160},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure8[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0180},

	SensorEnd
};
*/


static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,    //sensor_Exposure7,sensor_Exposure8,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
//	{0x3a00, 0x78},
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
//{0x002A, 0x0638},
//{0x0F12, 0x0001},
//{0x0F12, 0x0000},
//{0x0F12, 0x1478},
//{0x0F12, 0x0000},
//{0x0F12, 0x1A0A},
//{0x0F12, 0x0000},
//{0x0F12, 0x6810},
//{0x0F12, 0x0000},
//{0x0F12, 0x6810},
//{0x0F12, 0x0000},
//{0x0F12, 0xD020},
//{0x0F12, 0x0000},
//{0x0F12, 0x0428},
//{0x0F12, 0x0001},
//{0x0F12, 0x1A80},
//{0x0F12, 0x0006},
//{0x0F12, 0x1A80},
//{0x0F12, 0x0006},
//{0x0F12, 0x1A80},
//{0x0F12, 0x0006},
	//15fps ~ 3.75fps night mode for 60/50Hz light environment, 24Mhz clock input,24Mzh pclk
/*	{0x3034 ,0x1a},
	{0x3035 ,0x21},
	{0x3036 ,0x46},
	{0x3037 ,0x13},
	{0x3038 ,0x00},
	{0x3039 ,0x00},
	{0x3a00 ,0x7c},
	{0x3a08 ,0x01},
	{0x3a09 ,0x27},
	{0x3a0a ,0x00},
	{0x3a0b ,0xf6},
	{0x3a0d ,0x04},
	{0x3a0e ,0x04},
	{0x3a02 ,0x0b},
	{0x3a03 ,0x88},
	{0x3a14 ,0x0b},
	{0x3a15 ,0x88},*/
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG} 
};
/*static struct soc_camera_ops sensor_ops;*/


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
static int sensor_parameter_record(struct i2c_client *client)
{
#if 0
	u8 ret_l,ret_m,ret_h;
	int tp_l,tp_m,tp_h;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
	sensor_read(client,0x3a00, &ret_l);
	sensor_write(client,0x3a00, ret_l&0xfb);

	sensor_write(client,0x3503,0x07);	//stop AE/AG
	sensor_read(client,0x3406, &ret_l);
	sensor_write(client,0x3406, ret_l|0x01);

	sensor_read(client,0x3500,&ret_h);
	sensor_read(client,0x3501, &ret_m);
	sensor_read(client,0x3502, &ret_l);
	tp_l = ret_l;
	tp_m = ret_m;
	tp_h = ret_h;
	spsensor->parameter.preview_exposure = ((tp_h<<12) & 0xF000) | ((tp_m<<4) & 0x0FF0) | ((tp_l>>4) & 0x0F);
	
	//Read back AGC Gain for preview
	sensor_read(client,0x350b, &ret_l);
	spsensor->parameter.preview_gain = ret_l;
	
	SENSOR_DG(" %s Read 0x350b=0x%02x  PreviewExposure:%d 0x3500=0x%02x  0x3501=0x%02x 0x3502=0x%02x \n",
	 SENSOR_NAME_STRING(), tp_l,spsensor->parameter.preview_exposure,tp_h, tp_m, tp_l);
#endif
	return 0;
}
#define OV5640_FULL_PERIOD_PIXEL_NUMS_HTS		  (2844) 
#define OV5640_FULL_PERIOD_LINE_NUMS_VTS		  (1968) 
#define OV5640_PV_PERIOD_PIXEL_NUMS_HTS 		  (1896) 
#define OV5640_PV_PERIOD_LINE_NUMS_VTS			  (984) 
static int sensor_ae_transfer(struct i2c_client *client)
{
#if 0
	u8	ExposureLow;
	u8	ExposureMid;
	u8	ExposureHigh;
	u16 ulCapture_Exposure;
	u16 Preview_Maxlines;
	u8	Gain;
	u16 OV5640_g_iExtra_ExpLines;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
	//Preview_Maxlines = sensor->parameter.preview_line_width;
	Preview_Maxlines = spsensor->parameter.preview_maxlines;
	Gain = spsensor->parameter.preview_gain;
	
	
	ulCapture_Exposure = (spsensor->parameter.preview_exposure*OV5640_PV_PERIOD_PIXEL_NUMS_HTS)/OV5640_FULL_PERIOD_PIXEL_NUMS_HTS;
					
	SENSOR_DG("cap shutter calutaed = %d, 0x%x\n", ulCapture_Exposure,ulCapture_Exposure);
	
	// write the gain and exposure to 0x350* registers	
	sensor_write(client,0x350b, Gain);	

	if (ulCapture_Exposure <= 1940) {
		OV5640_g_iExtra_ExpLines = 0;
	}else {
		OV5640_g_iExtra_ExpLines = ulCapture_Exposure - 1940;
	}
	SENSOR_DG("Set Extra-line = %d, iExp = %d \n", OV5640_g_iExtra_ExpLines, ulCapture_Exposure);

	ExposureLow = (ulCapture_Exposure<<4)&0xff;
	ExposureMid = (ulCapture_Exposure>>4)&0xff;
	ExposureHigh = (ulCapture_Exposure>>12);
	
	sensor_write(client,0x350c, (OV5640_g_iExtra_ExpLines&0xff00)>>8);
	sensor_write(client,0x350d, OV5640_g_iExtra_ExpLines&0xff);
	sensor_write(client,0x3502, ExposureLow);
	sensor_write(client,0x3501, ExposureMid);
	sensor_write(client,0x3500, ExposureHigh);

	//SENSOR_DG(" %s Write 0x350b=0x%02x 0x350c=0x%2x  0x350d=0x%2x 0x3502=0x%02x 0x3501=0x%02x 0x3500=0x%02x\n",SENSOR_NAME_STRING(), Gain, ExposureLow, ExposureMid, ExposureHigh);
	mdelay(100);
#endif
	return 0;
}
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
	SENSOR_DG("%s",__FUNCTION__);
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
   // struct generic_sensor *sensor = to_generic_sensor(client);
    
	SENSOR_DG("%s",__FUNCTION__);
#if 0	
	if (sensor->info_priv.funmodule_state & SENSOR_INIT_IS_OK) {
	    sensor_write(client, 0x3017, 0x00);  // FREX,VSYNC,HREF,PCLK,D9-D6
        sensor_write(client, 0x3018, 0x03);  // D5-D0
        sensor_write(client,0x3019,0x00);    // STROBE,SDA
    }
#endif    
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	//struct generic_sensor*sensor = to_generic_sensor(client);	
	if (capture) {
		sensor_parameter_record(client);
	}

	return 0;
}
static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	u16 rdval = 0xffff;
	int ret;
	ret=sensor_write_reg2val2(client, 0x002c, 0x7000);
	ret=sensor_write_reg2val2(client, 0x002e, 0x01A4);
	ret=sensor_read_reg2val2(client,  0x0f12, &rdval);
	return rdval;
}

/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	//struct generic_sensor*sensor = to_generic_sensor(client);

	if (capture) {
		sensor_ae_transfer(client);
	}
	msleep(400);

	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}

	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	
	SENSOR_DG("mirror: %d",mirror);

	return 0;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback	
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);

	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	SENSOR_DG("flip: %d",flip);

	return 0;    
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback	
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);

	return 0;
}
/*
* the functions are focus callbacks
*/
/*
for 5640 focus
*/
struct af_cmdinfo
{
	char cmd_tag;
	char cmd_para[4];
	char validate_bit;
};
#if 0

static int sensor_af_cmdset(struct i2c_client *client, int cmd_main, struct af_cmdinfo *cmdinfo)
{
#if 0
	int i;
	char read_tag=0xff,cnt;

	if (cmdinfo) {
		for (i=0; i<4; i++) {
			if (cmdinfo->validate_bit & (1<<i)) {
				if (sensor_write(client, CMD_PARA0_Reg+i, cmdinfo->cmd_para[i])) {
					SENSOR_TR("%s write CMD_PARA_Reg(main:0x%x para%d:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main,i,cmdinfo->cmd_para[i]);
					goto sensor_af_cmdset_err;
				}
				SENSOR_DG("%s write CMD_PARA_Reg(main:0x%x para%d:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,i,cmdinfo->cmd_para[i]);
			}
		}
	
		if (cmdinfo->validate_bit & 0x80) {
			if (sensor_write(client, CMD_ACK_Reg, cmdinfo->cmd_tag)) {
				SENSOR_TR("%s write CMD_ACK_Reg(main:0x%x tag:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main,cmdinfo->cmd_tag);
				goto sensor_af_cmdset_err;
			}
			SENSOR_DG("%s write CMD_ACK_Reg(main:0x%x tag:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,cmdinfo->cmd_tag);
		}
		
	} else {
		if (sensor_write(client, CMD_ACK_Reg, 0x01)) {
			SENSOR_TR("%s write CMD_ACK_Reg(main:0x%x no tag) error!\n",SENSOR_NAME_STRING(),cmd_main);
			goto sensor_af_cmdset_err;
		}
		SENSOR_DG("%s write CMD_ACK_Reg(main:0x%x no tag) success!\n",SENSOR_NAME_STRING(),cmd_main);
	}

	if (sensor_write(client, CMD_MAIN_Reg, cmd_main)) {
		SENSOR_TR("%s write CMD_MAIN_Reg(main:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main);
		goto sensor_af_cmdset_err;
	}

	cnt = 0;
	do
	{
		msleep(5);
		if (sensor_read(client,CMD_ACK_Reg,&read_tag)){
		   SENSOR_TR("%s[%d] read TAG failed\n",SENSOR_NAME_STRING(),__LINE__);
		   break;
		}
	} while((read_tag != 0x00)&& (cnt++<100));

	SENSOR_DG("%s write CMD_MAIN_Reg(main:0x%x read tag:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,read_tag);
#endif
	return 0;
//sensor_af_cmdset_err:
//	return -1;
}
static int sensor_af_idlechk(struct i2c_client *client)
{

	int ret = 0;
#if 0

	char state; 
	struct af_cmdinfo cmdinfo;
	
	SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
	
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.validate_bit = 0x80;
	ret = sensor_af_cmdset(client, ReturnIdle_Cmd, &cmdinfo);
	if(0 != ret) {
		SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		ret = -1;
		goto sensor_af_idlechk_end;
	}
	

	do{
		ret = sensor_read(client, CMD_ACK_Reg, &state);
		if (ret != 0){
		   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		   ret = -1;
		   goto sensor_af_idlechk_end;
		}
	}while(0x00 != state);
#endif
	SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
//sensor_af_idlechk_end:
	return ret;
}
#endif
/*for 5640 focus end*/
//
static int sensor_focus_af_single_usr_cb(struct i2c_client *client);

static int sensor_focus_init_usr_cb(struct i2c_client *client)
{
    int ret = 0;// cnt;
  //  char state;
    
	sensor_write(client, 0x0028 , 0x7000);
	sensor_write(client, 0x002A , 0x3076);
	sensor_write(client, 0x0F12 , 0x0400);
	sensor_write(client, 0xFFFF , 0x000A);
	sensor_write(client, 0x002A , 0x028C);
	sensor_write(client, 0x0F12 , 0x0006);
   
  SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
    
#if 0

    msleep(1);
    ret = sensor_write_array(client, sensor_af_firmware);
    if (ret != 0) {
    	SENSOR_TR("%s Download firmware failed\n",SENSOR_NAME_STRING());
    	ret = -1;
    	goto sensor_af_init_end;
    }

    cnt = 0;
    do
    {
    	msleep(1);
    	if (cnt++ > 500)
    		break;
    	ret = sensor_read(client, STA_FOCUS_Reg, &state);
    	if (ret != 0){
    	   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
    	   ret = -1;
    	   goto sensor_af_init_end;
    	}
    } while (state != S_IDLE);

    if (state != S_IDLE) {
    	SENSOR_TR("%s focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),state);
    	ret = -1;
    	goto sensor_af_init_end;
    }
#endif
//sensor_af_init_end:
    SENSOR_DG("%s %s ret:0x%x \n",SENSOR_NAME_STRING(),__FUNCTION__,ret);
    return ret;
}


static int sensor_focus_af_single_usr_cb(struct i2c_client *client) 
{

  
  
	int ret = 0;
	char cnt;
	u16 state;
	struct af_cmdinfo cmdinfo;
	//char s_zone[5],i;
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.validate_bit = 0x80;


  sensor_write(client,0xFCFC, 0xD000),
  sensor_write(client,0x0028, 0x7000),
  sensor_write(client,0x002A, 0x3076),
  sensor_write(client,0x0F12, 0x0400),
  sensor_write(client,0xFFFF, 0x000A),
  sensor_write(client,0x002A, 0x028C),
  sensor_write(client,0x0F12, 0x0006),
  
  //sensor_write(client,0xffff, 150),	// delay 150ms
	
	cnt = 0;
	state=0x0000;
	do
	{
		if (cnt != 0) {
			msleep(1);
		}
		cnt++;
        sensor_write(client,0x002C, 0x7000);
		sensor_write(client,0x002E, 0x2EEE);
		ret = sensor_read(client, 0x0f12, &state);

	}while((state != 0x0002) && (cnt<100));

	if (state != 0x0002) {
		SENSOR_TR("%s[%d] focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),__LINE__,state);
		ret = -1;
		goto sensor_af_single_end;
	} else {
		SENSOR_DG("%s[%d] single focus mode set success!\n",SENSOR_NAME_STRING(),__LINE__);    
	}
sensor_af_single_end:
	return ret;
}
static struct rk_sensor_reg sensor_af_macro_regs[] = {
{0xFCFC,0xD000},
{0x0028,0x7000},
{0x002A,0x028E}, // write [7000 028E, REG_TC_AF_AfCmdParam]
{0x0F12,0x00D0}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
{0x002A,0x028C},
{0x0F12,0x0004}, // write [7000 028C, REG_TC_AF_AfCmd] = 0004 , manual AF.

{0xffff,150},	// delay 150ms

{0x002A,0x1648},
{0x0F12,0x9002},
SensorEnd

};

static int sensor_focus_af_near_usr_cb(struct i2c_client *client)
{
	int ret;
	SENSOR_TR("sensor_s_macro_af\n");
	ret = generic_sensor_write_array(client,sensor_af_macro_regs);
	if(ret < 0)
	  SENSOR_TR("sensor_af_macro_regs error\n");

	return 0;
}
static struct rk_sensor_reg sensor_af_infinity_regs[] = {
	{0xFCFC,0xD000},
	{0x0028,0x7000},
	{0x002A,0x028E}, // write [7000 028E, REG_TC_AF_AfCmdParam]
	{0x0F12,0x0000}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
	{0x002A,0x028C},
	{0x0F12,0x0004}, // write [7000 028C, REG_TC_AF_AfCmd] = 0004 , manual AF.
	
	{0xffff,150},	// delay 150ms
	
	{0x002A,0x1648},
	{0x0F12,0x9002},
	SensorEnd
};

static int sensor_focus_af_far_usr_cb(struct i2c_client *client)
{
	int ret;
	
	SENSOR_TR("sensor_s_macro_af\n");
	ret = generic_sensor_write_array(client,sensor_af_infinity_regs);
	if(ret < 0)
	  SENSOR_TR("sensor_af_macro_regs error\n");
	 
	return 0;
}


static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos)
{
#if 0
	struct af_cmdinfo cmdinfo;
	sensor_af_idlechk(client);
	return 0;
	cmdinfo.cmd_tag = StepFocus_Spec_Tag;
	cmdinfo.cmd_para[0] = pos;
	cmdinfo.validate_bit = 0x81;
	return sensor_af_cmdset(client, StepMode_Cmd, &cmdinfo);
#endif
	return 0;
}

static struct rk_sensor_reg sensor_af_continues_regs[] = {
	{0xFCFC,0xD000},
	{0x0028,0x7000},
	{0x002A,0x3076}, // write [7000 028E, REG_TC_AF_AfCmdParam]
	{0x0F12,0x0400}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
	{0x002A,0x028C},
	{0x0F12,0x0006},
	SensorEnd
};

static int sensor_focus_af_const_usr_cb(struct i2c_client *client)
{
	int ret = 0;
#if 0
	struct af_cmdinfo cmdinfo;
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.cmd_para[0] = 0x00;
	cmdinfo.validate_bit = 0x81;
	sensor_af_idlechk(client);
	if (sensor_af_cmdset(client, ConstFocus_Cmd, &cmdinfo)) {
		SENSOR_TR("%s[%d] const focus mode set error!\n",SENSOR_NAME_STRING(),__LINE__);
		ret = -1;
		goto sensor_af_const_end;
	} else {
		SENSOR_DG("%s[%d] const focus mode set success!\n",SENSOR_NAME_STRING(),__LINE__);	  
	}
#endif

SENSOR_TR("sensor_focus_af_const_usr_cb\n");
ret = generic_sensor_write_array(client,sensor_af_continues_regs);
if(ret < 0)
  SENSOR_TR("sensor_focus_af_const_usr_cb error\n");

//sensor_af_const_end:
	return ret;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client)
{

	int ret = 0; 
#if 0

	sensor_af_idlechk(client);
	if (sensor_af_cmdset(client, PauseFocus_Cmd, NULL)) {
		SENSOR_TR("%s pause focus mode set error!\n",SENSOR_NAME_STRING());
		ret = -1;
		goto sensor_af_pause_end;
	}
#endif
//sensor_af_pause_end:
	return ret;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{

	int ret = 0;
	//struct af_cmdinfo cmdinfo;
	//int zone_tm_pos[4];
	int zone_center_pos[2];
	//struct generic_sensor*sensor = to_generic_sensor(client);    
	
	if (zone_tm_pos) {
		zone_tm_pos[0] += 1000;
		zone_tm_pos[1] += 1000;
		zone_tm_pos[2]+= 1000;
		zone_tm_pos[3] += 1000;
		zone_center_pos[0] = ((zone_tm_pos[0] + zone_tm_pos[2])>>1)*SENSOR_PREVIEW_W/2000;
		zone_center_pos[1] = ((zone_tm_pos[1] + zone_tm_pos[3])>>1)*SENSOR_PREVIEW_H/2000;
	} else {

		zone_center_pos[0] = 32;
		zone_center_pos[1] = 24;

	}
	
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x1=%d, y1=%d\n",zone_tm_pos[0],zone_tm_pos[1]);
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x2=%d, y2=%d\n",zone_tm_pos[2],zone_tm_pos[3]);
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x2=%d, y2=%d\n",zone_center_pos[0],zone_center_pos[1]);
	

//sensor_af_zone_end:
	return ret;
}

/*
face defect call back
*/
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	struct rk_sensor_sequence *sensor_series;
	int i,num_series;
	
	if(spsensor && spsensor->common_sensor.info_priv.sensor_series)
	{
		sensor_series = spsensor->common_sensor.info_priv.sensor_series;
		num_series = spsensor->common_sensor.info_priv.num_series;
		for(i=0; i<num_series; i++,sensor_series++)
		{
			if ((sensor_series->gSeq_info.w==2592) && (sensor_series->gSeq_info.h==1944))
			{
				sensor_series->gSeq_info.w = 2560;
				sensor_series->gSeq_info.h = 1920;
			}
		}
	}
	
	return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();

 


