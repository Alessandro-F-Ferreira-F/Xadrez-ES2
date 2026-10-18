import io
import threading
import unicodedata
from typing import Optional
import segno
import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse
from neonize.client import NewClient
from neonize.events import ConnectedEv, PairStatusEv
from pydantic import BaseModel

app = FastAPI(title="WhatsApp Service (100% Python - Neonize)")

# Inicializa o cliente WhatsApp salvando a sessão no SQLite local
wa_client = NewClient("whatsapp_session.sqlite3")
is_connected = False
latest_qr_svg: Optional[str] = None


def normalize_text(text: str) -> str:
    """Remove acentos, converte para lowercase e remove espaços extras."""
    nfd = unicodedata.normalize("NFD", text or "")
    without_accents = "".join(ch for ch in nfd if unicodedata.category(ch) != "Mn")
    return without_accents.lower().strip()


def find_group_by_name(group_filter: str = "eu"):
    """
    Busca nos grupos do WhatsApp usando lowercase normalizado:
    1º Tenta match exato (ideal para o grupo 'EU')
    2º Tenta startswith (ideal para 'Pequenas Sereias <emoji>')
    3º Tenta substring (fallback)
    """
    target = normalize_text(group_filter)
    groups = wa_client.get_joined_groups()

    exact_match = None
    prefix_match = None
    substring_match = None

    for g in groups:
        raw_name = g.GroupName.Name or ""
        norm_name = normalize_text(raw_name)

        if norm_name == target:
            exact_match = g
            break
        elif norm_name.startswith(target) and prefix_match is None:
            prefix_match = g
        elif target in norm_name and substring_match is None:
            substring_match = g

    return exact_match or prefix_match or substring_match


@wa_client.event.qr
def on_qr(_: NewClient, data_qr: bytes):
    global latest_qr_svg
    qr = segno.make_qr(data_qr)
    buf = io.BytesIO()
    qr.save(buf, kind="svg", scale=8, dark="#111111", light="#ffffff")
    latest_qr_svg = buf.getvalue().decode("utf-8")
    print("\n📲 Novo QR Code gerado! Escaneie abaixo ou abra no navegador: http://127.0.0.1:3001/qr\n")
    qr.terminal(compact=True)


@wa_client.event(PairStatusEv)
def on_paired(_: NewClient, message: PairStatusEv):
    print(f"\n✅ Aparelho pareado com sucesso! ID: {message.ID.User}")


@wa_client.event(ConnectedEv)
def on_connected(_: NewClient, __: ConnectedEv):
    global is_connected, latest_qr_svg
    is_connected = True
    latest_qr_svg = None
    print("\n🚀 WhatsApp conectado via Python (Neonize)!")
    try:
        grupo_eu = find_group_by_name("eu")
        if grupo_eu:
            print(f"🎯 Grupo de teste localizado: '{grupo_eu.GroupName.Name}'")
        else:
            print("ℹ️ Conectado! (Grupo 'EU' ainda não localizado ou com outro nome)")
    except Exception as e:
        print(f"⚠️ Aviso ao listar grupos iniciais: {e}")


class SendRequest(BaseModel):
    message: str
    group_filter: Optional[str] = "eu"


@app.get("/qr", response_class=HTMLResponse)
def get_qr_page():
    """Exibe o QR Code diretamente no navegador (útil local ou em servidor remoto)."""
    if is_connected:
        return """
        <html><body style="font-family:sans-serif;text-align:center;padding:40px;">
            <h2>✅ WhatsApp já está conectado!</h2>
            <p>Você já pode fechar esta página e disparar os relatórios.</p>
        </body></html>
        """
    if not latest_qr_svg:
        return """
        <html><head><meta http-equiv="refresh" content="2"></head>
        <body style="font-family:sans-serif;text-align:center;padding:40px;">
            <h2>⏳ Gerando QR Code do WhatsApp...</h2>
            <p>Esta página atualiza sozinha a cada 2 segundos.</p>
        </body></html>
        """
    return f"""
    <html><head><meta http-equiv="refresh" content="15"></head>
    <body style="font-family:sans-serif;text-align:center;padding:40px;">
        <h2>📲 Escaneie o QR Code no seu WhatsApp</h2>
        <p>Aparelhos Conectados &rarr; Conectar um aparelho</p>
        <div style="margin:20px auto;display:inline-block;padding:16px;border:2px solid #ccc;border-radius:12px;">
            {latest_qr_svg}
        </div>
    </body></html>
    """


@app.get("/status")
def status(group_filter: str = "eu"):
    found_group_name = None
    if is_connected:
        try:
            g = find_group_by_name(group_filter)
            if g:
                found_group_name = g.GroupName.Name
        except Exception:
            pass

    return {
        "connected": bool(is_connected),
        "group_filter": group_filter,
        "target_group_found": bool(found_group_name),
        "target_group_name": found_group_name,
    }


@app.post("/send")
def send_message(body: SendRequest):
    if not is_connected:
        raise HTTPException(
            status_code=503,
            detail="WhatsApp ainda não está conectado. Abra http://127.0.0.1:3001/qr para escanear o QR Code.",
        )

    msg = body.message.strip()
    if not msg:
        raise HTTPException(status_code=400, detail="A mensagem não pode ser vazia.")

    filtro = body.group_filter or "eu"
    grupo = find_group_by_name(filtro)
    if not grupo:
        raise HTTPException(
            status_code=404,
            detail=f"Nenhum grupo correspondente ao filtro '{filtro}' (lowercase) foi encontrado.",
        )

    wa_client.send_message(grupo.JID, msg)
    return {
        "status": "ok",
        "sent_to": grupo.GroupName.Name,
        "filter_used": filtro,
    }


def start_wa_thread():
    """Roda a conexão WebSocket do WhatsApp em uma thread dedicada (reconecta se o QR expirar)."""
    import time

    while True:
        try:
            wa_client.connect()
        except Exception as e:
            print(f"⚠️ Reconectando WhatsApp: {e}")
        time.sleep(2)


if __name__ == "__main__":
    print("📲 Iniciando cliente WhatsApp em Python (Neonize) na porta 3001...")
    t = threading.Thread(target=start_wa_thread, daemon=True)
    t.start()
    uvicorn.run(app, host="127.0.0.1", port=3001, log_level="warning")
