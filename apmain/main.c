/*
 * Copyright:
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2014-2017 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     main.c
 * Release:  Version 2.0
 * ----------------------------------------------------------------
 *
 */

/*
 * --------Included Headers--------
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "SMM_MPS2.h"                   // MPS2 common header
#include "GLCD_SPI_MPS2.h"

#include "uart_stdout.h"

#include "../apaaci/apaaci.h"
#include "../apleds/apleds.h"
#include "../apssp/apssp.h"
#include "../apclcd/apclcd.h"
#include "../aptsc/aptsc.h"
#include "../apmem/apmem.h"
#include "../aptimer/aptimer.h"
#include "../aplan/aplan.h"
//#include "../apsonar/apsonar.h"
//#include "../apuart/apuart.h"


#include "gpio_m3.h"
//#include "LCD.h"
#include "LIB_Config.h"

#include "ov5640.h"
#include "LCD.h"

 	
// Local variables


//  state machine 
char current_state = 0;
char next_state = 0;

// Return status

void PORT0_7_Handler(void);
typedef enum sts
{
    TEST_S_UNKNOWN,
    TEST_S_NOTRUN,
    TEST_S_SUCCESS,
    TEST_S_FAILURE,
    MAX_STATUS
} Status_t;

char    *status_str[MAX_STATUS];

typedef struct test
{
    apError     (*test)(void);
    int         auto_run;
    char        *name;
}Test_t;

// Table of available tests
static Test_t tests[] =
{
    {&apAACI_TEST,          0,  "AACI (Audio)           "},
    {&apCLCD_TEST,          0,  "CLCD (Video)           "},
    {&apTSC_TEST,           0,  "TSC (touchscreen)      "},
    {&apLEDS_TEST,          0,  "LEDs/Switches/Buttons  "},
    {&apSSP_TEST,           0,  "SSP (eeprom)           "},
//    {&apSSP_TEST_S,         0,  "SSP (slave)            "},
//    {&apSSP_TEST_M,         0,  "SSP (master)           "},
    {&apLANI_TEST,          0,  "Ethernet               "},
    {&apMEM_TEST,           0,  "Memory                 "},
    {&apTIMER_TEST,         0,  "Timer                  "},
//    {&apSONAR_TEST,         0,  "Sonar Sensor Test      "},
//    {&apMICROPHONE_TEST,    0,  "Microphone Test        "},
//    {&apRHT03_TEST,         0,  "RHT03 Test             "},
//    {&apBT4_UART_TEST,      0,  "BT4 UART Test          "},
};

static Test_t prod_tests[] =
{
    {&apAACI_TEST,          0,  "AACI (Audio)           "},
    {&apCLCD_TEST,          0,  "CLCD (Video)           "},
    {&apTSC_TEST,           0,  "TSC (touchscreen)      "},
    {&apLEDS_TEST,          0,  "LEDs/Switches/Buttons  "},
    {&apSSP_TEST,           0,  "SSP (eeprom)           "},
    {&apLANI_TEST,          0,  "Ethernet               "},
    {&apMEM_TEST,           0,  "Memory                 "},
    {&apTIMER_TEST,         0,  "Timer                  "},
};

#define NO_OF_PROD_TESTS (sizeof(prod_tests) / sizeof(prod_tests[0]))
#define NO_OF_TESTS (sizeof(tests) / sizeof(tests[0]))

Status_t test_status[NO_OF_TESTS];
Status_t prod_test_status[NO_OF_PROD_TESTS];


// Initialisation routines
void Test_Init (void)
{
    int i;

    // Init test status and status strings.
    for (i = 0; i < MAX_STATUS; i++)
        status_str[i]  = "Unknown";

    status_str[TEST_S_NOTRUN]  = "Not Run";
    status_str[TEST_S_SUCCESS] = "PASS";
    status_str[TEST_S_FAILURE] = "FAIL";

    for (i = 0; i < NO_OF_TESTS; i++)
        test_status[i] = TEST_S_NOTRUN;
}

// User interface
char *Get_Status (Status_t status)
{
    if (status >= MAX_STATUS)
        status = TEST_S_UNKNOWN;

    return status_str[status];
}

void Print_Results (void)
{
    int i;
    int counts[MAX_STATUS];

    for (i = 0; i < MAX_STATUS; i++)
        counts[i] = 0;

        printf ("Summary of results\n");
        printf ("====================================\n");

    for (i = 0; i < NO_OF_TESTS; i++) {
        printf ("%2d %-20s : %s\n",(i+1), tests[i].name, Get_Status (test_status[i]));

        if (test_status[i] < MAX_STATUS)
            counts[test_status[i]]++;
    }
}

void Print_Production_Results(void)
{
    int i;
    int counts[MAX_STATUS];

    for (i = 0; i < MAX_STATUS; i++)
        counts[i] = 0;

        printf ("Summary of results\n");
        printf ("====================================\n");

    for (i = 0; i < NO_OF_PROD_TESTS; i++) {
        printf ("%2d %-20s : %s\n",(i+1), (prod_tests[i]).name,
                Get_Status (prod_test_status[i]));

        if (prod_test_status[i] < MAX_STATUS)
            counts[prod_test_status[i]]++;
    }

    printf("%2d Repeat Production Tests:\n",NO_OF_PROD_TESTS + 1);
    printf("%2d Back to interactive Selftest:\n",NO_OF_PROD_TESTS + 2);
}

#ifdef SEMIHOST
#define LOG_LINE_MAX                       100
char logfilepath[LOG_LINE_MAX];
FILE *logfile;
#endif

void Run_Production_Test(int test)
{
    apError status;

    test--;
    printf ("\nTesting %s\n", (prod_tests[test]).name);

    status = (*prod_tests[test].test)();

    if (status == apERR_NONE)
        prod_test_status[test] = TEST_S_SUCCESS;
    else
        prod_test_status[test] = TEST_S_FAILURE;

    printf_err("%s    : test result : %s\n\n", (prod_tests[test]).name, Get_Status (prod_test_status[test]));
}

void Run_Test (int test)
{
    apError status;

    test--;
    printf ("\nTesting %s\n", tests[test].name);

    status = (*tests[test].test)();

    if (status == apERR_NONE)
        test_status[test] = TEST_S_SUCCESS;
    else
        test_status[test] = TEST_S_FAILURE;

    printf ("%s    : test result : %s\n\n", tests[test].name, Get_Status (test_status[test]));
}


void Run_All_Tests (void)
{
    int i;
#ifdef SEMIHOST
    char *logbasedir = "c:\\arm\\dspl\\logs\\";
    char *logfiledir = "c:\\arm\\dspl\\logs\\mpb\\";
    char *board_info_path = "c:\\arm\\dspl\\Temp\\functest.txt";
    char *syscommand = "IF NOT EXIST c:\\arm\\dspl\\logs\\mpb "
                       "MKDIR c:\\arm\\dspl\\logs\\mpb\n";
    char serial[LOG_LINE_MAX];
    char date[LOG_LINE_MAX];
    char epoch[LOG_LINE_MAX];
    time_t now;
    int epoch_i;
    FILE *board_info;

    memset(serial, 0, LOG_LINE_MAX);
    memset(date, 0, LOG_LINE_MAX);
    memset(epoch, 0, LOG_LINE_MAX);

    if ((board_info = fopen(board_info_path,"r")) == 0)
        printf("Opening %s failed.\n", board_info_path);
    else
    {
        // Read and check validity of test information file on host
        fgets(epoch, LOG_LINE_MAX, board_info);
        fgets(date, LOG_LINE_MAX, board_info);
        fgets(serial, LOG_LINE_MAX, board_info);
    }

    printf("Epoch timestamp: %s\n", epoch);
    printf("Date: %s\n", date);
    printf("Serial no: %s\n", serial);

    time(&now);
    // Convert epoch to integer
    epoch_i = atoi(epoch);

    printf("Creating directory with command: %s\n", syscommand);
    system(syscommand);

    // Timestamp not recent, writing to default file.
    if ((int)now - 300 > epoch_i) {
        memcpy(&serial[0], "0000B00000", 10);
        strcpy(&serial[10], ".txt");
        strcpy(logfilepath, logbasedir);
        strcpy(&logfilepath[strlen(logbasedir)], serial);
    } else {
        strcpy(&serial[strlen(serial)-1], ".txt");
        strcpy(logfilepath, logfiledir);
        strcpy(&logfilepath[strlen(logfiledir)], serial);
    }
    printf("Using log file:%s\n", logfilepath);
    if ((int)(logfile = fopen(logfilepath, "a")) == 0)
        printf("Unable to open %s\n", logfilepath);
    else {
        printf("Opened %s\n", logfilepath);
        fprintf(logfile, "\n-- %s", ctime(&now));
        fclose(logfile);
    }
#endif
    // Make sure all of the test harness is plugged in
    printf("\nStarting Production testing...\n");
    printf("Please plug in the Ethernet, SSP, VGA,\n");
    printf("and Audio cables, \n");
    Wait_For_Enter(TRUE);

    for (i = 0; i < NO_OF_PROD_TESTS; i++) {
        Run_Production_Test(i + 1);
    }
    return;
}

//int Select_Test (void)
//{
//    unsigned int choice;
//    char         buffer[8];

//    if(RunAllTests == TRUE) {
//        printf("\nSelect the test you wish to run. (X - Exit)\n");
//        do {
//            choice = 0;
//            printf ("\nChoice: ");
//            GetLine (buffer, sizeof(buffer));

//            if(toupper(buffer[0]) == 'X')
//                return 0;

//            sscanf (buffer, "%d", &choice);
//        }
//        while (!choice  || (choice > NO_OF_PROD_TESTS + 2));

//    }
//    else {
//        printf("\nSelect the test you wish to run. (X - Exit)\n");
//        do {
//            choice = 0;
//            printf ("\nChoice: ");
//           	GetLine (buffer, sizeof(buffer));

//            if (toupper(buffer[0]) == 'X')
//                return 0;

//            sscanf (buffer, "%d", &choice);
//        }
//        while (!choice  || (choice > (NO_OF_TESTS + 1)));


//    }
//    return choice;
//}

//// Display Colour bars on the VGA screen
//static void VGA_Test (void)
//{
//    unsigned int x, y, col, pxl, fpxl;

//    // Display Colour bars on the VGA screen
//    for (y = 0; y < 128; y++)
//    {
//      for (x = 0; x < (128 * 3); x++)
//      {
//    	  // Colour 0x0000RGB (4 bit colour)
//    	  if (x < 128)
//    		  col = ((x >> 3) & 0xF);
//    	  else if (x < 256)
//    		  col = ((x >> 3) & 0xF) << 4;
//    	  else
//    		  col = ((x >> 3) & 0xF) << 8;

//    	  // Pixel position
//    	  pxl = (x * 4) + (y * 512 * 4);

//    	  // Write to screen buffer
//    	  *((unsigned int *)(MPS2_VGA_BUFFER + pxl)) = col;
//      }
//    }

//    // Display test chart face on the VGA screen
//    for (y = 0; y < 128; y++)
//    {
//      for (x = 384; x < 512; x++)
//      {
//    	  // Flyer pixel position
//    	  fpxl = (x - 224) + ((y + 109) * 320);

//    	  // Colour 0x0000RGB (4 bit colour)
//    	  col  = (flyerData[fpxl] >> 1) & 0x000F;
//    	  col |= (flyerData[fpxl] >> 3) & 0x00F0;
//    	  col |= (flyerData[fpxl] >> 4) & 0x0F00;

//    	  // Screen pixel position
//    	  pxl = (x * 4) + (y * 512 * 4);

//    	  // Write to screen buffer
//    	  *((unsigned int *)(MPS2_VGA_BUFFER + pxl)) = col;
//      }
//    }
//}

int main (void)
{
//    int choice;
//    unsigned int id, fpgaid, apnote, rev, aid;
//    unsigned int count = 0;
			U8 xpos = 0;
			U16 ypos = 0;
	
			uint16_t reg;
			char * char_buff;
			
			Sleepms(500);

    // Enable AHB and APB clock
    CMSDK_SYSCON->AHBCLKCFG0SET = 0xF;  // GPIO
    CMSDK_SYSCON->APBCLKCFG0SET = 0x37;	// UART0, UART1, TIMER0, TIMER1, DUAL TIMER

//  while (1)
//  {
//      count += 1;
//      CMSDK_GPIO0->DATAOUT = count;
//      CMSDK_GPIO0->OUTENABLESET = (0xff);
//  }


    // UART init
    UartStdOutInit();

//    Test_Init();

    NVIC_ClearAllPendingIRQ();
		
//		gpio_m3_out(OV5640_INIT_DONE_GPIO, OV5640_INIT_DONE_PIN, 0);//OV5640  init not done

    // Display the program version
    printf ("\n\n\nCortex M3 DesignStart Eval (V2M-MPS2+) Test Suite\n");
    printf ("Version 1.0.0" " Build date: " __DATE__ "\n");
    printf ("Copyright (C) ARM Ltd 2015-2017. All rights reserved.\n\n");
		
		//altfun of GPIO 11-14
		CMSDK_GPIO0->ALTFUNCCLR = 0xFFFF;//0111 1000 0000 0000
		CMSDK_GPIO0->ALTFUNCSET = 0x6800;//0110 1000 0000 0000
//		printf("%x",CMSDK_gpio_GetAltFunc(CMSDK_GPIO0));
//		
//		gpio_m3_out(CMSDK_GPIO0,1,0);
////////comment here 
//    // Display the board revision
//    rev = MPS2_SCC->CFG_REG4;
//    printf("V2M-MPS2 revison %c\n\n", rev + 'A');

//    fpgaid = MPS2_SCC->SCC_ID;
//    aid = MPS2_SCC->SCC_AID;

//    if (rev != EXTRACT_BITS(aid, 23, 20))
//        printf("WARNING: Board revision does not match FPGA target revision\n\n");

//    apnote = EXTRACT_BITS(fpgaid, 15, 4);

//    printf("Application Note AN%x, FPGA build %d\n", apnote, EXTRACT_BITS(aid, 31, 24));

//    // Display CPU ID
//    id = SCB->CPUID;
//	if(EXTRACT_BITS(id, 15, 8) == 0xC6)
//		printf ("\nCPU: Cortex-M%d+ r%dp%d\n\n",EXTRACT_BITS(id, 7, 4),EXTRACT_BITS(id, 23, 20),EXTRACT_BITS(id, 3, 0));
//	else
//		printf ("\nCPU: Cortex-M%d r%dp%d\n\n",EXTRACT_BITS(id, 7, 4),EXTRACT_BITS(id, 23, 20),EXTRACT_BITS(id, 3, 0));

////    // Display Colour bars on the VGA screen
//    VGA_Test();

    // CLCD screen register setup
//    GLCD_Initialize();

//    // Fill CLCD screen
//    GLCD_Clear(Navy);

    // Draw intro screen image
    //GLCD_Bitmap (22, 71, 276, 99, (unsigned short *)introData);
//	while(1)
//	{
//    GLCD_Bitmap (0, 0, 320, 240, (unsigned short *)introData);
//	}
//    // Start Test
//    RunAllTests = FALSE;
//    while(1)
//    {
//        if(RunAllTests) {
//            Print_Production_Results();
//        } else {
//            Print_Results();
//        }

//        choice = Select_Test();

//        if (choice == 0)
//            break;

//        if(!RunAllTests) {
//            if (choice == NO_OF_TESTS + 1) {
//                RunAllTests = TRUE;
//                Run_All_Tests();
//            }
//            else {
//                Run_Test(choice);
//            }
//        }
//        else
//        {  // Production tests
//            if(choice == NO_OF_PROD_TESTS + 2)
//            {
//                RunAllTests = FALSE;    // Set Interactive mode
//                continue;               // Skip running production and next
//                                        // Loop will start in interactive mode.
//            }
//            else if(choice == NO_OF_PROD_TESTS + 1)
//            {
//                Run_All_Tests();
//            }
//            else
//            {
//                Run_Production_Test(choice);
//            }
//        }
//    }

//		  MPS2_SSP3->CR1   =   0;                 /* Synchronous serial port disable  */
//			MPS2_SSP3->DMACR =   0;                 /* Disable FIFO DMA                 */
//			MPS2_SSP3->IMSC  =   0;                 /* Mask all FIFO/IRQ interrupts     */
//			MPS2_SSP3->ICR   = ((1ul <<  0) |       /* Clear SSPRORINTR interrupt       */
//													(1ul <<  1) );      /* Clear SSPRTINTR interrupt        */
//			MPS2_SSP3->CR0   = ((7ul <<  0) |       /* 8 bit data size                  */
//													(0ul <<  4) |       /* Motorola frame format            */
//													(1ul <<  6) |       /* CPOL = 0                         */
//													(1ul <<  7) |       /* CPHA = 0                         */
//													(1ul <<  8) );      /* Set serial clock rate            */
//			MPS2_SSP3->CPSR  =  (2ul <<  0);        /* set SSP clk to 6MHz (6.6MHz max) */
//			MPS2_SSP3->CR1   = ((1ul <<  1) |       /* Synchronous serial port enable   */
//													(0ul <<  2) );      /* Device configured as master      */

			lcd_init();
		

		//set interupt of GPIO7
		CMSDK_GPIO0->INTENCLR=0xFFFF;
		CMSDK_gpio_ClrOutEnable(CMSDK_GPIO0,7);

