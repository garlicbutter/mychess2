/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include "string.h"
#include "lvgl/lvgl.h"
#include "ui.h"
#include "queue.h"

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
CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;
osThreadId lvglTaskHandle;
/* USER CODE BEGIN PV */

TS_StateTypeDef TsState;
#define BYTES_PER_PIXEL (LV_COLOR_DEPTH / 8)
static uint8_t imgbuf1[ILI9341_LCD_PIXEL_WIDTH * ILI9341_LCD_PIXEL_HEIGHT / 10 * BYTES_PER_PIXEL];
QueueHandle_t print_queue;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartLvglTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void my_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    uint32_t flush_width = area->x2 - area->x1 + 1;
    uint32_t flush_height = area->y2 - area->y1 + 1;


    uint32_t screen_width = BSP_LCD_GetXSize();
    uint32_t fb_start_address = LtdcHandler.LayerCfg[0].FBStartAdress;
    uint8_t * dest_address = (uint8_t *)fb_start_address +
                             (((area->y1 * screen_width) + area->x1) * BYTES_PER_PIXEL);

    uint8_t * source_address = px_map;

    for(uint32_t y = 0; y < flush_height; y++)
    {
        memcpy(dest_address, source_address, (flush_width * BYTES_PER_PIXEL));
        source_address += (flush_width * BYTES_PER_PIXEL);
        dest_address += (screen_width * BYTES_PER_PIXEL);
    }

    // CRITICAL: You MUST tell LVGL that you have finished flushing!
    // If you forget this line, LVGL will draw one frame and freeze forever.
    lv_display_flush_ready(display);
}
void my_input_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    BSP_TS_GetState(&TsState);

    if (TsState.TouchDetected)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = TsState.X;
        data->point.y = TsState.Y;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
        /* Note: You do not need to update data->point.x or y here.
           LVGL is smart enough to remember the last known coordinates
           internally to handle "click and drag" release events. */
    }
}
void drag_event_cb(lv_event_t * e)
{
    /* Get the specific object that triggered the event (your panel) */
    lv_obj_t * obj = lv_event_get_target(e);

    /* Get the input device (your touchscreen) associated with this event */
    lv_indev_t * indev = lv_event_get_indev(e);
    if(indev == NULL) return;

    /* Get the "vector" (how many pixels the finger moved since the last frame) */
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    /* Get current coordinates and add the movement vector */
    lv_coord_t x = lv_obj_get_x(obj) + vect.x;
    lv_coord_t y = lv_obj_get_y(obj) + vect.y;

    /* Apply the new position */
    lv_obj_set_pos(obj, x, y);
}
#include "ui.h"

#include "ui.h"
#include <string.h> // Needed for strncpy

void update_debug_terminal(void) {
    if(print_queue == NULL || objects.debug_terminal == NULL) return;

    char buf[64];
    char c;
    int i = 0;

    /* Pull up to 63 characters per frame out of the RTOS queue */
    while(i < 63 && xQueueReceive(print_queue, &c, 0) == pdTRUE) {
        buf[i++] = c;
    }

    if(i > 0) {
        buf[i] = '\0';

        /* 1. Append the new characters */
        lv_textarea_add_text(objects.debug_terminal, buf);

        /* 2. Get the current text and count the lines */
        const char * text = lv_textarea_get_text(objects.debug_terminal);
        int newline_count = 0;
        const char * ptr = text;

        while(*ptr) {
            if(*ptr == '\n') {
                newline_count++;
            }
            ptr++;
        }

        /* 3. If we exceed 10 lines, chop off the oldest ones safely */
        if(newline_count > 10) {
            int lines_to_remove = newline_count - 10;
            ptr = text;

            /* Move pointer forward until we pass the old lines */
            while(lines_to_remove > 0 && *ptr) {
                if(*ptr == '\n') {
                    lines_to_remove--;
                }
                ptr++;
            }

            /* * THE FIX: Use a static buffer. It stays in global memory,
             * so it won't crash your FreeRTOS stack, and it completely
             * prevents LVGL from reading from memory it is trying to free.
             */
            static char safe_buffer[500];

            /* Copy the pruned text into our safe holding area */
            strncpy(safe_buffer, ptr, sizeof(safe_buffer) - 1);
            safe_buffer[sizeof(safe_buffer) - 1] = '\0'; // Guarantee null termination

            /* Now update LVGL safely */
            lv_textarea_set_text(objects.debug_terminal, safe_buffer);
        }
    }
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
  MX_CRC_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

//  LCD init code
	if (LCD_OK != BSP_LCD_Init()){
		Error_Handler();
	}
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
	BSP_LCD_SelectLayer(0);
	BSP_LCD_DisplayOn();
	BSP_LCD_Clear(LCD_COLOR_WHITE);

//	Touch Screen init code
	if (TS_OK != BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize())){
		Error_Handler();
	}

