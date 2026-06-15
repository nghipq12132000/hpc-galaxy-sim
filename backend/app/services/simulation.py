import os
import sys
import time
import subprocess
import threading
from app.config import SCRIPTS_DIR, DATA_DIR, BACKEND_DIR
from app.schemas.simulation import RunConfig

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

# Global state instance shared across service and routers
sim_state = SimulationState()

class SimulationService:
    @staticmethod
    def run_worker(config: RunConfig):
        global sim_state
        sim_state.is_running = True
        sim_state.current_step = 0
        sim_state.total_steps = config.steps
        sim_state.processes = config.processes
        sim_state.nodes = config.nodes
        sim_state.dataset = config.dataset
        sim_state.start_time = time.time()
        sim_state.error_message = ""
        
        # Resolve dataset absolute path
        data_path = os.path.join(DATA_DIR, config.dataset)
        
        # Build execution command
        mpi_binary = os.path.join(SCRIPTS_DIR, "nbody_mpi")
        if os.name == 'nt':
            mpi_binary += ".exe"
            
        cmd = ["mpirun", "-np", str(config.processes)]
        
        # Add mock hostfile configuration for multi-node simulation
        if config.nodes > 1:
            hostfile_path = os.path.join(BACKEND_DIR, "hostfile")
            try:
                with open(hostfile_path, "w") as f:
                    f.write("localhost slots=16\n")
                cmd.extend(["--hostfile", hostfile_path])
            except Exception as e:
                print(f"Error writing hostfile: {e}", file=sys.stderr)
                
        cmd.extend([mpi_binary, data_path, str(config.steps), str(config.dt)])
        sim_state.cmd_line = " ".join(cmd)
        
        print(f"Executing: {sim_state.cmd_line}", file=sys.stderr)
        
        # Compile if missing (Linux only)
        if not os.path.exists(mpi_binary) and os.name != 'nt':
            try:
                print("MPI binary not found. Attempting to compile...", file=sys.stderr)
                subprocess.run(["mpicc", "-O3", os.path.join(SCRIPTS_DIR, "nbody_mpi.c"), "-o", mpi_binary, "-lm"], check=True)
            except Exception as e:
                print(f"Compilation failed: {e}", file=sys.stderr)
                
        # Run subprocess
        try:
            # Fallback to simulation if on Windows or executable is missing
            if os.name == 'nt' or not os.path.exists(mpi_binary):
                print("Falling back to SIMULATED MPI execution for local debugging...", file=sys.stderr)
                for step in range(config.steps):
                    if not sim_state.is_running:
                        break
                    time.sleep(0.05)
                    sim_state.current_step = step + 1
                    sim_state.elapsed_time = time.time() - sim_state.start_time
                sim_state.is_running = False
            else:
                proc = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    cwd=SCRIPTS_DIR
                )
                
                while proc.poll() is None:
                    if not sim_state.is_running:
                        proc.kill()
                        break
                    line = proc.stdout.readline()
                    if "Step" in line:
                        try:
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
            print("Falling back to SIMULATED MPI due to error...", file=sys.stderr)
            for step in range(config.steps):
                if not sim_state.is_running:
                    break
                time.sleep(0.05)
                sim_state.current_step = step + 1
                sim_state.elapsed_time = time.time() - sim_state.start_time
            sim_state.is_running = False

    @staticmethod
    def start_simulation(config: RunConfig):
        global sim_state
        if sim_state.is_running:
            return False, "Simulation is already running"
            
        sim_state.sim_thread = threading.Thread(target=SimulationService.run_worker, args=(config,))
        sim_state.sim_thread.daemon = True
        sim_state.sim_thread.start()
        return True, sim_state.cmd_line

    @staticmethod
    def stop_simulation():
        global sim_state
        if not sim_state.is_running:
            return "idle"
        sim_state.is_running = False
        return "stopping"
