
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_accelero.h"
#include <stdio.h>


/*dichiaro la variabile che sarà l 'oggetto che userò nel mio programma per
  riferirmi a USART!*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
int16_t acc_raw[3];      // accelerometro X,Y,Z
char buf[128];           // buffer per UART
static uint8_t header_sent = 0;   // <--- AGGIUNGI QUESTA
static uint8_t vib_alarm = 0;   // <--- LATCH: 0=off, 1=attivo
/* USER CODE BEGIN PD */


// MOD: soglie esplicite
#define THR_CUTOFF_CENTI 3200   // 32.00 °C
#define THR_COLD_MG      120    // soglia a T <= 32°C
#define THR_HOT_MG       500   // soglia a T  > 32°C


#define BTN_PORT        GPIOC
#define BTN_PIN         GPIO_PIN_13
#define BTN_ACTIVE_LVL  GPIO_PIN_RESET   // su queste board PC13 è attivo-basso
#define BTN_DEBOUNCE_MS 30
/* USER CODE END PD */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	float T;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */

  //l’HAL è la libreria che ti permette di programmare lo STM32 senza dover mettere le mani nei registri a basso livello.
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  BSP_TSENSOR_Init();
  BSP_ACCELERO_Init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);
  //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	  //legge dal sensore di temperatura
	  T = BSP_TSENSOR_ReadTemp();

	      /*moltiplico x100 per avere un numero intero in centesimi es 31.87C --> 3187,
	        uso il cast int_16_t per castomizzare in INTERO A 16 BIT --> questo perche T è ancora float*/
	  	  int16_t T_centi = (int16_t)(T * 100.0f);


	  	  //variabile che cambia valore in base a quanto sta T_cent --> se > della soglia prende HOT se < COLD
	  	  int32_t dyn_thr_mg = (T_centi > THR_CUTOFF_CENTI) ? THR_HOT_MG : THR_COLD_MG;


	  	      /*Leggi accelerometro ripempiendo un array --> acc_row che abbiamo creato prima nello,
	  	         USER CODE BEGINE PV come variabile globale --> NB valori in milligrammi (mg)*/
	  	      BSP_ACCELERO_AccGetXYZ(acc_raw);



	  	    /*prendo i valori dei 3 assi e li definisco interia 32 bit e non più a 16 in quanto poi
	  	      farò il modulo e visto che i valori dell' accellerometro arrivano a +- 4000 mg allora
	  	      4000*4000 = 16 000 000 e quindi ho bisgono di un range molto più grande --> rischio OVERFLOW
	  	      NB USIAMO IL MODULO PERCHE E' UNA MISURA UNICA DELLA FORZA TOTALE PERCEPITA DAL SENSORE
	  	      INDIPENDENTEMENTE DALL' ORIENTAMENTO
	  	      I 3 valori che prendo da dal sensore formano un vettore e il MODULO di quel vettore è
	  	      |a| = sqrt(ax**2 + ay**" + az**2)
	  	      NB --> noi BAYPASSIAMO LA RADICE QUADRATA in quanto non mi serve sapere esattamente il valore
	  	      del modulo ma solo se sta dentro un certo range --> così facendo evito di far fare un calcolo
	  	      lungo e dispendioso di risorse dentro al ciclo while*/
	  	    int32_t ax = acc_raw[0], ay = acc_raw[1], az = acc_raw[2];
	  	    int32_t mag2 = ax*ax + ay*ay + az*az;

	  	    // MOD: banda attorno a 1g usando dyn_thr_mg
	  	    /*Costruisco una finetra di tolleranza attorno a 1g = 1000mg --> se la varibile gyn_thr_mg è grande
	  	     allora il range sarà più grande e quindi il sensore meno sensibile
	  	     NB --> visto che non usiamo il vero modulo per non fare la radice, allora dobbiamo elevare al quadrato */
	  	    const int32_t lower2 = (1000 - dyn_thr_mg) * (1000 - dyn_thr_mg);
	  	    const int32_t upper2 = (1000 + dyn_thr_mg) * (1000 + dyn_thr_mg);


	  	    /*Qua verifico se vib_strong esce dal range e se SI allora avrà valore 1
	  	     poi credo una costante stringa che con un puntatore "status" punterà a
	  	     ALARM se vib_strong è 1 o OK se sarà a 0*/
	  	    uint8_t vib_strong = (mag2 < lower2) || (mag2 > upper2);
	  	    const char* status = vib_strong ? "ALARM" : "OK";    // NEW: status per CSV


	  	    //se vibrazione forte vib_alarm = 1
	  	    if (vib_strong) {
	  	        vib_alarm = 1;
	  	    }

	  	    /*Se vib_alarm = 1 --> chiamo funzione toogle che cambia stato al pin dichiarato
	  	     se invece = 0 allora chiamo funzione Write che mi resetta il pin*/
	  	    if (vib_alarm) {
	  	        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);   // con HAL_Delay(500) → ~1 Hz
	  	    } else {
	  	        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	  	    }



	  	      /* Timestamp in millisecondi ad ogni riga e mi dirà quindi a quanti millisecondi
	  	        prendo il dato*/
	  	      uint32_t ms = HAL_GetTick();


	  	    /*creo l' intestazione dando un nome alle colonne che avro nel file csv
	  	     NB --> la funzione snprintf ha come parametri:
	  	     buf --> buffer dichiarto su che conterrà  --> ms,T_centiC,ax_mg,ay_mg,az_mg,thr_mg,status\r\n\0
	  	     sizeof(buf) --> quanto è grande il buffer (128)
	  	     e in fine --> la stringa di riferimento
	  	     NB --> questa FUNZIONE RESTITUISCE UN NUMERO INTERO "m" quindi m = 42
	  	     UART --> cosi sa grazie a m quanti byte inviare e non tutto il buffer
	  	      */
	  	    if (!header_sent) {
	  	        int m = snprintf(buf, sizeof(buf),
	  	                         "ms,T_centiC,ax_mg,ay_mg,az_mg,thr_mg,status\r\n");
	  	        /*&huart1 -->è la variabile di tipo UART_HandleTypeDef che contiene tutte
	  	         le configurazioni della tua periferica USART1 (baud rate 115200, 8N1, ecc.)
	  	         (uint8_t*)buf -->(uint8_t*)buf = cast: tratto l'array di char come array di byte (uint8_t)
                       I dati non cambiano: ogni char ASCII ('O','K',',', ecc.) è già 1 byte.
                       Serve solo perché HAL_UART_Transmit vuole un puntatore a uint8_t*.*/
	  	        HAL_UART_Transmit(&huart1, (uint8_t*)buf, m, 100);
	  	        header_sent = 1;
	  	    }



	  	  // qua fa la stessa cosa di prima ma ad ogni ciclo e con i dati
	  	  int n = snprintf(buf, sizeof(buf),
	  	                   "%lu,%d,%d,%d,%d,%d,%s\r\n",
	  	                   (unsigned long)ms,
	  	                   (int)T_centi,
	  	                   (int)acc_raw[0], (int)acc_raw[1], (int)acc_raw[2],
	  	                   (int)dyn_thr_mg,
	  	                   status);

	  	      // Invia su UART
	  	      HAL_UART_Transmit(&huart1, (uint8_t*)buf, n, 100);



	  	    /*Ciclo che dura complessivamente 500 ms ma spezzettato in 50 giri da 10 ms
	  	     cosi controllo più spesso se devo resettare col pulsante ed essere più reattivo
	  	     */
	  	    for (int i = 0; i < 50; ++i) {           // 50 * 10 ms = 500 ms
	  	        if (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == BTN_ACTIVE_LVL) {
	  	            HAL_Delay(BTN_DEBOUNCE_MS);      // debounce
	  	            if (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == BTN_ACTIVE_LVL) {
	  	                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // spegni LED
	  	                NVIC_SystemReset();          // soft reset: riparte da zero
	  	            }
	  	        }
	  	        HAL_Delay(10);
	  	    }

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
