import os
from typing import Optional
import httpx
from dotenv import load_dotenv
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

# Carrega as variáveis do arquivo .env
load_dotenv()

app = FastAPI(
    title="Xadrez-ES2 - Agente Scrum Master API",
    description="API REST para orquestração do Agente via OpenRouter e MCP",
    version="0.1.0",
)

OPENROUTER_URL = "https://openrouter.ai/api/v1/chat/completions"
WHATSAPP_SERVICE_URL = os.getenv("WHATSAPP_SERVICE_URL", "http://127.0.0.1:3001")


class ChatRequest(BaseModel):
    prompt: str = "Olá! Diga qual modelo você é e confirme que a conexão via OpenRouter está funcionando."
    model: Optional[str] = None


class WhatsAppMessageRequest(BaseModel):
    message: str = "🤖 Teste de conexão da API REST em Python do Agente Scrum Master (Xadrez-ES2)!"
    group_filter: str = "eu"


@app.get("/health")
async def health_check():
    """Verifica se o servidor FastAPI está online e se a chave do OpenRouter foi configurada."""
    load_dotenv(override=True)
    api_key = os.getenv("OPENROUTER_API_KEY", "").strip()
    default_model = os.getenv("OPENROUTER_MODEL", "openrouter/free")
    return {
        "status": "ok",
        "openrouter_key_configured": bool(api_key),
        "default_model": default_model,
    }


@app.post("/test-llm")
async def test_llm(body: ChatRequest):
    """
    Faz uma chamada de teste para o OpenRouter enviando o prompt recebido
    e retorna a resposta da LLM.
    """
    load_dotenv(override=True)
    api_key = os.getenv("OPENROUTER_API_KEY", "").strip()
    if not api_key:
        raise HTTPException(
            status_code=400,
            detail="OPENROUTER_API_KEY não configurada no arquivo agent/.env",
        )

    # Usa o modelo enviado no body da requisição ou o padrão do .env
    chosen_model = body.model or os.getenv("OPENROUTER_MODEL", "openrouter/free")

    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2",
        "X-Title": "Xadrez-ES2 Scrum Agent",
    }

    payload = {
        "model": chosen_model,
        "messages": [
            {
                "role": "system",
                "content": "Você é o assistente técnico do projeto Xadrez-ES2. Responda sempre em português de forma direta.",
            },
            {
                "role": "user",
                "content": body.prompt,
            },
        ],
    }

    async with httpx.AsyncClient(timeout=60.0) as client:
        response = await client.post(OPENROUTER_URL, headers=headers, json=payload)

    if response.status_code != 200:
        raise HTTPException(
            status_code=response.status_code,
            detail={
                "erro": "Falha ao chamar o OpenRouter",
                "resposta_openrouter": response.text,
            },
        )

    data = response.json()
    reply = data.get("choices", [{}])[0].get("message", {}).get("content", "")

    return {
        "modelo_solicitado": chosen_model,
        "modelo_utilizado": data.get("model", chosen_model),
        "resposta_llm": reply,
        "uso_tokens": data.get("usage", {}),
    }


@app.post("/test-agent-github")
async def test_agent_github(body: ChatRequest):
    """
    Aciona o Orquestrador (Agent Loop):
    Conecta no MCP oficial do GitHub, expõe apenas as tools filtradas de leitura
    e permite que a LLM do OpenRouter consulte o repositório Xadrez-ES2 sozinha.
    """
    from orchestrator import executar_agente_github

    load_dotenv(override=True)
    try:
        resultado = await executar_agente_github(
            prompt_usuario=body.prompt,
            model=body.model,
        )
        return resultado
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/whatsapp-status")
async def whatsapp_status(group_filter: str = "eu"):
    """Consulta o servidor REST do WhatsApp (porta 3001) para verificar se está conectado."""
    async with httpx.AsyncClient(timeout=10.0) as client:
        try:
            resp = await client.get(
                f"{WHATSAPP_SERVICE_URL}/status",
                params={"group_filter": group_filter},
            )
            return resp.json()
        except Exception as e:
            raise HTTPException(
                status_code=503,
                detail=f"Serviço do WhatsApp offline em {WHATSAPP_SERVICE_URL}: {e}",
            )


