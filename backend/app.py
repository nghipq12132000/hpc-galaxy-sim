from fastapi import FastAPI, BackgroundTasks, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import subprocess
import os
import time
import psutil
import threading
import sys
import random

app = FastAPI(title="HPC Galaxy Simulation Monitor API")

# Enable CORS for frontend integration
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Global state
class SimulationState:
    def __init__(self):
        self.is_running = False
        self.current_step = 0
        self.total_steps = 100
        self.processes = 1
        self.nodes = 1
        self.elapsed_time = 0.0
        self.dataset = "galaxy_collision_10k.txt"
        self.start_time = 0.0
        self.cmd_line = ""
        self.error_message = ""
        self.sim_thread = None

sim_state = SimulationState()

class RunConfig(BaseModel):
    processes: int
    nodes: int
    dataset: str
    steps: int
    dt: float

def run_simulation_worker(config: RunConfig):
    global sim_state
    sim_state.is_running = True
    sim_state.current_step = 0
    sim_state.total_steps = config.steps
    sim_state.processes = config.processes
    sim_state.nodes = config.nodes
    sim_state.dataset = config.dataset
    sim_state.start_time = time.time()
    sim_state.error_message = ""
    
    # Path setup
    base_dir = r"G:\My Drive\Classroom\Tính toán hiệu năng cao\Báo cáo"
    scripts_dir = os.path.join(base_dir, "Scripts")
    data_path = os.path.join(base_dir, "Data", config.dataset)
    
    # Build execution command
    # e.g., mpirun -np 4 ./nbody_mpi ../Data/galaxy_collision_10k.txt 100 0.01
    mpi_binary = os.path.join(scripts_dir, "nbody_mpi")
    if os.name == 'nt':
        mpi_binary += ".exe"
        
    cmd = ["mpirun", "-np", str(config.processes)]
    
    # Add mock hostfile configuration for multi-node simulation
    if config.nodes > 1:
        hostfile_path = os.path.join(base_dir, "backend", "hostfile")
        try:
            with open(hostfile_path, "w") as f:
                f.write("localhost slots=16\n") # Simplification for local run
            cmd.extend(["--hostfile", hostfile_path])
        except Exception as e:
            print(f"Error writing hostfile: {e}")
            
    cmd.extend([mpi_binary, data_path, str(config.steps), str(config.dt)])
    sim_state.cmd_line = " ".join(cmd)
    
    print(f"Executing: {sim_state.cmd_line}", file=sys.stderr)
    
    # Check if executable exists, compile if needed (Linux only)
    if not os.path.exists(mpi_binary) and os.name != 'nt':
        try:
            print("MPI binary not found. Attempting to compile...", file=sys.stderr)
            subprocess.run(["mpicc", "-O3", os.path.join(scripts_dir, "nbody_mpi.c"), "-o", mpi_binary, "-lm"], check=True)
        except Exception as e:
            print(f"Compilation failed: {e}", file=sys.stderr)
            
    # Run the subprocess
    try:
        # If we are on Windows and mpirun/mpi_binary doesn't exist, we fallback to a simulated run
        if os.name == 'nt' or not os.path.exists(mpi_binary):
            print("Falling back to SIMULATED MPI execution for local debugging...", file=sys.stderr)
            # Simulate steps and loads
            for step in range(config.steps):
                if not sim_state.is_running:
                    break
                time.sleep(0.05) # simulate work
                sim_state.current_step = step + 1
                sim_state.elapsed_time = time.time() - sim_state.start_time
            sim_state.is_running = False
        else:
            # Real execution
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                cwd=scripts_dir
            )
            
            # Read stdout to monitor progress
            while proc.poll() is None:
                if not sim_state.is_running:
                    proc.kill()
                    break
                line = proc.stdout.readline()
                if "Step" in line:
                    try:
                        # Parse "Step X/Y completed..."
                        parts = line.split("Step ")[1].split("/")
                        step_num = int(parts[0])
                        sim_state.current_step = step_num
                    except Exception:
                        pass
                sim_state.elapsed_time = time.time() - sim_state.start_time
                time.sleep(0.01)
                
            stdout, stderr = proc.communicate()
            if proc.returncode != 0:
                sim_state.error_message = stderr
                print(f"Subprocess error: {stderr}", file=sys.stderr)
                
            sim_state.is_running = False
    except Exception as e:
        sim_state.error_message = str(e)
        print(f"Execution failed: {e}", file=sys.stderr)
        # Fallback to simulated
        print("Falling back to SIMULATED MPI due to error...", file=sys.stderr)
        for step in range(config.steps):
            if not sim_state.is_running:
                break
            time.sleep(0.05)
            sim_state.current_step = step + 1
            sim_state.elapsed_time = time.time() - sim_state.start_time
        sim_state.is_running = False

