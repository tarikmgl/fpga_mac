/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f1xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CLK_PORT    GPIOA
#define CLK_PIN     GPIO_PIN_0

#define CMD1_PORT   GPIOA
#define CMD1_PIN    GPIO_PIN_1

#define CMD0_PORT   GPIOA
#define CMD0_PIN    GPIO_PIN_2

#define DIN_PORT    GPIOA
#define DIN_PIN     GPIO_PIN_3

#define DOUT_PORT   GPIOA
#define DOUT_PIN    GPIO_PIN_4

#define CMD_BUF_SIZE 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}

void MAC_Clock(void)
{
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    delay_us(200);
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    delay_us(200);
}

void MAC_SetCommand(uint8_t command)
{
    HAL_GPIO_WritePin(
        CMD1_PORT, CMD1_PIN,
        (command & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        CMD0_PORT, CMD0_PIN,
        (command & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
}

void MAC_Reset(void)
{
    MAC_SetCommand(0);
    MAC_Clock();
}

void MAC_Write(uint16_t a, uint16_t b)
{
    uint32_t value;
    int i;

    value = ((uint32_t)b << 16) | a;

    MAC_SetCommand(1);

    for (i = 0; i < 32; i++)
    {
        HAL_GPIO_WritePin(
            DIN_PORT, DIN_PIN,
            (value & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET
        );

        MAC_Clock();

        value >>= 1;
    }
}

void MAC_Sum(void)
{
    MAC_SetCommand(2);
    MAC_Clock();
}

uint32_t MAC_Read(void)
{
    uint32_t value = 0;
    int i;

    MAC_SetCommand(3);

    for (i = 0; i < 32; i++)
    {
        value <<= 1;

        if (HAL_GPIO_ReadPin(DOUT_PORT, DOUT_PIN) == GPIO_PIN_SET)
        {
            value |= 1;
        }

        MAC_Clock();
    }

    return value;
}

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

static uint8_t UART_ReadLine(char *buf, uint8_t max_len)
{
    uint8_t idx = 0;
    uint8_t ch;

    while (idx < max_len - 1)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1, HAL_MAX_DELAY) != HAL_OK)
        {
            continue;
        }

        if (ch == '\r' || ch == '\n')
        {
            if (idx == 0)
            {
                continue;
            }
            break;
        }

        HAL_UART_Transmit(&huart1, &ch, 1, HAL_MAX_DELAY);
        buf[idx++] = (char)ch;
    }

    buf[idx] = '\0';
    UART_SendString("\r\n");

    return idx;
}

void MAC_ProcessCommand(void)
{
    char line[CMD_BUF_SIZE];

    UART_SendString("mac> ");
    UART_ReadLine(line, CMD_BUF_SIZE);

    if (strncmp(line, "write", 5) == 0)
    {
        uint32_t a, b;

        if (sscanf(line, "write %lu %lu", &a, &b) == 2)
        {
            MAC_Write((uint16_t)a, (uint16_t)b);

            char msg[64];
            snprintf(msg, sizeof(msg), "OK: A=%lu B=%lu written\r\n", a, b);
            UART_SendString(msg);
        }
        else
        {
            UART_SendString("ERR: kullanim -> write <a> <b>\r\n");
        }
    }
    else if (strncmp(line, "sum", 3) == 0)
    {
        MAC_Sum();
        UART_SendString("OK: sum yapildi\r\n");
    }
    else if (strncmp(line, "reset", 5) == 0)
    {
        MAC_Reset();
        UART_SendString("OK: reset yapildi\r\n");
    }
    else if (strncmp(line, "read", 4) == 0)
    {
        uint32_t result = MAC_Read();

        char msg[64];
        snprintf(msg, sizeof(msg), "C = %lu\r\n", result);
        UART_SendString(msg);
    }
    else
    {
        UART_SendString("ERR: bilinmeyen komut (write/sum/reset/read)\r\n");
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
  //MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  DWT_Delay_Init();
  MAC_Reset();
  MAC_Write(10, 20);
  MAC_Sum();
  uint32_t result = MAC_Read();
  //UART_SendString("MAC Terminal hazir. Komutlar: write <a> <b>, sum, reset, read\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    //MAC_ProcessCommand();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
  __disable_irq();
  while (1)
  {
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

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