////		CMSDK_gpio_SetIntFallingEdge(CMSDK_GPIO0,7);
//		CMSDK_gpio_SetIntHighLevel(CMSDK_GPIO0,7);
//		
		CMSDK_GPIO0->INTENSET = (1 << 7);
//		CMSDK_GPIO0->INTENCLR = (1 << 7);
		
		CMSDK_gpio_SetIntFallingEdge(CMSDK_GPIO0,7);
		

		gpio_m3_out(OV5640_INIT_DONE_GPIO, OV5640_INIT_DONE_PIN, 0);
		
		NVIC_EnableIRQ(PORT01_7_IRQn);

//		while(OV5640_Init())
//		{
//						Sleepms(500);
////						printf("OV5640 Initialization error, please check !\r\n");
//						
//		}      

		reg = OV5640_RD_Reg(0x4300);
		sprintf(char_buff,"0x4300:%x",reg);
		lcd_display_string(10,100,(const uint8_t *)char_buff,FONT_1608,WHITE); 
		
		

		
		
//		OV5640_JPEG_Mode();		//JPEG
		
		
		 
//		OV5640_RGB565_Mode();
//		OV5640_Focus_Init(); 

//		
//		
//					
//		OV5640_Light_Mode(0);	   //set auto
//		OV5640_Color_Saturation(3); //default
//		OV5640_Brightness(4);	//default
//		OV5640_Contrast(3);     //default
//		OV5640_Sharpness(33);	//set auto
//		OV5640_Auto_Focus();
		

