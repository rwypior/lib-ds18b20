/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "ds18b20.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

volatile float temperature;
volatile DS18B20_Bool anyParasitic;

// DS functions

void dsSetPinDir(const OneWire *ow, OneWire_PinDirection dir)
{
	uint32_t pin;
	GPIO_TypeDef *port;

	switch(ow->id)
	{
	case 1:
		pin = sensor_Pin;
		port = sensor_GPIO_Port;
		break;

	default: return;
	}

	GPIO_InitTypeDef initstruct = dir == OneWire_PinDir_Output ?
	(GPIO_InitTypeDef){ .Pin = pin,	.Mode = GPIO_MODE_OUTPUT_OD,	.Speed = GPIO_SPEED_FREQ_MEDIUM,	.Pull = GPIO_NOPULL } :		// Output
	(GPIO_InitTypeDef){ .Pin = pin,	.Mode = GPIO_MODE_INPUT,		.Speed = GPIO_SPEED_FREQ_MEDIUM,	.Pull = GPIO_NOPULL };		// Input

	HAL_GPIO_Init(port, &initstruct);
}

void dsSetPinState(const OneWire *ow, OneWire_PinState state)
{
	uint32_t pin;
	GPIO_TypeDef *port;

	switch(ow->id)
	{
	case 1:
		pin = sensor_Pin;
		port = sensor_GPIO_Port;
		break;

	default: return;
	}

	HAL_GPIO_WritePin(port, pin, state == OneWire_PinState_High ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

OneWire_PinState dsGetPinState(const OneWire *ow)
{
	uint32_t pin;
	GPIO_TypeDef *port;

	switch(ow->id)
	{
	case 1:
		pin = sensor_Pin;
		port = sensor_GPIO_Port;
		break;

	default: return OneWire_PinState_Low;
	}

	return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET ? OneWire_PinState_High : OneWire_PinState_Low;
}

void dsStartTimer(const OneWire *ow)
{
	TIM_HandleTypeDef *tim;

	switch(ow->id)
	{
	case 1: tim = &htim4; break;

	default: return;
	}

	__HAL_TIM_SET_COUNTER(tim, 0);
}

OneWire_Counter dsGetTimer(const OneWire *ow)
{
	TIM_HandleTypeDef *tim;

	switch(ow->id)
	{
	case 1: tim = &htim4; break;

	default: return 0;
	}

	return tim->Instance->CNT;
}

void detection(const OneWire *ow)
{
	asm volatile("nop");
}

void dsFinished(DS18B20 *ds, DS18b20State operation, DS18B20_Address addr, DS18B20CallbackFlags flags)
{
	if (operation == DS18b20_State_Convert)
		ds18b20ReadScratchpad(ds, addr);
	else if (operation == DS18b20_State_ReadScratchpad)
	{
		if (ds18b20VerifyCrc(ds))
			temperature = ds18b20GetTemperatureFloat(ds);
	}
	else if (operation == DS18b20_State_ReadPowersupply)
	{
		if (ds18b20VerifyCrc(ds))
			anyParasitic = flags == DS18B20_Callback_Parasitic;
	}
}

void onewireSearchCallback(const OneWire *ow)
{
	volatile OneWire_Address addr = ow->searchedAddress;
	(void)addr;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  // Initialize OneWire and DS18B20

      OneWire onewire = {0};
      DS18B20 ds = {0};

      onewireInit(&onewire, 1, &dsSetPinDir, &dsSetPinState, &dsGetPinState, &dsStartTimer, &dsGetTimer);
      ds18b20Init(&ds, &onewire);

      ds.readMode = DS18b20_Read_CRC;

      // Specify search finished callback

      onewire.onSearchDone = &onewireSearchCallback;

      // Optional - specify callback for presence detection

      onewire.detectedCallback = &detection;

      // Set operation finished callback

      ds.onOperationFinished = &dsFinished;

      // Start 1us timer

      HAL_TIM_Base_Start(&htim4);

      // Keep track of result for debug purposes

      DS18B20Result result;
      (void)result;


      // Search

      onewireSearch(&onewire, OneWire_False);
      onewireWait(&onewire);

      // Read ROM

      ds18b20RequestReadRom(&ds);
      ds18b20Wait(&ds);

      DS18B20_Address rom;
      memcpy((DS18B20_Byte*)&rom, ds.buffer, sizeof(rom));


      // Check for parasitic powered devices

      ds18b20ReadPowerSupply(&ds);
      ds18b20Wait(&ds);


      // Check if genuine

      DS18B20_Bool genuine = ds18b20CheckAuthentic(rom);
      (void)genuine;


      // Load scratchpad from eeprom

      ds18b20RecallEeprom(&ds, DS18B20_ROM_NONE);
      ds18b20Wait(&ds);


      // Read scratchpad

      result = ds18b20ReadScratchpad(&ds, rom);
      ds18b20Wait(&ds);

      asm volatile("nop");


      // Set resolution

      result = ds18b20SetResolution(&ds, DS18B20_Resolution_11, (DS18B20_Byte[2]){42, 13}, DS18B20_ROM_NONE);
      ds18b20Wait(&ds);


      // Save scratchpad to eeprom

      result = ds18b20CopyScratchpad(&ds, rom);
      ds18b20Wait(&ds);

      asm volatile("nop");


      // Clear buffer - just for debug

      memset(ds.buffer, 0, sizeof(ds.buffer));


      // Read scratchpad

      result = ds18b20ReadScratchpad(&ds, DS18B20_ROM_NONE);
      ds18b20Wait(&ds);

      asm volatile("nop");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  DS18b20State state = ds18b20Process(&ds);

	  	if (state == DS18b20_State_Finished || state == DS18b20_State_Idle)
	  	{
	  		volatile DS18B20Result res = ds18b20BeginConversion(&ds, DS18B20_ROM_NONE);
	  		(void)res;
	  	}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
