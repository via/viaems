#ifndef TASKS_H
#define TASKS_H

void handle_fuel_pump();
void handle_boost_control();
void handle_idle_control();
void handle_emergency_shutdown();

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests();
#endif

#endif