//		

		gpio_m3_out(OV5640_INIT_DONE_GPIO, OV5640_INIT_DONE_PIN, 0);
		

		
//		
		rgb565_test();
		
		Sleepms(50);
		reg = OV5640_RD_Reg(0x4300);
		sprintf(char_buff,"0x4300:%x",reg);
		lcd_display_string(10,120,(const uint8_t *)char_buff,FONT_1608,WHITE);
//		OV5640 init done 
//while(1){

		
//		Sleepms(50);
//	lcd_clear_screen(BLUE);
//}
		
		OV5640_INIT_DONE

		
		lcd_clear_screen(GREEN);
		gpio_m3_out(OV5640_READY_GPIO,OV5640_READY_PIN,1);
	while (1) {
		
		gpio_m3_out(OV5640_READY_GPIO,OV5640_READY_PIN,1);
		
		
		if (gpio_m3_in(OV5640_W_DONE_GPIO,OV5640_W_DONE_PIN)==1)
		{
			gpio_m3_out(OV5640_READY_GPIO,OV5640_READY_PIN,0);		
				
//			switch(current_state){
//				case 0x00:
//										
//										break;
//				case 0x01:
//										break;
//				case 0x02:
//										break;
//				case 0x03:
//										break;
//				case 0x04:
//										break;
//			
//			}
			

			
			gpio_m3_out(OV5640_W_EN_GPIO,OV5640_W_EN_PIN,1);
			
			//draw screen without set cusor
			lcd_set_cursor(0, 0);
			lcd_write_byte(0x22, LCD_CMD);
			__LCD_DC_SET();
			__LCD_CS_CLR();
			
			for(ypos = 0;ypos <320;ypos++)
			{
				for(xpos = 0;xpos < 240;xpos++)
				{
						
						if(readbit_RAM(((2*xpos*640)+2*ypos)&0x7FFFF)==0){
//								lcd_draw_point(xpos,ypos,BLACK);
									__LCD_WRITE_BYTE(0x00);
									__LCD_WRITE_BYTE(0x00);
						}
						else {
//								lcd_draw_point(xpos,ypos,WHITE);
								__LCD_WRITE_BYTE(0xFF);
								__LCD_WRITE_BYTE(0xFF);
						}
						      //disable  write enable
						
				}
			}
			__LCD_CS_SET();
			
			
			//end of  draw screen without set cusor
			
			
			gpio_m3_out(OV5640_READY_GPIO,OV5640_READY_PIN,1);
		
//			break;
			gpio_m3_out(OV5640_W_EN_GPIO,OV5640_W_EN_PIN,0); 
		}
		
		Sleepms(200);
	}

	
    
//while(1)
//{
//		gpio_m3_out(CMSDK_GPIO0,(U8)1,(U8)1);
//		Sleepms(1000);
//		gpio_m3_out(CMSDK_GPIO0,(U8)1,(U8)0);
//		gpio_m3_out(CMSDK_GPIO0,(U8)2,(U8)1);
//		Sleepms(1000);
//		gpio_m3_out(CMSDK_GPIO0,(U8)2,(U8)0);
//		gpio_m3_out(CMSDK_GPIO0,(U8)3,(U8)1);
//		Sleepms(1000);
//		gpio_m3_out(CMSDK_GPIO0,(U8)3,(U8)0);
//		gpio_m3_out(CMSDK_GPIO0,(U8)1,(U8)1);
//		Sleepms(1000);
//}

//    return TRUE;
}

void PORT0_7_Handler(void){
//	lcd_clear_screen(BLUE);
//	printf("interupt !\n");
	CMSDK_gpio_IntClear(CMSDK_GPIO0,7);
	}
