import os

# Backend root directory
BACKEND_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Repository root directory
REPO_DIR = os.path.dirname(BACKEND_DIR)

# Scripts directory
SCRIPTS_DIR = os.path.join(REPO_DIR, "scripts")

# Data directory
DATA_DIR = os.path.join(REPO_DIR, "data")
