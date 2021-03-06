/**
  ******************************************************************************
  * @file    SPI.h
  * @author  Waveshare Team
  * @version 
  * @date    13-October-2014
  * @brief   Header for SPI.c file
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, WAVESHARE SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _USE_SPI_H_
#define _USE_SPI_H_

/* Includes ------------------------------------------------------------------*/
#include "MacroAndConst.h"
//#include "stm32f10x_spi.h"
#include "SMM_MPS2.h" 


#define  SPI_CLK_period      1000  //us  
#define  SPI_CLK_DELAY()     Sleepus(SPI_CLK_period/2);//Sleepmus()   half of the CLK period

#define  LCD_SPI	MPS2_SSP3	


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern uint8_t spi_read_write_byte( uint8_t chByte);
//extern uint16_t spi_read_write_word( uint16_t hwWord);

#endif

/*-------------------------------END OF FILE-------------------------------*/

