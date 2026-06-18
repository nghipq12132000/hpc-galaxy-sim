from pydantic import BaseModel

class RunConfig(BaseModel):
    processes: int
    nodes: int
    dataset: str
    steps: int
    dt: float
    master_ip: str
    node_ips: list[str]

class GenerateConfig(BaseModel):
    particles: int


