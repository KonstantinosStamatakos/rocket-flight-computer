/*
 * flight_app.h
 *
 *  Created on: Jun 9, 2026
 *      Author: deanstamatakos
 */

#ifndef FLIGHT_APP_H
#define FLIGHT_APP_H

/*
 * ============================================================
 * flight_app.h
 * ------------------------------------------------------------
 * This is the top-level application interface.
 *
 * The idea:
 * - main.c should stay simple
 * - all real flight logic will live in flight_app.c
 *
 * main.c only:
 *   1. initializes hardware
 *   2. calls FlightApp_Init()
 *   3. calls FlightApp_Run() forever
 * ============================================================
 */

void FlightApp_Init(void);
void FlightApp_Run(void);

#endif
