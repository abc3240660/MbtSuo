/* 
 * File:   015_ADC0.h
 * Author: Administrator
 *
 * Created on August 11, 2019, 10:05 PM
 */

#ifndef ADC0_H
#define	ADC0_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h>
#include <stdbool.h>

void ADC0_Init(void);
bool ADC0_GetValue(uint32_t *value);

#ifdef	__cplusplus
}
#endif

#endif	/* ADC0_H */

