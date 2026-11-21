// File: /home/kami/ics-guard/tests/test_cbf_invariance.cpp

#include "ics_guard/pipeline/guard_engine.hpp"
#include <cassert>
#include <iostream>

int main()
{
    std::cout << "[TEST] RK4 Plant ODE & Nagumo CBF Safety Invariance... ";

    ics_guard::pipeline::GuardEngine engine;
    plant_state_t st{85.0, 1200.0, 100.0};
    const plant_control_t nom_ctrl{0.2, 0.5, 0.1};

    for (int i = 0; i < 10; ++i)
    {
        plant_ode_step_rk4(&st, &nom_ctrl, 0.1);
    }
    assert(engine.process_plant_cbf(st, nom_ctrl) == ics_guard::GuardStatus::PASS);

    // Exothermic runaway state (> 120 C max_temp_c)
    const plant_state_t runaway{134.2, 3600.0, 168.5};
    const plant_control_t attack_ctrl{1.0, 1.0, 0.0};
    assert(engine.process_plant_cbf(runaway, attack_ctrl) == ics_guard::GuardStatus::REJECT_CBF_INVARIANCE_BREACH);
    assert(engine.is_airgap_tripped() == true);

    std::cout << "PASSED\n";
    return 0;
}