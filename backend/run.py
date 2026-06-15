import uvicorn
import os
import sys

if __name__ == '__main__':
    # Ensure backend folder is in PYTHONPATH to import 'app' module correctly
    current_dir = os.path.dirname(os.path.abspath(__file__))
    if current_dir not in sys.path:
        sys.path.append(current_dir)
        
    # Start uvicorn server pointing to app/main.py:app
    uvicorn.run("app.main:app", host="0.0.0.0", port=8000, reload=True)