@app.post("/api/run")
def start_simulation(config: RunConfig, background_tasks: BackgroundTasks):
    global sim_state
    if sim_state.is_running:
        raise HTTPException(status_code=400, detail="Simulation is already running")
        
    sim_state.sim_thread = threading.Thread(target=run_simulation_worker, args=(config,))
    sim_state.sim_thread.daemon = True
    sim_state.sim_thread.start()
    return {"status": "started", "cmd": sim_state.cmd_line}

@app.post("/api/stop")
def stop_simulation():
    global sim_state
    if not sim_state.is_running:
        return {"status": "idle"}
    sim_state.is_running = False
    return {"status": "stopping"}

@app.get("/api/status")
def get_status():
    global sim_state
    return {
        "isRunning": sim_state.is_running,
        "currentStep": sim_state.current_step,
        "totalSteps": sim_state.total_steps,
        "elapsedTime": round(sim_state.elapsed_time, 2),
        "processes": sim_state.processes,
        "nodes": sim_state.nodes,
        "dataset": sim_state.dataset,
        "cmdLine": sim_state.cmd_line,
        "errorMessage": sim_state.error_message
    }

@app.get("/api/monitor")
def get_monitor_stats():
    global sim_state
    
    # 1. Master load is the actual host CPU load
    master_cpu = psutil.cpu_percent()
    
    # 2. Simulate Node loads based on active run configuration
    node_loads = [2.0, 2.0, 2.0, 2.0] # default idle loads
    
    if sim_state.is_running:
        # Distribute active processes across the selected nodes
        procs = sim_state.processes
        nodes = sim_state.nodes
        
        # Base active CPU load
        active_load = 75.0 + random.uniform(-10.0, 15.0)
        active_load = min(100.0, max(0.0, active_load))
        
        for idx in range(4):
            # If the node is selected and has processes mapped
            if idx < nodes:
                # Calculate simple process load factor
                node_procs = (procs // nodes) + (1 if idx < (procs % nodes) else 0)
                if node_procs > 0:
                    node_loads[idx] = active_load * (0.6 + 0.4 * (node_procs / (16 // nodes)))
                    node_loads[idx] = min(100.0, node_loads[idx])
                else:
                    node_loads[idx] = 5.0 + random.uniform(0.0, 5.0)
            else:
                node_loads[idx] = 1.0 + random.uniform(0.0, 2.0)
    else:
        # Idle fluctuations
        for idx in range(4):
            node_loads[idx] = 2.0 + random.uniform(-1.0, 3.0)
            
    # Mock network traffic generated by MPI communications
    # Scales with the number of processes and nodes
    network_traffic = 0.0
    if sim_state.is_running:
        network_traffic = sim_state.processes * sim_state.nodes * (12.4 + random.uniform(-1.5, 3.5)) # MB/s
        
    return {
        "master": round(master_cpu, 1),
        "node1": round(node_loads[0], 1),
        "node2": round(node_loads[1], 1),
        "node3": round(node_loads[2], 1),
        "node4": round(node_loads[3], 1),
        "networkTraffic": round(network_traffic, 1)
    }

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
