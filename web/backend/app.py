
from fastapi import FastAPI

from .explain import router as explain_router

app = FastAPI(title="SapChess Move Explainer")

app.include_router(explain_router)


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}
