/**
  ******************************************************************************
  * File Name          : main.hpp
  * Description        : This file contains the common defines of the application
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H
  /* Includes ------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define led13_Pin GPIO_PIN_13
#define led13_GPIO_Port GPIOC
#define button_Pin GPIO_PIN_0
#define button_GPIO_Port GPIOA
#define ch_pd_Pin GPIO_PIN_1
#define ch_pd_GPIO_Port GPIOA
#define buzzer_Pin GPIO_PIN_5
#define buzzer_GPIO_Port GPIOA
#define buttonA6_Pin GPIO_PIN_6
#define buttonA6_GPIO_Port GPIOA
#define buttonA7_Pin GPIO_PIN_7
#define buttonA7_GPIO_Port GPIOA
#define buttonB0_Pin GPIO_PIN_0
#define buttonB0_GPIO_Port GPIOB
#define Boot_output_Pin GPIO_PIN_12
#define Boot_output_GPIO_Port GPIOB
#define Ring_input_Pin GPIO_PIN_13
#define Ring_input_GPIO_Port GPIOB
#define segments_Pin GPIO_PIN_14
#define segments_GPIO_Port GPIOB
#define segmentsB15_Pin GPIO_PIN_15
#define segmentsB15_GPIO_Port GPIOB
#define segmentsA8_Pin GPIO_PIN_8
#define segmentsA8_GPIO_Port GPIOA
#define segmentsA9_Pin GPIO_PIN_9
#define segmentsA9_GPIO_Port GPIOA
#define segmentsA10_Pin GPIO_PIN_10
#define segmentsA10_GPIO_Port GPIOA
#define segmentsA15_Pin GPIO_PIN_15
#define segmentsA15_GPIO_Port GPIOA
#define segmentsB3_Pin GPIO_PIN_3
#define segmentsB3_GPIO_Port GPIOB
#define segmentsB4_Pin GPIO_PIN_4
#define segmentsB4_GPIO_Port GPIOB
#define segmentsB5_Pin GPIO_PIN_5
#define segmentsB5_GPIO_Port GPIOB
#define door_Pin GPIO_PIN_6
#define door_GPIO_Port GPIOB
#define seg_in_Pin GPIO_PIN_7
#define seg_in_GPIO_Port GPIOB
#define seg_inB8_Pin GPIO_PIN_8
#define seg_inB8_GPIO_Port GPIOB
#define seg_inB9_Pin GPIO_PIN_9
#define seg_inB9_GPIO_Port GPIOB

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
