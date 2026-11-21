// File: /home/kami/ics-guard/src/defense/plant_ode.c

#include "ics_guard/defense/plant_ode.h"
#include <math.h>

static void plant_dynamics(const plant_state_t *state, const plant_control_t *control, plant_state_t *dstate)
{
    dstate->temperature = (5.0 * control->heater_power) - (0.05 * (state->temperature - 65.0)) - (0.1 * control->valve_opening);
    dstate->vibration = (10.0 * control->pump_speed) - (0.2 * (state->vibration - 1000.0));
    dstate->pressure = (5.0 * control->pump_speed) - (8.0 * control->valve_opening) - (0.02 * (state->pressure - 100.0));
}

void plant_ode_step_rk4(plant_state_t *state, const plant_control_t *control, double dt)
{
    plant_state_t k1, k2, k3, k4;
    plant_state_t temp_state;

    plant_dynamics(state, control, &k1);

    temp_state.temperature = state->temperature + 0.5 * dt * k1.temperature;
    temp_state.vibration = state->vibration + 0.5 * dt * k1.vibration;
    temp_state.pressure = state->pressure + 0.5 * dt * k1.pressure;
    plant_dynamics(&temp_state, control, &k2);

    temp_state.temperature = state->temperature + 0.5 * dt * k2.temperature;
    temp_state.vibration = state->vibration + 0.5 * dt * k2.vibration;
    temp_state.pressure = state->pressure + 0.5 * dt * k2.pressure;
    plant_dynamics(&temp_state, control, &k3);

    temp_state.temperature = state->temperature + dt * k3.temperature;
    temp_state.vibration = state->vibration + dt * k3.vibration;
    temp_state.pressure = state->pressure + dt * k3.pressure;
    plant_dynamics(&temp_state, control, &k4);

    state->temperature += (dt / 6.0) * (k1.temperature + 2.0 * k2.temperature + 2.0 * k3.temperature + k4.temperature);
    state->vibration += (dt / 6.0) * (k1.vibration + 2.0 * k2.vibration + 2.0 * k3.vibration + k4.vibration);
    state->pressure += (dt / 6.0) * (k1.pressure + 2.0 * k2.pressure + 2.0 * k3.pressure + k4.pressure);
}

bool plant_state_is_safe(const plant_state_t *state, double max_temp_c, double max_vibration, double max_pressure)
{
    return (state->temperature <= max_temp_c) &&
           (state->vibration <= max_vibration) &&
           (state->pressure <= max_pressure);
}

double plant_cbf_evaluate(const plant_state_t *state, double max_temp_c)
{
    return max_temp_c - state->temperature;
}

double plant_cbf_dot_evaluate(const plant_state_t *state, const plant_control_t *control)
{
    plant_state_t dstate;
    plant_dynamics(state, control, &dstate);
    return -dstate.temperature;
}

bool plant_cbf_check_invariance(const plant_state_t *state, const plant_control_t *control, double max_temp_c, double alpha)
{
    double h_val = plant_cbf_evaluate(state, max_temp_c);
    double h_dot_val = plant_cbf_dot_evaluate(state, control);
    return (h_dot_val + alpha * h_val) >= 0.0;
}