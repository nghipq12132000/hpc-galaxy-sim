import os
from fastapi import APIRouter, HTTPException
from app.schemas.simulation import RunConfig, GenerateConfig
from app.services.simulation import SimulationService, sim_state
from app.config import DATA_DIR

router = APIRouter(prefix="/api", tags=["simulation"])

@router.post("/run")
def start_simulation(config: RunConfig):
    success, detail = SimulationService.start_simulation(config)
    if not success:
        raise HTTPException(status_code=400, detail=detail)
    return {"status": "started", "cmd": detail}

@router.post("/generate-data")
def generate_data(config: GenerateConfig):
    success, filename_or_err = SimulationService.generate_dataset(config.particles)
    if not success:
        raise HTTPException(status_code=400, detail=filename_or_err)
    return {"status": "success", "filename": filename_or_err}

@router.get("/datasets")
def get_datasets():
    try:
        if not os.path.exists(DATA_DIR):
            return []
        files = [f for f in os.listdir(DATA_DIR) if f.endswith(".txt")]
        return sorted(files)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


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
