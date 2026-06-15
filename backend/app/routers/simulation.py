from fastapi import APIRouter, HTTPException
from app.schemas.simulation import RunConfig
from app.services.simulation import SimulationService, sim_state

router = APIRouter(prefix="/api", tags=["simulation"])

@router.post("/run")
def start_simulation(config: RunConfig):
    success, detail = SimulationService.start_simulation(config)
    if not success:
        raise HTTPException(status_code=400, detail=detail)
    return {"status": "started", "cmd": detail}

@router.post("/stop")
def stop_simulation():
    status = SimulationService.stop_simulation()
    return {"status": status}

@router.get("/status")
def get_status():
    return {
        "isRunning": sim_state.is_running,
        "currentStep": sim_state.current_step,
        "totalSteps": sim_state.total_steps,
        "elapsedTime": round(sim_state.elapsed_time, 2),
        "processes": sim_state.processes,
        "nodes": sim_state.nodes,
        "dataset": sim_state.dataset,
        "cmdLine": sim_state.cmd_line,
        "errorMessage": sim_state.error_message,
        "logs": sim_state.logs
    }
