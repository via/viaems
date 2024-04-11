#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include "platform.h"

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define configUSE_PREEMPTION		1
#define configUSE_TIME_SLICING 0
#define configUSE_IDLE_HOOK		0
#define configUSE_TICK_HOOK		0
#define configCPU_CLOCK_HZ		( ( unsigned long ) 168000000 )	
#define configTICK_RATE_HZ		( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 8 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define configSUPPORT_STATIC_ALLOCATION 1
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_MUTEXES		0
#define configCHECK_FOR_STACK_OVERFLOW	0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configGENERATE_RUN_TIME_STATS 0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE() cycle_count()
#define configQUEUE_REGISTRY_SIZE 4
#define configSTACK_DEPTH_TYPE unsigned int

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet	0
#define INCLUDE_uxTaskPriorityGet	0
#define INCLUDE_vTaskDelete		0
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend		1
#define INCLUDE_vTaskDelayUntil		0
#define INCLUDE_vTaskDelay		1
#define INCLUDE_xEventGroupSetBitFromISR 1

#define configKERNEL_INTERRUPT_PRIORITY 	255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	128 /* equivalent to 0x80, or priority 8. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	8
#define configSYSTEM_CALL_STACK_SIZE 100
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY 1

#define configUSE_MPU_WRAPPERS_V1 0
#define configPROTECTED_KERNEL_OBJECT_POOL_SIZE 150
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS 0

#endif /* FREERTOS_CONFIG_H */
