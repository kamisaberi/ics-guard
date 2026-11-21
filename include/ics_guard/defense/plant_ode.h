// File: /home/kami/ics-guard/include/ics_guard/defense/plant_ode.h

#ifndef ICS_GUARD_PLANT_ODE_H
#define ICS_GUARD_PLANT_ODE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double temperature; // Thermal state (deg C)
    double vibration;   // RPM / Mechanical vibration
    double pressure;    // Hydraulic pressure (PSI)
} plant_state_t;

typedef struct {
    double heater_power;  // Control u1: 0.0 to 1.0
    double pump_speed;    // Control u2: 0.0 to 1.0
    double valve_opening; // Control u3: 0.0 to 1.0
} plant_control_t;

void plant_ode_step_rk4(plant_state_t *state, const plant_control_t *control, double dt);
bool plant_state_is_safe(const plant_state_t *state, double max_temp_c, double max_vibration, double max_pressure);
double plant_cbf_evaluate(const plant_state_t *state, double max_temp_c);
double plant_cbf_dot_evaluate(const plant_state_t *state, const plant_control_t *control);
bool plant_cbf_check_invariance(const plant_state_t *state, const plant_control_t *control, double max_temp_c, double alpha);

#ifdef __cplusplus
}
#endif

#endif // ICS_GUARD_PLANT_ODE_H