@app.post("/test-whatsapp")
async def test_whatsapp(body: WhatsAppMessageRequest):
    """Envia uma mensagem de teste via API REST do WhatsApp para o grupo filtrado (padrão 'eu')."""
    async with httpx.AsyncClient(timeout=20.0) as client:
        try:
            resp = await client.post(
                f"{WHATSAPP_SERVICE_URL}/send",
                json={"message": body.message, "group_filter": body.group_filter},
            )
        except Exception as e:
            raise HTTPException(
                status_code=503,
                detail=f"Serviço do WhatsApp offline em {WHATSAPP_SERVICE_URL}: {e}",
            )

    if resp.status_code != 200:
        raise HTTPException(status_code=resp.status_code, detail=resp.json())

    return resp.json()


class SprintReportRequest(BaseModel):
    group_filter: str = "eu"
    model: Optional[str] = None
    custom_prompt: Optional[str] = None


@app.post("/run-sprint-report")
async def run_sprint_report(body: SprintReportRequest):
    """
    Fluxo Completo Fim-a-Fim:
    1. Chama o Orquestrador (OpenRouter + GitHub MCP) para ler docs/sprints.md e as Issues do repositório;
    2. A LLM calcula o Burndown e o EVM (PV, EV, SPI) da Sprint atual e redige o relatório;
    3. Envia a resposta final via REST para o WhatsApp (no grupo 'eu' por padrão).
    """
    from orchestrator import executar_agente_github
    from datetime import datetime

    load_dotenv(override=True)

    data_hoje = datetime.now().strftime("%d/%m/%Y")

    prompt_padrao = (
        f"Você é o Agente Scrum Master do projeto Xadrez-ES2.\n"
        f"Data de referência: {data_hoje}.\n\n"
        "Siga estes passos:\n"
        "1. Use a ferramenta get_file_contents para ler 'docs/sprints.md' e use list_issues(state='all') para verificar o status real das Issues e PRs no repositório.\n"
        f"2. Identifique em qual Sprint estamos com base na data ({data_hoje}) e nas datas descritas na Seção 3 e 4 do documento.\n"
        "3. Monte um relatório executivo para WhatsApp com o seguinte formato exato (sem emojis, apenas texto limpo com títulos em negrito usando asteriscos):\n\n"
        "*STATUS DA SPRINT - XADREZ-ES2*\n"
        f"*Data:* {data_hoje} | *Sprint Atual:* [Número/Nome da Sprint]\n\n"
        "*ENTREGAS RECENTES:*\n"
        "- [Liste 2 a 3 entregas principais concluídas ou adiantadas]\n\n"
        "*FOCO ATUAL E PENDÊNCIAS:*\n"
        "- [Liste as principais pendências da Sprint e issues em aberto]\n\n"
        "*METRICAS AGEIS (EVM E BURNDOWN):*\n"
        "- *Horas Planejadas (PV):* [X]h\n"
        "- *Horas Agregadas (EV):* [Y]h\n"
        "- *SPI (EV/PV):* [Valor, ex: 1.02]\n"
        "- *Status de Prazo:* [No prazo / Adiantado / Atrasado]\n\n"
        "Mantenha a mensagem direta, concisa e sem nenhum emoji."
    )

    try:
        resultado_agente = await executar_agente_github(
            prompt_usuario=body.custom_prompt or prompt_padrao,
            model=body.model,
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro no Orquestrador/OpenRouter: {e}")

    relatorio_texto = resultado_agente.get("resposta_final", "").strip()
    if not relatorio_texto:
        raise HTTPException(status_code=500, detail="A LLM retornou uma resposta vazia.")

    # Agora envia via API REST para o nosso serviço Python do WhatsApp
    async with httpx.AsyncClient(timeout=20.0) as client:
        try:
            wa_resp = await client.post(
                f"{WHATSAPP_SERVICE_URL}/send",
                json={
                    "message": relatorio_texto,
                    "group_filter": body.group_filter,
                },
            )
        except Exception as e:
            raise HTTPException(
                status_code=503,
                detail={
                    "aviso": "O relatório foi gerado pela LLM via GitHub MCP, mas o serviço do WhatsApp está offline.",
                    "erro_whatsapp": str(e),
                    "resultado_agente": resultado_agente,
                },
            )

    if wa_resp.status_code != 200:
        raise HTTPException(
            status_code=wa_resp.status_code,
            detail={
                "erro_whatsapp": wa_resp.json(),
                "resultado_agente": resultado_agente,
            },
        )

    return {
        "status": "relatorio_gerado_e_enviado_no_whatsapp",
        "whatsapp_entrega": wa_resp.json(),
        "resultado_agente": resultado_agente,
    }



