import json
import os
import shutil
from typing import Any, Dict, List, Optional
import httpx
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

OPENROUTER_URL = "https://openrouter.ai/api/v1/chat/completions"

# Filtro: expomos para a LLM apenas as ferramentas de LEITURA que queremos do GitHub MCP oficial!
ALLOWED_GITHUB_TOOLS = {
    "get_file_contents",
    "list_issues",
    "get_issue",
    "list_pull_requests",
}


def filtrar_e_converter_tools_para_openrouter(mcp_tools: List[Any]) -> List[Dict[str, Any]]:
    """
    Recebe todas as 26 ferramentas do servidor MCP oficial do GitHub,
    filtra apenas as ferramentas permitidas (ALLOWED_GITHUB_TOOLS)
    e converte para o formato JSON Schema que o OpenRouter espera.
    """
    openrouter_tools = []
    for tool in mcp_tools:
        if tool.name in ALLOWED_GITHUB_TOOLS:
            openrouter_tools.append(
                {
                    "type": "function",
                    "function": {
                        "name": tool.name,
                        "description": tool.description or "",
                        "parameters": tool.inputSchema,
                    },
                }
            )
    return openrouter_tools


def obter_comando_npx() -> str:
    """Usa o npx do Node 22 instalado localmente ou o npx padrão do sistema."""
    node22_npx = os.path.expanduser("~/.local/node22/bin/npx")
    if os.path.exists(node22_npx):
        return node22_npx
    return shutil.which("npx") or "npx"


async def executar_agente_github(
    prompt_usuario: str,
    model: Optional[str] = None,
    max_iteracoes: int = 6,
) -> Dict[str, Any]:
    """
    Orquestrador (Agent Loop):
    1. Inicia e conecta no servidor MCP oficial do GitHub (@modelcontextprotocol/server-github)
    2. Filtra apenas as tools de leitura desejadas
    3. Executa o loop enviando mensagens + tools para o OpenRouter e rodando call_tool no MCP
    """
    api_key = os.getenv("OPENROUTER_API_KEY", "").strip()
    if not api_key:
        raise ValueError("OPENROUTER_API_KEY não configurada no arquivo agent/.env")

    chosen_model = model or os.getenv("OPENROUTER_MODEL", "openrouter/free")
    github_token = os.getenv("GITHUB_PERSONAL_ACCESS_TOKEN", "").strip()

    # Ambiente para o processo do MCP oficial do GitHub
    mcp_env = os.environ.copy()
    node22_bin = os.path.expanduser("~/.local/node22/bin")
    if os.path.exists(node22_bin):
        mcp_env["PATH"] = f"{node22_bin}:{mcp_env.get('PATH', '')}"
    if github_token:
        mcp_env["GITHUB_PERSONAL_ACCESS_TOKEN"] = github_token

    server_params = StdioServerParameters(
        command=obter_comando_npx(),
        args=["-y", "@modelcontextprotocol/server-github"],
        env=mcp_env,
    )

    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2",
        "X-Title": "Xadrez-ES2 Scrum Agent",
    }

    historico_tool_calls: List[Dict[str, Any]] = []

    # Abre a conexão via protocolo MCP com a lib oficial do GitHub
    async with stdio_client(server_params) as (read_stream, write_stream):
        async with ClientSession(read_stream, write_stream) as mcp_session:
            await mcp_session.initialize()

            # 1. Pergunta ao MCP do GitHub quais tools existem e filtra apenas as nossas
            tools_result = await mcp_session.list_tools()
            tools_openrouter = filtrar_e_converter_tools_para_openrouter(tools_result.tools)

            messages: List[Dict[str, Any]] = [
                {
                    "role": "system",
                    "content": (
                        "Você é o Agente Scrum Master do repositório GitHub "
                        "owner='Alessandro-F-Ferreira-F' e repo='Xadrez-ES2'. "
                        "Sempre que precisar consultar arquivos (como 'docs/sprints.md') ou Issues do projeto, "
                        "use as ferramentas MCP disponíveis passando owner='Alessandro-F-Ferreira-F' e repo='Xadrez-ES2'. "
                        "Responda sempre em português."
                    ),
                },
                {
                    "role": "user",
                    "content": prompt_usuario,
                },
            ]

            modelo_utilizado = chosen_model

            # 2. AGENT LOOP: Conversa com o OpenRouter até o modelo não pedir mais tools
            async with httpx.AsyncClient(timeout=90.0) as http_client:
                for iteracao in range(1, max_iteracoes + 1):
                    payload = {
                        "model": chosen_model,
                        "messages": messages,
                        "tools": tools_openrouter,
                    }

                    resp = await http_client.post(OPENROUTER_URL, headers=headers, json=payload)
                    if resp.status_code != 200:
                        raise RuntimeError(f"Erro OpenRouter ({resp.status_code}): {resp.text}")

                    data = resp.json()
                    modelo_utilizado = data.get("model", chosen_model)
                    choice = data.get("choices", [{}])[0]
                    assistant_msg = choice.get("message", {})

                    # Adiciona a resposta da LLM no histórico da conversa
                    messages.append(assistant_msg)

                    tool_calls = assistant_msg.get("tool_calls")

                    # Se a LLM NÃO pediu nenhuma tool, chegamos na resposta final!
                    if not tool_calls:
                        return {
                            "modelo_solicitado": chosen_model,
                            "modelo_utilizado": modelo_utilizado,
                            "tools_expostas_ao_modelo": [t["function"]["name"] for t in tools_openrouter],
                            "total_iteracoes": iteracao,
                            "chamadas_mcp_executadas": historico_tool_calls,
                            "resposta_final": assistant_msg.get("content", ""),
                        }

                    # 3. Se a LLM pediu para chamar uma ou mais tools, executamos no MCP do GitHub!
                    for tc in tool_calls:
                        func_info = tc.get("function", {})
                        tool_name = func_info.get("name")
                        raw_args = func_info.get("arguments", "{}")

                        try:
                            tool_args = json.loads(raw_args) if isinstance(raw_args, str) else raw_args
                        except json.JSONDecodeError:
                            tool_args = {}

                        # Executa a ferramenta de verdade no servidor MCP oficial do GitHub
                        mcp_res = await mcp_session.call_tool(tool_name, tool_args)

                        # Extrai o texto retornado pelo MCP
                        texto_retorno = "\n".join(
                            item.text for item in mcp_res.content if hasattr(item, "text")
                        )

                        historico_tool_calls.append(
                            {
                                "iteracao": iteracao,
                                "tool": tool_name,
                                "argumentos": tool_args,
                                "preview_retorno": texto_retorno[:300] + ("..." if len(texto_retorno) > 300 else ""),
                            }
                        )

                        # Devolve o resultado da tool para a LLM na próxima volta do loop
                        messages.append(
                            {
                                "role": "tool",
                                "tool_call_id": tc.get("id"),
                                "name": tool_name,
                                "content": texto_retorno,
                            }
                        )

            return {
                "modelo_solicitado": chosen_model,
                "modelo_utilizado": modelo_utilizado,
                "tools_expostas_ao_modelo": [t["function"]["name"] for t in tools_openrouter],
                "total_iteracoes": max_iteracoes,
                "chamadas_mcp_executadas": historico_tool_calls,
                "resposta_final": "Limite máximo de iterações atingido.",
            }
