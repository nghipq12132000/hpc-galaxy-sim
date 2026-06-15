from pydantic import BaseModel

class RunConfig(BaseModel):
    processes: int
    nodes: int
    dataset: str
    steps: int
    dt: float
