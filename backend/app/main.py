from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routers import simulation, monitor

app = FastAPI(
    title="HPC Galaxy Simulation Monitor API",
    description="Professional modular API structure for N-Body Galaxy Collision simulation",
    version="1.0.0"
)

# Enable CORS for frontend integration
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include modular routers
app.include_router(simulation.router)
app.include_router(monitor.router)