//	LVGL init
	lv_init();
	lv_tick_set_cb(HAL_GetTick);
//	LVGL display hook
	lv_display_t * display1 = lv_display_create(ILI9341_LCD_PIXEL_WIDTH, ILI9341_LCD_PIXEL_HEIGHT);
	lv_display_set_buffers(display1, imgbuf1, NULL, sizeof(imgbuf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display1, my_flush_cb);

//	LVGL touchscreen hook (need to call it after display init)
	lv_indev_t * indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);


//	ui (build from eez studio)
	ui_init();
	lv_obj_add_event_cb(objects.bishop_w_1, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.bishop_w_2, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.knight_w_1, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.knight_w_2, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.rook_w_1, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.rook_w_2, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.king_w, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.queen_w, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_1, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_2, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_3, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_4, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_5, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_6, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_7, drag_event_cb, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(objects.pawn_w_8, drag_event_cb, LV_EVENT_PRESSING, NULL);


  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	print_queue = xQueueCreate(512, sizeof(char));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityIdle, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of lvglTask */
  osThreadDef(lvglTask, StartLvglTask, osPriorityNormal, 0, 4096);
  lvglTaskHandle = osThreadCreate(osThread(lvglTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, RDX_Pin|WRX_DCX_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, LD3_Pin|LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : NCS_MEMS_SPI_Pin CSX_Pin OTG_FS_PSO_Pin */
  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin MEMS_INT1_Pin MEMS_INT2_Pin TP_INT1_Pin */
  GPIO_InitStruct.Pin = B1_Pin|MEMS_INT1_Pin|MEMS_INT2_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ACP_RST_Pin */
  GPIO_InitStruct.Pin = ACP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TE_Pin */
  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RDX_Pin WRX_DCX_Pin */
  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD4_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* Intercept printf and send characters to our RTOS queue */
int _write(int file, char *ptr, int len)
{
	//	VCP (Virtual Com Port is not avaible) for my DISCO board.
	//	because is comes with ST-LINK V2. so... printf doens't work via UART
//	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);

    if(print_queue != NULL) {
        for(int i = 0; i < len; i++) {
            /* Send char to queue. Wait max 5 ticks if the buffer is full. */
            xQueueSend(print_queue, &ptr[i], 5);
        }
    }
    return len;
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 5 */
	/* Infinite loop */
	for (;;) {
		osDelay(100);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartLvglTask */
/**
* @brief Function implementing the lvglTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLvglTask */
void StartLvglTask(void const * argument)
{
  /* USER CODE BEGIN StartLvglTask */
	printf("StartLvglTask1\n");
	printf("StartLvglTask2\n");
	printf("StartLvglTask3\n");
	printf("StartLvglTask4\n");
	printf("StartLvglTask5\n");
	printf("StartLvglTask6\n");
	printf("StartLvglTask7\n");
	printf("StartLvglTask8\n");
	printf("StartLvglTask9\n");
	printf("StartLvglTask10\n");
	printf("StartLvglTask11\n");
	printf("StartLvglTask12\n");
	printf("StartLvglTask13\n");
	printf("StartLvglTask14\n");
	printf("StartLvglTask15\n");
  /* Infinite loop */
	for (;;) {
		update_debug_terminal();

	  uint32_t time_till_next = lv_timer_handler();

	  /*If there is nothing to do now, check again a little bit later.*/
	  if(time_till_next == LV_NO_TIMER_READY) {
		 time_till_next = LV_DEF_REFR_PERIOD; /*33 ms by default in lv_conf.h*/
	  }
	  osDelay(time_till_next); /*Sleep the thread*/
	}
  /* USER CODE END StartLvglTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
